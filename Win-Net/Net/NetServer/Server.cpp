/*
	MIT License

	Copyright (c) 2022 Tobias Staack

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include <NetServer/Server.h>
#include <Net/Import/Kernel32.hpp>
#include <Net/Import/Ws2_32.hpp>

Net::Server::IPRef::IPRef(const char* pointer)
{
	this->pointer = (char*)pointer;
}

Net::Server::IPRef::~IPRef()
{
	FREE<char>(pointer);
}

const char* Net::Server::IPRef::get() const
{
	return pointer;
}

Net::Server::Server::Server()
{
	SetListenSocket(INVALID_SOCKET);
	SetRunning(false);
	curTime = 0;
	hSyncClockNTP = nullptr;
	hReSyncClockNTP = nullptr;
	optionBitFlag = 0;
	socketOptionBitFlag = 0;
}

Net::Server::Server::~Server()
{
	for (auto& entry : socketoption)
		FREE<SocketOptionInterface_t>(entry);

	socketoption.clear();

	for (auto& entry : option)
		FREE<OptionInterface_t>(entry);

	option.clear();
}

bool Net::Server::Server::Isset(const DWORD opt) const
{
	// use the bit flag to perform faster checks
	return optionBitFlag & opt;
}

bool Net::Server::Server::Isset_SocketOpt(const DWORD opt) const
{
	// use the bit flag to perform faster checks
	return socketOptionBitFlag & opt;
}

#pragma region Network Structure
void Net::Server::Server::network_t::setData(byte* pointer)
{
	deallocData();
	_data = pointer;
}

void Net::Server::Server::network_t::allocData(const size_t size)
{
	clear();
	_data = ALLOC<byte>(size + 1);
	memset(getData(), NULL, size * sizeof(byte));
	getData()[size] = '\0';

	setDataSize(size);
}

void Net::Server::Server::network_t::deallocData()
{
	_data.free();
}

byte* Net::Server::Server::network_t::getData() const
{
	return _data.get();
}

void Net::Server::Server::network_t::reset()
{
	memset(_dataReceive, NULL, NET_OPT_DEFAULT_MAX_PACKET_SIZE);
}

void Net::Server::Server::network_t::clear()
{
	deallocData();
	_data_size = 0;
	_data_full_size = 0;
	_data_offset = 0;
	_data_original_uncompressed_size = 0;
}

void Net::Server::Server::network_t::setDataSize(const size_t size)
{
	_data_size = size;
}

size_t Net::Server::Server::network_t::getDataSize() const
{
	return _data_size;
}

void Net::Server::Server::network_t::setDataFullSize(const size_t size)
{
	_data_full_size = size;
}

size_t Net::Server::Server::network_t::getDataFullSize() const
{
	return _data_full_size;
}

void Net::Server::Server::network_t::SetDataOffset(const size_t offset)
{
	_data_offset = offset;
}

size_t Net::Server::Server::network_t::getDataOffset() const
{
	return _data_offset;
}

void Net::Server::Server::network_t::SetUncompressedSize(const size_t size)
{
	_data_original_uncompressed_size = size;
}

size_t Net::Server::Server::network_t::getUncompressedSize() const
{
	return _data_original_uncompressed_size;
}

bool Net::Server::Server::network_t::dataValid() const
{
	return _data.valid();
}

byte* Net::Server::Server::network_t::getDataReceive()
{
	return _dataReceive;
}
#pragma endregion

#pragma region Cryption Structure
void Net::Server::Server::cryption_t::createKeyPair(const size_t size)
{
	RSA.generateKeys(size, 3);
	setHandshakeStatus(false);
}

void Net::Server::Server::cryption_t::deleteKeyPair()
{
	RSA.deleteKeys();
	setHandshakeStatus(false);
}

void Net::Server::Server::cryption_t::setHandshakeStatus(const bool status)
{
	RSAHandshake = status;
}

bool Net::Server::Server::cryption_t::getHandshakeStatus() const
{
	return RSAHandshake;
}
#pragma endregion

struct	 CalcLatency_t
{
	Net::Server::Server* server;
	Net::Server::Server::peerInfo* peer;
};

NET_TIMER(CalcLatency)
{
	const auto info = (CalcLatency_t*)param;
	if (!info) NET_STOP_TIMER;

	const auto server = info->server;
	const auto peer = info->peer;

	// tmp: disabled till linux support for icmp
	//	peer->latency = Net::Protocol::ICMP::Exec(peer->IPAddr().get());

	Net::Timer::SetTime(peer->hCalcLatency, server->Isset(NET_OPT_INTERVAL_LATENCY) ? server->GetOption<int>(NET_OPT_INTERVAL_LATENCY) : NET_OPT_DEFAULT_INTERVAL_LATENCY);
	NET_CONTINUE_TIMER;
}

NET_TIMER(NTPSyncClock)
{
	const auto server = (Net::Server::Server*)param;
	if (!server) NET_STOP_TIMER;

	tm* tm = localtime(&server->curTime);
	tm->tm_sec += 1;
	server->curTime = mktime(tm);
	NET_CONTINUE_TIMER;
}

NET_TIMER(NTPReSyncClock)
{
	const auto server = (Net::Server::Server*)param;
	if (!server) NET_STOP_TIMER;

	auto time = Net::Protocol::NTP::Exec(server->Isset(NET_OPT_NTP_HOST) ? server->GetOption<char*>(NET_OPT_NTP_HOST) : NET_OPT_DEFAULT_NTP_HOST,
		server->Isset(NET_OPT_NTP_PORT) ? server->GetOption<u_short>(NET_OPT_NTP_PORT) : NET_OPT_DEFAULT_NTP_PORT);

	if (!time.valid())
		NET_CONTINUE_TIMER;

	server->curTime = (time_t)(time.frame().txTm_s - NTP_TIMESTAMP_DELTA);

	Net::Timer::SetTime(server->hReSyncClockNTP, server->Isset(NET_OPT_NTP_SYNC_INTERVAL) ? server->GetOption<int>(NET_OPT_NTP_SYNC_INTERVAL) : NET_OPT_DEFAULT_NTP_SYNC_INTERVAL);
	NET_CONTINUE_TIMER;
}

Net::Server::Server::peerInfo* Net::Server::Server::CreatePeer(const sockaddr_in client_addr, const SOCKET socket)
{
	// UniqueID is equal to socket, since socket is already an unique ID
	const auto peer = ALLOC<NET_IPEER>();
	peer->UniqueID = socket;
	peer->pSocket = socket;
	peer->client_addr = client_addr;

	/* Set Read Timeout */
	timeval tv = {};
	tv.tv_sec = Isset(NET_OPT_TIMEOUT_TCP_READ) ? GetOption<long>(NET_OPT_TIMEOUT_TCP_READ) : NET_OPT_DEFAULT_TIMEOUT_TCP_READ;
	tv.tv_usec = 0;
	Ws2_32::setsockopt(peer->pSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof tv);

	// Set socket options
	for (const auto& entry : socketoption)
	{
		const auto res = Ws2_32::setsockopt(peer->pSocket, entry->level, entry->opt, entry->value(), entry->optlen());
		if (res == SOCKET_ERROR) NET_LOG_ERROR(CSTRING("'%s' => unable to apply socket option { %i : %i }"), SERVERNAME(this), entry->opt, LAST_ERROR);
	}

	if (Isset(NET_OPT_DISABLE_LATENCY_REQUEST) ? GetOption<bool>(NET_OPT_DISABLE_LATENCY_REQUEST) : NET_OPT_DEFAULT_LATENCY_REQUEST)
	{
		const auto _CalcLatency = ALLOC<CalcLatency_t>();
		_CalcLatency->server = this;
		_CalcLatency->peer = peer;
		//peer->hCalcLatency = Timer::Create(CalcLatency, Isset(NET_OPT_INTERVAL_LATENCY) ? GetOption<int>(NET_OPT_INTERVAL_LATENCY) : NET_OPT_DEFAULT_INTERVAL_LATENCY, _CalcLatency, true);
	}

	if (CreateTOTPSecret(peer))
		NET_LOG_PEER(CSTRING("'%s' :: [%s] => created TOTP-Hash"), SERVERNAME(this), peer->IPAddr().get());

	NET_LOG_PEER(CSTRING("'%s' :: [%s] => connected"), SERVERNAME(this), peer->IPAddr().get());

	// callback
	OnPeerConnect(peer);

	return peer;
}

bool Net::Server::Server::ErasePeer(NET_PEER peer, bool clear)
{
	PEER_NOT_VALID(peer,
		return false;
	);

	if (clear)
	{
		// close endpoint
		SOCKET_VALID(peer->pSocket)
		{
			bool bBlocked = false;
			do
			{
				bBlocked = false;
				Ws2_32::shutdown(peer->pSocket, SOCKET_WR);
				if (Ws2_32::closesocket(peer->pSocket) == SOCKET_ERROR)
				{
#ifdef BUILD_LINUX
					if (errno == EWOULDBLOCK)
#else
					if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
#endif
					{
						bBlocked = true;
#ifdef BUILD_LINUX
						usleep(FREQUENZ(this) * 1000);
#else
						Kernel32::Sleep(FREQUENZ(this));
#endif
					}
				}

			} while (bBlocked);

			peer->pSocket = INVALID_SOCKET;
		}

		Net::Timer::WaitSingleObjectStopped(peer->hWaitForNetProtocol);
		peer->hWaitForNetProtocol = nullptr;

		if (peer->hCalcLatency)
		{
			// stop latency interval
			//Timer::WaitSingleObjectStopped(peer->hCalcLatency);
			peer->hCalcLatency = nullptr;
		}

		// callback
#ifdef BUILD_LINUX
		OnPeerDisconnect(peer, errno);
#else
		OnPeerDisconnect(peer, Ws2_32::WSAGetLastError());
#endif

		NET_LOG_PEER(CSTRING("'%s' :: [%s] => finished"), SERVERNAME(this), peer->IPAddr().get());

		peer->clear();

		return true;
	}

	peer->bErase = true;
	return true;
}

size_t Net::Server::Server::GetNextPacketSize(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return 0;
	);

	return peer->network.getDataFullSize();
}

size_t Net::Server::Server::GetReceivedPacketSize(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return 0;
	);

	return peer->network.getDataSize();
}

float Net::Server::Server::GetReceivedPacketSizeAsPerc(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return 0.0f;
	);

	// Avoid dividing zero with zero
	if (peer->network.getDataSize() <= 0)
		return 0.0f;

	if (peer->network.getDataFullSize() <= 0)
		return 0;

	auto perc = static_cast<float>(peer->network.getDataSize()) / static_cast<float>(peer->network.getDataFullSize()) * 100;
	if (perc < 0.0f) // should never happen, just in case
		perc = 0.0f;
	else if (perc > 100.0f) // could happen, as we play around with the sizes later on
		perc = 100.0f;

	return perc;
}

void Net::Server::Server::peerInfo::clear()
{
	UniqueID = INVALID_UID;
	pSocket = INVALID_SOCKET;
	client_addr = sockaddr_in();
	estabilished = false;
	NetVersionMatched = false;
	bErase = false;
	latency = -1;
	hCalcLatency = nullptr;

	network.clear();
	network.reset();

	cryption.deleteKeyPair();

	FREE<byte>(totp_secret);
	totp_secret_len = 0;
	curToken = 0;
	lastToken = 0;
}

typeLatency Net::Server::Server::peerInfo::getLatency() const
{
	return latency;
}

Net::Server::IPRef Net::Server::Server::peerInfo::IPAddr() const
{
	const auto buf = ALLOC<char>(INET_ADDRSTRLEN);
	return IPRef(Ws2_32::inet_ntop(AF_INET, &client_addr.sin_addr, buf, INET_ADDRSTRLEN));
}

void Net::Server::Server::DisconnectPeer(NET_PEER peer, const int code, const bool skipNotify)
{
	PEER_NOT_VALID(peer,
		return;
	);

	/*
	* NET_OPT_EXECUTE_PACKET_ASYNC allow packet execution in different threads
	* that threads might call DisconnectPeer
	* soo require a mutex to block it
	*/
	std::lock_guard<std::mutex> guard(peer->_mutex_disconnectPeer);

	if (!skipNotify)
	{
		NET_PACKET PKG;
		PKG[CSTRING("code")] = code;
		NET_SEND(peer, NET_NATIVE_PACKET_ID::PKG_Close, pkg);
	}

	if (code == 0)
	{
		NET_LOG_PEER(CSTRING("'%s' :: [%s] => disconnected"), SERVERNAME(this), peer->IPAddr().get());
	}
	else
	{
		NET_LOG_PEER(CSTRING("'%s' :: [%s] => disconnected due to the following reason '%s'"), SERVERNAME(this), peer->IPAddr().get(), Net::Codes::NetGetErrorMessage(code));
	}

	ErasePeer(peer);
}

void Net::Server::Server::SetListenSocket(const SOCKET ListenSocket)
{
	this->ListenSocket = ListenSocket;
}

void Net::Server::Server::SetRunning(const bool bRunning)
{
	this->bRunning = bRunning;
}

SOCKET Net::Server::Server::GetListenSocket() const
{
	return ListenSocket;
}

bool Net::Server::Server::IsRunning() const
{
	return bRunning;
}

NET_THREAD(TickThread)
{
	const auto server = (Net::Server::Server*)parameter;
	if (!server) return 0;

	while (server->IsRunning())
	{
		server->Tick();
#ifdef BUILD_LINUX
		usleep(FREQUENZ(server) * 1000);
#else
		Kernel32::Sleep(FREQUENZ(server));
#endif
	}

	return 0;
}

NET_THREAD(AcceptorThread)
{
	const auto server = (Net::Server::Server*)parameter;
	if (!server) return 0;

	while (server->IsRunning())
	{
		server->Acceptor();
#ifdef BUILD_LINUX
		usleep(FREQUENZ(server) * 1000);
#else
		Kernel32::Sleep(FREQUENZ(server));
#endif
	}

	return 0;
}

#ifdef BUILD_LINUX
static void usleep_wrapper(DWORD duration)
{
	usleep(duration * 1000);
}
#endif

bool Net::Server::Server::Run()
{
	if (IsRunning())
		return false;

	// our sockets for the server
	SetListenSocket(INVALID_SOCKET);

	// address info for the server to listen to
	addrinfo* result = nullptr;
	int res = 0;

#ifndef BUILD_LINUX
	WSADATA wsaData;
	res = Ws2_32::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NULL)
	{
		NET_LOG_ERROR(CSTRING("'%s' => [WSAStartup] failed with error: %d"), SERVERNAME(this), res);
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		NET_LOG_ERROR(CSTRING("'%s' => unable to find usable version of [Winsock.dll]"), SERVERNAME(this));
		Ws2_32::WSACleanup();
		return false;
	}
#endif

	// set address information
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	const auto Port = std::to_string(Isset(NET_OPT_PORT) ? GetOption<u_short>(NET_OPT_PORT) : NET_OPT_DEFAULT_PORT);
	res = Ws2_32::getaddrinfo(NULLPTR, Port.data(), &hints, &result);

	if (res != 0) {
		NET_LOG_ERROR(CSTRING("'%s' => [getaddrinfo] failed with error: %d"), SERVERNAME(this), res);
#ifndef BUILD_LINUX
		Ws2_32::WSACleanup();
#endif
		return false;
	}

	// Create a SOCKET for connecting to server
	SetListenSocket(Ws2_32::socket(result->ai_family, result->ai_socktype, result->ai_protocol));

	if (GetListenSocket() == INVALID_SOCKET)
	{
		NET_LOG_ERROR(CSTRING("'%s' => creation of a listener socket failed with error: %ld"), SERVERNAME(this), LAST_ERROR);
		Ws2_32::freeaddrinfo(result);
#ifndef BUILD_LINUX
		Ws2_32::WSACleanup();
#endif
		return false;
	}

	// Set the mode of the socket to be nonblocking
	unsigned long iMode = 1;
	res = Ws2_32::ioctlsocket(GetListenSocket(), FIONBIO, &iMode);

	if (res == SOCKET_ERROR)
	{
		NET_LOG_ERROR(CSTRING("'%s' => [ioctlsocket] failed with error: %d"), SERVERNAME(this), LAST_ERROR);
		Ws2_32::closesocket(GetListenSocket());
#ifndef BUILD_LINUX
		Ws2_32::WSACleanup();
#endif
		return false;
	}

	// Setup the TCP listening socket
	res = Ws2_32::bind(GetListenSocket(), result->ai_addr, static_cast<int>(result->ai_addrlen));

	if (res == SOCKET_ERROR) {
		NET_LOG_ERROR(CSTRING("'%s' => [bind] failed with error: %d"), SERVERNAME(this), LAST_ERROR);
		Ws2_32::freeaddrinfo(result);
		Ws2_32::closesocket(GetListenSocket());
#ifndef BUILD_LINUX
		Ws2_32::WSACleanup();
#endif
		return false;
	}

	// no longer need address information
	Ws2_32::freeaddrinfo(result);

	// start listening for new clients attempting to connect
	res = Ws2_32::listen(GetListenSocket(), SOMAXCONN);

	if (res == SOCKET_ERROR)
	{
		NET_LOG_ERROR(CSTRING("'%s' => listen failed with error: %d"), SERVERNAME(this), LAST_ERROR);
		Ws2_32::closesocket(GetListenSocket());
#ifndef BUILD_LINUX
		Ws2_32::WSACleanup();
#endif
		return false;
	}

	// Create all needed Threads
	// spawn timer thread to sync clock with ntp - only effects having 2-step enabled
	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		curTime = time(nullptr);
		if (Isset(NET_OPT_USE_NTP) ? GetOption<bool>(NET_OPT_USE_NTP) : NET_OPT_DEFAULT_USE_NTP)
		{
			auto time = Net::Protocol::NTP::Exec(Isset(NET_OPT_NTP_HOST) ? GetOption<char*>(NET_OPT_NTP_HOST) : NET_OPT_DEFAULT_NTP_HOST,
				Isset(NET_OPT_NTP_PORT) ? GetOption<u_short>(NET_OPT_NTP_PORT) : NET_OPT_DEFAULT_NTP_PORT);

			if (!time.valid())
				return false;

			curTime = (time_t)(time.frame().txTm_s - NTP_TIMESTAMP_DELTA);

			hSyncClockNTP = Timer::Create(NTPSyncClock, 1000, this);
			hReSyncClockNTP = Timer::Create(NTPReSyncClock, Isset(NET_OPT_NTP_SYNC_INTERVAL) ? GetOption<int>(NET_OPT_NTP_SYNC_INTERVAL) : NET_OPT_DEFAULT_NTP_SYNC_INTERVAL, this);
		}
	}

	PeerPoolManager.set_max_peers(4);

	PeerPoolManager.set_sleep_time(FREQUENZ(this));

#ifdef BUILD_LINUX
	PeerPoolManager.set_sleep_function(&usleep_wrapper);
#else
	PeerPoolManager.set_sleep_function(&Kernel32::Sleep);
#endif;

	Thread::Create(TickThread, this);
	Thread::Create(AcceptorThread, this);

	SetRunning(true);
	NET_LOG_SUCCESS(CSTRING("'%s' => running on port %d"), SERVERNAME(this), SERVERPORT(this));
	return true;
}

bool Net::Server::Server::Close()
{
	if (!IsRunning())
	{
		NET_LOG_ERROR(CSTRING("'%s' => server is still running unable to close it"), SERVERNAME(this));
		return false;
	}

	if (hSyncClockNTP)
	{
		Timer::WaitSingleObjectStopped(hSyncClockNTP);
		hSyncClockNTP = nullptr;
	}

	if (hReSyncClockNTP)
	{
		Timer::WaitSingleObjectStopped(hReSyncClockNTP);
		hReSyncClockNTP = nullptr;
	}

	curTime = 0;

	SetRunning(false);

	if (GetListenSocket())
		Ws2_32::closesocket(GetListenSocket());

#ifndef BUILD_LINUX
	Ws2_32::WSACleanup();
#endif

	NET_LOG_DEBUG(CSTRING("'%s' => finished"), SERVERNAME(this));
	return true;
}

void Net::Server::Server::SingleSend(NET_PEER peer, const char* data, size_t size, bool& bPreviousSentFailed, const uint32_t sendToken)
{
	PEER_NOT_VALID(peer,
		return;
	);

	if (peer->bErase) return;
	if (bPreviousSentFailed)
		return;

	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		char* ptr = (char*)data;
		for (size_t it = 0; it < size; ++it)
			ptr[it] = ptr[it] ^ sendToken;
	}

	do
	{
		const auto res = Ws2_32::send(peer->pSocket, reinterpret_cast<const char*>(data), size, MSG_NOSIGNAL);
		if (res == SOCKET_ERROR)
		{
#ifdef BUILD_LINUX
			if (errno == EWOULDBLOCK)
			{
				usleep(FREQUENZ(this) * 1000);
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				ErasePeer(peer);
				if (ERRNO_ERROR_TRIGGERED) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(errno).c_str());
				return;
			}
#else
			if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Kernel32::Sleep(FREQUENZ(this));
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				ErasePeer(peer);
				if (Ws2_32::WSAGetLastError() != 0) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(Ws2_32::WSAGetLastError()).c_str());
				return;
			}
#endif
		}
		if (res < 0)
			break;

		size -= res;
	} while (size > 0);
}

void Net::Server::Server::SingleSend(NET_PEER peer, BYTE*& data, size_t size, bool& bPreviousSentFailed, const uint32_t sendToken)
{
	PEER_NOT_VALID(peer,
		FREE<byte>(data);
	return;
	);

	if (peer->bErase)
	{
		FREE<byte>(data);
		return;
	}

	if (bPreviousSentFailed)
	{
		FREE<byte>(data);
		return;
	}

	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		for (size_t it = 0; it < size; ++it)
			data[it] = data[it] ^ sendToken;
	}

	do
	{
		const auto res = Ws2_32::send(peer->pSocket, reinterpret_cast<const char*>(data), size, MSG_NOSIGNAL);
		if (res == SOCKET_ERROR)
		{
#ifdef BUILD_LINUX
			if (errno == EWOULDBLOCK)
			{
				usleep(FREQUENZ(this) * 1000);
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				FREE(data);
				ErasePeer(peer);
				if (ERRNO_ERROR_TRIGGERED) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(errno).c_str());
				return;
			}
#else
			if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Kernel32::Sleep(FREQUENZ(this));
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				FREE<byte>(data);
				ErasePeer(peer);
				if (Ws2_32::WSAGetLastError() != 0) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(Ws2_32::WSAGetLastError()).c_str());
				return;
			}
#endif
		}
		if (res < 0)
			break;

		size -= res;
	} while (size > 0);

	FREE<byte>(data);
}

void Net::Server::Server::SingleSend(NET_PEER peer, NET_CPOINTER<BYTE>& data, size_t size, bool& bPreviousSentFailed, const uint32_t sendToken)
{
	PEER_NOT_VALID(peer,
		data.free();
	return;
	);

	if (peer->bErase)
	{
		data.free();
		return;
	}

	if (bPreviousSentFailed)
	{
		data.free();
		return;
	}

	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		for (size_t it = 0; it < size; ++it)
			data.get()[it] = data.get()[it] ^ sendToken;
	}

	do
	{
		const auto res = Ws2_32::send(peer->pSocket, reinterpret_cast<const char*>(data.get()), size, MSG_NOSIGNAL);
		if (res == SOCKET_ERROR)
		{
#ifdef BUILD_LINUX
			if (errno == EWOULDBLOCK)
			{
				usleep(FREQUENZ(this) * 1000);
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				data.free();
				ErasePeer(peer);
				if (ERRNO_ERROR_TRIGGERED) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(errno).c_str());
				return;
			}
#else
			if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Kernel32::Sleep(FREQUENZ(this));
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				data.free();
				ErasePeer(peer);
				if (Ws2_32::WSAGetLastError() != 0) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(Ws2_32::WSAGetLastError()).c_str());
				return;
			}
#endif
		}
		if (res < 0)
			break;

		size -= res;
	} while (size > 0);

	data.free();
}

void Net::Server::Server::SingleSend(NET_PEER peer, Net::RawData_t& data, bool& bPreviousSentFailed, const uint32_t sendToken)
{
	if (!data.valid()) return;

	PEER_NOT_VALID(peer,
		data.free();
	return;
	);

	if (peer->bErase)
	{
		data.free();
		return;
	}

	if (bPreviousSentFailed)
	{
		data.free();
		return;
	}

	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		for (size_t it = 0; it < data.size(); ++it)
			data.value()[it] = data.value()[it] ^ sendToken;
	}

	size_t size = data.size();
	do
	{
		const auto res = Ws2_32::send(peer->pSocket, reinterpret_cast<const char*>(data.value()), size, MSG_NOSIGNAL);
		if (res == SOCKET_ERROR)
		{
#ifdef BUILD_LINUX
			if (errno == EWOULDBLOCK)
			{
				usleep(FREQUENZ(this) * 1000);
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				data.free();
				ErasePeer(peer);
				if (ERRNO_ERROR_TRIGGERED) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(errno).c_str());
				return;
			}
#else
			if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Kernel32::Sleep(FREQUENZ(this));
				continue;
			}
			else
			{
				bPreviousSentFailed = true;
				data.free();
				ErasePeer(peer);
				if (Ws2_32::WSAGetLastError() != 0) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(Ws2_32::WSAGetLastError()).c_str());
				return;
			}
#endif
		}
		if (res < 0)
			break;

		size -= res;
	} while (size > 0);

	data.free();
}

/*
*							Visualisation of packet structure in NET
*	---------------------------------------------------------------------------------------------------------------------------------
*				CRYPTED VERSION					|		NON-CRYPTED VERSION
*	---------------------------------------------------------------------------------------------------------------------------------
*	{KEYWORD}{SIZE}DATA
*	---------------------------------------------------------------------------------------------------------------------------------
*	{BP}								*		{BP}
*		{PS}{...}						*			{PS}{...}
*	---------------------------------------------------------------------------------------------------------------------------------
*									COMPRESSION
*		{POS}{...}						*			{POS}{...}
*	---------------------------------------------------------------------------------------------------------------------------------
*			{AK}{...}...				*						-
*			{AV}{...}...				*						-
*			{RDK}{...}...				*				{RDK}{...}...
*			{RD}{...}...				*				{RD}{...}...
*			{D}{...}...					*				{D}{...}...
*	{EP}								*		{EP}
*
*/
void Net::Server::Server::DoSend(NET_PEER peer, const int id, NET_PACKET& pkg)
{
	PEER_NOT_VALID(peer,
		return;
	);

	if (peer->bErase)
		return;

	std::lock_guard<std::mutex> guard(peer->network._mutex_send);

	uint32_t sendToken = INVALID_UINT_SIZE;
	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
		sendToken = Net::Coding::TOTP::generateToken(peer->totp_secret, peer->totp_secret_len, Isset(NET_OPT_USE_NTP) ? GetOption<bool>(NET_OPT_USE_NTP) : NET_OPT_DEFAULT_USE_NTP ? curTime : time(nullptr), Isset(NET_OPT_TOTP_INTERVAL) ? (int)(GetOption<int>(NET_OPT_TOTP_INTERVAL) / 2) : (int)(NET_OPT_DEFAULT_TOTP_INTERVAL / 2));

	Net::Json::Document doc;
	doc[CSTRING("ID")] = id;
	doc[CSTRING("CONTENT")] = pkg.Data();

	auto buffer = doc.Serialize(Net::Json::SerializeType::UNFORMATTED);

	auto dataBufferSize = buffer.size();
	NET_CPOINTER<BYTE> dataBuffer(ALLOC<BYTE>(dataBufferSize + 1));
	memcpy(dataBuffer.get(), buffer.get().get(), dataBufferSize);
	dataBuffer.get()[dataBufferSize] = '\0';
	buffer.clear();

	size_t combinedSize = 0;

	/* Crypt */
	if ((Isset(NET_OPT_USE_CIPHER) ? GetOption<bool>(NET_OPT_USE_CIPHER) : NET_OPT_DEFAULT_USE_CIPHER) && peer->cryption.getHandshakeStatus())
	{
		NET_AES aes;

		/* Generate new AES Keypair */
		size_t aesKeySize = Isset(NET_OPT_CIPHER_AES_SIZE) ? GetOption<size_t>(NET_OPT_CIPHER_AES_SIZE) : NET_OPT_DEFAULT_AES_SIZE;
		NET_CPOINTER<BYTE> Key(ALLOC<BYTE>(aesKeySize + 1));
		Random::GetRandStringNew(Key.reference().get(), aesKeySize);
		Key.get()[aesKeySize] = '\0';

		NET_CPOINTER<BYTE> IV(ALLOC<BYTE>(CryptoPP::AES::BLOCKSIZE + 1));
		Random::GetRandStringNew(IV.reference().get(), CryptoPP::AES::BLOCKSIZE);
		IV.get()[CryptoPP::AES::BLOCKSIZE] = '\0';

		if (!aes.init(reinterpret_cast<const char*>(Key.get()), reinterpret_cast<const char*>(IV.get())))
		{
			Key.free();
			IV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_InitAES, true);
			return;
		}

		/* Encrypt AES Keypair using RSA */
		if (!peer->cryption.RSA.encryptBase64(Key.reference().get(), aesKeySize))
		{
			Key.free();
			IV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_CryptKeyBase64, true);
			return;
		}

		size_t IVSize = CryptoPP::AES::BLOCKSIZE;
		if (!peer->cryption.RSA.encryptBase64(IV.reference().get(), IVSize))
		{
			Key.free();
			IV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_CryptIVBase64, true);
			return;
		}

		/* Compression */
		size_t original_dataBufferSize = dataBufferSize;
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			/* Compress Data */
			CompressData(dataBuffer.reference().get(), dataBufferSize);

			/* Compress Raw Data */
			if (PKG.HasRawData())
			{
				for (auto& entry : PKG.GetRawData())
				{
					entry.set_original_size(entry.size());
					CompressData(entry.value(), entry.size());
				}
			}
		}

		/* Crypt Buffer using AES and Encode to Base64 */
		aes.encrypt(dataBuffer.get(), dataBufferSize);

		if (PKG.HasRawData())
		{
			std::vector<Net::RawData_t>& rawData = PKG.GetRawData();
			for (auto& data : rawData)
				aes.encrypt(data.value(), data.size());
		}

		combinedSize = dataBufferSize + NET_PACKET_HEADER_LEN + NET_PACKET_SIZE_LEN + NET_DATA_LEN + NET_PACKET_FOOTER_LEN + NET_AES_KEY_LEN + NET_AES_IV_LEN + aesKeySize + IVSize + 8;

		/* Compression */
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			combinedSize += NET_PACKET_ORIGINAL_SIZE_LEN;
			combinedSize += 2; // begin & end tag

			std::string original_dataBufferSize_str = std::to_string(original_dataBufferSize);
			combinedSize += original_dataBufferSize_str.length();
		}

		// Append Raw data packet size
		if (PKG.HasRawData()) combinedSize += PKG.GetRawDataFullSize(Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION);

		std::string dataSizeStr = std::to_string(dataBufferSize);
		combinedSize += dataSizeStr.length();

		const auto KeySizeStr = std::to_string(aesKeySize);
		combinedSize += KeySizeStr.length();

		const auto IVSizeStr = std::to_string(IVSize);
		combinedSize += IVSizeStr.length();

		const auto EntirePacketSizeStr = std::to_string(combinedSize + std::to_string(combinedSize).length());

		auto bPreviousSentFailed = false;

		/* Append Packet Header */
		SingleSend(peer, NET_PACKET_HEADER, NET_PACKET_HEADER_LEN, bPreviousSentFailed, sendToken);

		// Append Packet Size Syntax
		SingleSend(peer, NET_PACKET_SIZE, NET_PACKET_SIZE_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, EntirePacketSizeStr.data(), EntirePacketSizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);

		/* Append Original Uncompressed Packet Size */
		/* Compression */
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			const auto UnCompressedPacketSizeStr = std::to_string(original_dataBufferSize + std::to_string(original_dataBufferSize).length());

			SingleSend(peer, NET_PACKET_ORIGINAL_SIZE, NET_PACKET_ORIGINAL_SIZE_LEN, bPreviousSentFailed, sendToken);
			SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
			SingleSend(peer, UnCompressedPacketSizeStr.data(), UnCompressedPacketSizeStr.length(), bPreviousSentFailed, sendToken);
			SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		}

		/* Append Packet Key */
		SingleSend(peer, NET_AES_KEY, NET_AES_KEY_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, KeySizeStr.data(), KeySizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, Key, aesKeySize, bPreviousSentFailed, sendToken);

		/* Append Packet IV */
		SingleSend(peer, NET_AES_IV, NET_AES_IV_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, IVSizeStr.data(), IVSizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, IV, IVSize, bPreviousSentFailed, sendToken);

		/* Append Packet Data */
		if (PKG.HasRawData())
		{
			for (auto& data : PKG.GetRawData())
			{
				// Append Key
				SingleSend(peer, NET_RAW_DATA_KEY, NET_RAW_DATA_KEY_LEN, bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);

				const auto KeyLengthStr = std::to_string(strlen(data.key()) + 1);

				SingleSend(peer, KeyLengthStr.data(), KeyLengthStr.length(), bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				SingleSend(peer, data.key(), strlen(data.key()) + 1, bPreviousSentFailed, sendToken);

				// Append Original Size
				/* Compression */
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					const auto OriginalSizeStr = std::to_string(data.original_size() + std::to_string(data.original_size()).length());

					SingleSend(peer, NET_RAW_DATA_ORIGINAL_SIZE, NET_RAW_DATA_ORIGINAL_SIZE_LEN, bPreviousSentFailed, sendToken);
					SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
					SingleSend(peer, OriginalSizeStr.data(), OriginalSizeStr.length(), bPreviousSentFailed, sendToken);
					SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				}

				// Append Raw Data
				SingleSend(peer, NET_RAW_DATA, NET_RAW_DATA_LEN, bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);

				const auto rawDataLengthStr = std::to_string(data.size());

				SingleSend(peer, rawDataLengthStr.data(), rawDataLengthStr.length(), bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				SingleSend(peer, data, bPreviousSentFailed, sendToken);

				data.set_free(false);
			}
		}

		SingleSend(peer, NET_DATA, NET_DATA_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, dataSizeStr.data(), dataSizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, dataBuffer, dataBufferSize, bPreviousSentFailed, sendToken);

		/* Append Packet Footer */
		SingleSend(peer, NET_PACKET_FOOTER, NET_PACKET_FOOTER_LEN, bPreviousSentFailed, sendToken);
	}
	else
	{
		/* Compression */
		size_t original_dataBufferSize = dataBufferSize;
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			/* Compress Data */
			CompressData(dataBuffer.reference().get(), dataBufferSize);

			/* Compress Raw Data */
			if (PKG.HasRawData())
			{
				for (auto& entry : PKG.GetRawData())
				{
					entry.set_original_size(entry.size());
					CompressData(entry.value(), entry.size());
				}
			}
		}

		combinedSize = dataBufferSize + NET_PACKET_HEADER_LEN + NET_PACKET_SIZE_LEN + NET_DATA_LEN + NET_PACKET_FOOTER_LEN + 4;

		/* Compression */
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			combinedSize += NET_PACKET_ORIGINAL_SIZE_LEN;
			combinedSize += 2; // begin & end tag

			std::string original_dataBufferSize_str = std::to_string(original_dataBufferSize);
			combinedSize += original_dataBufferSize_str.length();
		}

		// Append Raw data packet size
		if (PKG.HasRawData()) combinedSize += PKG.GetRawDataFullSize(Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION);

		std::string dataSizeStr = std::to_string(dataBufferSize);
		combinedSize += dataSizeStr.length();

		const auto EntirePacketSizeStr = std::to_string(combinedSize + std::to_string(combinedSize).length());

		auto bPreviousSentFailed = false;

		/* Append Packet Header */
		SingleSend(peer, NET_PACKET_HEADER, NET_PACKET_HEADER_LEN, bPreviousSentFailed, sendToken);

		// Append Packet Size Syntax
		SingleSend(peer, NET_PACKET_SIZE, NET_PACKET_SIZE_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, EntirePacketSizeStr.data(), EntirePacketSizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);

		/* Append Original Uncompressed Packet Size */
		/* Compression */
		if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
		{
			const auto UnCompressedPacketSizeStr = std::to_string(original_dataBufferSize + std::to_string(original_dataBufferSize).length());

			SingleSend(peer, NET_PACKET_ORIGINAL_SIZE, NET_PACKET_ORIGINAL_SIZE_LEN, bPreviousSentFailed, sendToken);
			SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
			SingleSend(peer, UnCompressedPacketSizeStr.data(), UnCompressedPacketSizeStr.length(), bPreviousSentFailed, sendToken);
			SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		}

		/* Append Packet Data */
		if (PKG.HasRawData())
		{
			for (auto& data : PKG.GetRawData())
			{
				// Append Key
				SingleSend(peer, NET_RAW_DATA_KEY, NET_RAW_DATA_KEY_LEN, bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);

				const auto KeyLengthStr = std::to_string(strlen(data.key()) + 1);

				SingleSend(peer, KeyLengthStr.data(), KeyLengthStr.length(), bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				SingleSend(peer, data.key(), strlen(data.key()) + 1, bPreviousSentFailed, sendToken);

				// Append Original Size
				/* Compression */
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					const auto OriginalSizeStr = std::to_string(data.original_size() + std::to_string(data.original_size()).length());

					SingleSend(peer, NET_RAW_DATA_ORIGINAL_SIZE, NET_RAW_DATA_ORIGINAL_SIZE_LEN, bPreviousSentFailed, sendToken);
					SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);
					SingleSend(peer, OriginalSizeStr.data(), OriginalSizeStr.length(), bPreviousSentFailed, sendToken);
					SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				}

				// Append Raw Data
				SingleSend(peer, NET_RAW_DATA, NET_RAW_DATA_LEN, bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_OPEN, 1, bPreviousSentFailed, sendToken);

				const auto rawDataLengthStr = std::to_string(data.size());

				SingleSend(peer, rawDataLengthStr.data(), rawDataLengthStr.length(), bPreviousSentFailed, sendToken);
				SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
				SingleSend(peer, data, bPreviousSentFailed, sendToken);

				data.set_free(false);
			}
		}

		SingleSend(peer, NET_DATA, NET_DATA_LEN, bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_OPEN, strlen(NET_PACKET_BRACKET_OPEN), bPreviousSentFailed, sendToken);
		SingleSend(peer, dataSizeStr.data(), dataSizeStr.length(), bPreviousSentFailed, sendToken);
		SingleSend(peer, NET_PACKET_BRACKET_CLOSE, 1, bPreviousSentFailed, sendToken);
		SingleSend(peer, dataBuffer, dataBufferSize, bPreviousSentFailed, sendToken);

		/* Append Packet Footer */
		SingleSend(peer, NET_PACKET_FOOTER, NET_PACKET_FOOTER_LEN, bPreviousSentFailed, sendToken);
	}
}

struct Receive_t
{
	Net::Server::Server* server;
	NET_PEER peer;
};

Net::PeerPool::WorkStatus_t PeerWorker(void* pdata)
{
	const auto data = (Receive_t*)pdata;
	if (!data) return Net::PeerPool::WorkStatus_t::STOP;

	auto peer = data->peer;
	const auto server = data->server;

	if (!server->IsRunning()) return Net::PeerPool::WorkStatus_t::STOP;
	if (peer->bErase) return Net::PeerPool::WorkStatus_t::STOP;
	if (peer->pSocket == INVALID_SOCKET) return Net::PeerPool::WorkStatus_t::STOP;

	server->OnPeerUpdate(peer);

	return (!server->DoReceive(peer) ? Net::PeerPool::WorkStatus_t::FORWARD : Net::PeerPool::WorkStatus_t::CONTINUE);
}

void OnPeerDelete(void* pdata)
{
	const auto data = (Receive_t*)pdata;
	if (!data) return;

	auto peer = data->peer;
	const auto server = data->server;

	// erase him
	server->ErasePeer(peer, true);
}

NET_TIMER(TimerPeerCheckAwaitNetProtocol)
{
	const auto data = (Receive_t*)param;
	if (!data) return 0;

	auto peer = data->peer;
	const auto server = data->server;

	if (peer->hWaitForNetProtocol == nullptr) 	NET_STOP_TIMER;

	NET_LOG_PEER(CSTRING("'%s' :: [%s] => might not be using proper protocol"), SERVERNAME(server), peer->IPAddr().get());

	server->ErasePeer(peer);

	NET_STOP_TIMER;
}

NET_THREAD(PeerStartRoutine)
{
	const auto data = (Receive_t*)parameter;
	if (!data) return 0;

	auto peer = data->peer;
	const auto server = data->server;

	/*
		rsa -> version -> all other
	*/
	if (server->Isset(NET_OPT_USE_CIPHER) ? server->GetOption<bool>(NET_OPT_USE_CIPHER) : NET_OPT_DEFAULT_USE_CIPHER)
	{
		peer->cryption.createKeyPair(server->Isset(NET_OPT_CIPHER_RSA_SIZE) ? server->GetOption<size_t>(NET_OPT_CIPHER_RSA_SIZE) : NET_OPT_DEFAULT_RSA_SIZE);

		const auto PublicKey = peer->cryption.RSA.publicKey();

		size_t b64len = PublicKey.size();
		BYTE* b64 = ALLOC<BYTE>(b64len + 1);
		memcpy(b64, PublicKey.data(), b64len);
		b64[b64len] = 0;

		Net::Coding::Base64::encode(b64, b64len);

		NET_PACKET PKG;
		PKG[CSTRING("PublicKey")] = reinterpret_cast<char*>(b64);
		server->NET_SEND(peer, NET_NATIVE_PACKET_ID::PKG_RSAHandshake, pkg);

		//FREE<byte>(b64);
	}
	/*
		version -> all other
	*/
	else
	{
		// keep it empty, we get it filled back
		NET_PACKET PKG;
		server->NET_SEND(peer, NET_NATIVE_PACKET_ID::PKG_Version, pkg);
	}

	peer->hWaitForNetProtocol = Net::Timer::Create(TimerPeerCheckAwaitNetProtocol, server->Isset(NET_OPT_NET_PROTOCOL_CHECK_TIME) ? server->GetOption<double>(NET_OPT_NET_PROTOCOL_CHECK_TIME) : NET_OPT_DEFAULT_NET_PROTOCOL_CHECK_TIME, parameter);

	/* add peer to peer thread pool */
	Net::PeerPool::peerInfo_t pInfo;
	pInfo.SetPeer(parameter);
	pInfo.SetWorker(&PeerWorker);
	pInfo.SetCallbackOnDelete(&OnPeerDelete);
	server->add_to_peer_threadpool(pInfo);

	return 0;
}

void Net::Server::Server::Acceptor()
{
	/* This function manages all the incomming connection */

	// if client waiting, accept the connection and save the socket
	auto client_addr = sockaddr_in();
	socklen_t slen = sizeof(client_addr);

	SOCKET accept_socket = INVALID_SOCKET;
	do
	{
		accept_socket = Ws2_32::accept(GetListenSocket(), (sockaddr*)&client_addr, &slen);
		if (accept_socket == INVALID_SOCKET)
		{
#ifdef BUILD_LINUX
			if (errno == EWOULDBLOCK)
#else
			if (Ws2_32::WSAGetLastError() == WSAEWOULDBLOCK)
#endif
			{
#ifdef BUILD_LINUX
				usleep(FREQUENZ(this) * 1000);
#else
				Kernel32::Sleep(FREQUENZ(this));
#endif
			}
			else
			{
				NET_LOG_ERROR(CSTRING("'%s' => [accept] failed with error %d"), SERVERNAME(this), LAST_ERROR);
				return;
			}
		}
	} while (accept_socket == INVALID_SOCKET);

	const auto pdata = ALLOC<Receive_t>();
	pdata->server = this;
	pdata->peer = CreatePeer(client_addr, accept_socket);
	if (pdata->peer) Net::Thread::Create(PeerStartRoutine, pdata);
}

/*
*							Visualisation of packet structure in NET
*	---------------------------------------------------------------------------------------------------------------------------------
*				CRYPTED VERSION					|		NON-CRYPTED VERSION
*	---------------------------------------------------------------------------------------------------------------------------------
*	{KEYWORD}{SIZE}DATA
*	---------------------------------------------------------------------------------------------------------------------------------
*	{BP}								*		{BP}
*		{PS}{...}						*			{PS}{...}
*	---------------------------------------------------------------------------------------------------------------------------------
*									COMPRESSION
*		{POS}{...}						*			{POS}{...}
*	---------------------------------------------------------------------------------------------------------------------------------
*			{AK}{...}...				*						-
*			{AV}{...}...				*						-
*			{RDK}{...}...				*				{RDK}{...}...
*			{RD}{...}...				*				{RD}{...}...
*			{D}{...}...					*				{D}{...}...
*	{EP}								*		{EP}
*
*/
bool Net::Server::Server::DoReceive(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return true;
	);

	SOCKET_NOT_VALID(peer->pSocket)
		return true;

	auto data_size = Ws2_32::recv(peer->pSocket, reinterpret_cast<char*>(peer->network.getDataReceive()), NET_OPT_DEFAULT_MAX_PACKET_SIZE, 0);
	if (data_size == SOCKET_ERROR)
	{
#ifdef BUILD_LINUX
		if (errno != EWOULDBLOCK)
#else
		if (Ws2_32::WSAGetLastError() != WSAEWOULDBLOCK)
#endif
		{
			peer->network.reset();
			ErasePeer(peer);

#ifdef BUILD_LINUX
			if (ERRNO_ERROR_TRIGGERED) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(errno).c_str());
#else
			if (Ws2_32::WSAGetLastError() != 0) NET_LOG_PEER(CSTRING("'%s' :: [%s] => %s"), SERVERNAME(this), peer->IPAddr().get(), Net::sock_err::getString(Ws2_32::WSAGetLastError()).c_str());
#endif

			return true;
		}

		ProcessPackets(peer);
		peer->network.reset();
		return true;
	}

	// graceful disconnect
	if (data_size == 0)
	{
		peer->network.reset();
		ErasePeer(peer);
		NET_LOG_PEER(CSTRING("'%s' :: [%s] => connection gracefully closed"), SERVERNAME(this), peer->IPAddr().get());
		return true;
	}

	if (!peer->network.dataValid())
	{
		peer->network.allocData(data_size);
		memcpy(peer->network.getData(), peer->network.getDataReceive(), data_size);
	}
	else
	{
		if (peer->network.getDataFullSize() > 0
			&& peer->network.getDataSize() + data_size < peer->network.getDataFullSize())
		{
			memcpy(&peer->network.getData()[peer->network.getDataSize()], peer->network.getDataReceive(), data_size);
			peer->network.setDataSize(peer->network.getDataSize() + data_size);
		}
		else
		{
			/* store incomming */
			const auto newBuffer = ALLOC<BYTE>(peer->network.getDataSize() + data_size + 1);
			memcpy(newBuffer, peer->network.getData(), peer->network.getDataSize());
			memcpy(&newBuffer[peer->network.getDataSize()], peer->network.getDataReceive(), data_size);
			newBuffer[peer->network.getDataSize() + data_size] = '\0';
			peer->network.setDataSize(peer->network.getDataSize() + data_size);
			peer->network.setData(newBuffer); // pointer swap
		}
	}

	peer->network.reset();
	ProcessPackets(peer);
	return false;
}

bool Net::Server::Server::ValidHeader(NET_PEER peer, bool& use_old_token)
{
	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		// shift the first bytes to check if we are using the correct token - using old token
		for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
			peer->network.getData()[it] = peer->network.getData()[it] ^ peer->lastToken;

		if (memcmp(&peer->network.getData()[0], NET_PACKET_HEADER, NET_PACKET_HEADER_LEN) != 0)
		{
			// shift back
			for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
				peer->network.getData()[it] = peer->network.getData()[it] ^ peer->lastToken;

			// shift the first bytes to check if we are using the correct token - using cur token
			for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
				peer->network.getData()[it] = peer->network.getData()[it] ^ peer->curToken;

			if (memcmp(&peer->network.getData()[0], NET_PACKET_HEADER, NET_PACKET_HEADER_LEN) != 0)
			{
				// shift back
				for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
					peer->network.getData()[it] = peer->network.getData()[it] ^ peer->curToken;

				peer->lastToken = peer->curToken;
				peer->curToken = Net::Coding::TOTP::generateToken(peer->totp_secret, peer->totp_secret_len, Isset(NET_OPT_USE_NTP) ? GetOption<bool>(NET_OPT_USE_NTP) : NET_OPT_DEFAULT_USE_NTP ? curTime : time(nullptr), Isset(NET_OPT_TOTP_INTERVAL) ? (int)(GetOption<int>(NET_OPT_TOTP_INTERVAL) / 2) : (int)(NET_OPT_DEFAULT_TOTP_INTERVAL / 2));

				// shift the first bytes to check if we are using the correct token - using new token
				for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
					peer->network.getData()[it] = peer->network.getData()[it] ^ peer->curToken;

				// [PROTOCOL] - check header is actually valid
				if (memcmp(&peer->network.getData()[0], NET_PACKET_HEADER, NET_PACKET_HEADER_LEN) != 0)
				{
					peer->network.clear();
					DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_InvalidFrameHeader);
					return false;
				}

				// sift back using new token
				for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
					peer->network.getData()[it] = peer->network.getData()[it] ^ peer->curToken;

				use_old_token = false;
			}
			else
			{
				// sift back using cur token
				for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
					peer->network.getData()[it] = peer->network.getData()[it] ^ peer->curToken;

				use_old_token = false;
			}
		}
		else
		{
			// sift back using old token
			for (size_t it = 0; it < NET_PACKET_HEADER_LEN; ++it)
				peer->network.getData()[it] = peer->network.getData()[it] ^ peer->lastToken;
		}
	}
	else
	{
		// [PROTOCOL] - check header is actually valid
		if (memcmp(&peer->network.getData()[0], NET_PACKET_HEADER, NET_PACKET_HEADER_LEN) != 0)
		{
			peer->network.clear();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_InvalidFrameHeader);
			return false;
		}
	}

	return true;
}

void Net::Server::Server::ProcessPackets(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return;
	);

	if (!peer->network.getDataSize())
		return;

	if (peer->network.getDataSize() == INVALID_SIZE)
		return;

	if (peer->network.getDataSize() < NET_PACKET_HEADER_LEN) return;

	auto use_old_token = true;
	bool already_checked = false;

	// [PROTOCOL] - read data full size from header
	if (!peer->network.getDataFullSize() || peer->network.getDataFullSize() == INVALID_SIZE)
	{
		already_checked = true;
		if (!ValidHeader(peer, use_old_token)) return;

		const size_t start = NET_PACKET_HEADER_LEN + NET_PACKET_SIZE_LEN + 1;
		for (size_t i = start; i < peer->network.getDataSize(); ++i)
		{
			// shift the bytes
			if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
				peer->network.getData()[i] = peer->network.getData()[i] ^ (use_old_token ? peer->lastToken : peer->curToken);

			// iterate until we have found the end tag
			if (!memcmp(&peer->network.getData()[i], NET_PACKET_BRACKET_CLOSE, 1))
			{
				peer->network.SetDataOffset(i);
				const auto size = i - start;
				char* end = (char*)peer->network.getData()[start] + size;
				peer->network.setDataFullSize(strtoull((const char*)&peer->network.getData()[start], &end, 10));

				// awaiting more bytes
				if (peer->network.getDataFullSize() > peer->network.getDataSize())
				{
					// pre-allocate enough space
					const auto newBuffer = ALLOC<BYTE>(peer->network.getDataFullSize() + 1);
					memcpy(newBuffer, peer->network.getData(), peer->network.getDataSize());
					newBuffer[peer->network.getDataFullSize()] = '\0';
					peer->network.setData(newBuffer); // pointer swap

					// shift all the way back
					if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
					{
						for (size_t it = start; it < i + 1; ++it)
							peer->network.getData()[it] = peer->network.getData()[it] ^ (use_old_token ? peer->lastToken : peer->curToken);
					}

					return;
				}

				// shift all the way back
				if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
				{
					for (size_t it = start; it < i + 1; ++it)
						peer->network.getData()[it] = peer->network.getData()[it] ^ (use_old_token ? peer->lastToken : peer->curToken);
				}

				break;
			}
		}
	}

	/* Compression */
	if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
	{
		const size_t start = peer->network.getDataOffset() + NET_PACKET_ORIGINAL_SIZE_LEN + 2; // 2 - Begin & End Tag
		for (size_t i = start; i < peer->network.getDataSize(); ++i)
		{
			// iterate until we have found the end tag
			if (!memcmp(&peer->network.getData()[i], NET_PACKET_BRACKET_CLOSE, 1))
			{
				peer->network.SetDataOffset(i);
				const auto size = i - start;
				char* end = (char*)peer->network.getData()[start] + size;
				peer->network.SetUncompressedSize(strtoull((const char*)&peer->network.getData()[start], &end, 10));

				break;
			}
		}
	}

	// keep going until we have received the entire packet
	if (!peer->network.getDataFullSize() || peer->network.getDataFullSize() == INVALID_SIZE || peer->network.getDataSize() < peer->network.getDataFullSize()) return;

	if (!already_checked)
		if (!ValidHeader(peer, use_old_token)) return;

	// shift only as much as required
	if (Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP)
	{
		for (size_t it = 0; it < peer->network.getDataFullSize(); ++it)
			peer->network.getData()[it] = peer->network.getData()[it] ^ (use_old_token ? peer->lastToken : peer->curToken);
	}

	// [PROTOCOL] - check footer is actually valid
	if (memcmp(&peer->network.getData()[peer->network.getDataFullSize() - NET_PACKET_FOOTER_LEN], NET_PACKET_FOOTER, NET_PACKET_FOOTER_LEN) != 0)
	{
		peer->network.clear();
		DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_InvalidFrameFooter);
		return;
	}

	// Execute the packet
	ExecutePacket(peer);

	// re-alloc buffer
	const auto leftSize = static_cast<int>(peer->network.getDataSize() - peer->network.getDataFullSize()) > 0 ? peer->network.getDataSize() - peer->network.getDataFullSize() : INVALID_SIZE;
	if (leftSize != INVALID_SIZE
		&& leftSize > 0)
	{
		const auto leftBuffer = ALLOC<BYTE>(leftSize + 1);
		memcpy(leftBuffer, &peer->network.getData()[peer->network.getDataFullSize()], leftSize);
		leftBuffer[leftSize] = '\0';
		peer->network.clear();
		peer->network.setData(leftBuffer); // swap pointer
		peer->network.setDataSize(leftSize);
		return;
	}

	peer->network.clear();
}

struct TPacketExcecute
{
	Net::Packet* m_packet;
	Net::Server::Server* m_server;
	NET_PEER m_peer;
	int m_packetId;
};

NET_THREAD(ThreadPacketExecute)
{
	auto tpe = (TPacketExcecute*)parameter;
	if (!tpe)
	{
		return 1;
	}

	if (!tpe->m_server->CheckDataN(tpe->m_peer, tpe->m_packetId, *tpe->m_packet))
		if (!tpe->m_server->CheckData(tpe->m_peer, tpe->m_packetId, *tpe->m_packet))
			tpe->m_server->DisconnectPeer(tpe->m_peer, NET_ERROR_CODE::NET_ERR_UndefinedFrame);

	/* because we had to create a copy to work with this data in seperate thread, we also have to handle the deletion of this block */
	if (tpe->m_packet->HasRawData())
	{
		std::vector<Net::RawData_t>& rawData = tpe->m_packet->GetRawData();
		for (auto& data : rawData)
		{
			data.free();
		}
	}

	FREE<Net::Packet>(tpe->m_packet);
	FREE<TPacketExcecute>(tpe);
	return 0;
}

void Net::Server::Server::ExecutePacket(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return;
	);

	NET_CPOINTER<BYTE> data;
	NET_CPOINTER<Net::Packet> pPacket(ALLOC<Net::Packet>());
	if (!pPacket.valid())
	{
		DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DataInvalid);
		return;
	}

	/* Crypt */
	if ((Isset(NET_OPT_USE_CIPHER) ? GetOption<bool>(NET_OPT_USE_CIPHER) : NET_OPT_DEFAULT_USE_CIPHER) && peer->cryption.getHandshakeStatus())
	{
		auto offset = peer->network.getDataOffset() + 1;

		NET_CPOINTER<BYTE> AESKey;
		size_t AESKeySize;

		// look for key tag
		if (!memcmp(&peer->network.getData()[offset], NET_AES_KEY, NET_AES_KEY_LEN))
		{
			offset += NET_AES_KEY_LEN;

			// read size
			for (auto y = offset; y < peer->network.getDataSize(); ++y)
			{
				if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
				{
					const auto psize = y - offset - 1;
					NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
					memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
					dataSizeStr.get()[psize] = '\0';
					AESKeySize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
					dataSizeStr.free();

					offset += psize + 2;
					break;
				}
			}

			// read the data
			AESKey = ALLOC<BYTE>(AESKeySize + 1);
			memcpy(AESKey.get(), &peer->network.getData()[offset], AESKeySize);
			AESKey.get()[AESKeySize] = '\0';

			offset += AESKeySize;
		}

		NET_CPOINTER<BYTE> AESIV;
		size_t AESIVSize;

		// look for iv tag
		if (!memcmp(&peer->network.getData()[offset], NET_AES_IV, NET_AES_IV_LEN))
		{
			offset += NET_AES_IV_LEN;

			// read size
			for (auto y = offset; y < peer->network.getDataSize(); ++y)
			{
				if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
				{
					const auto psize = y - offset - 1;
					NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
					memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
					dataSizeStr.get()[psize] = '\0';
					AESIVSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
					dataSizeStr.free();

					offset += psize + 2;
					break;
				}
			}

			// read the data
			AESIV = ALLOC<BYTE>(AESIVSize + 1);
			memcpy(AESIV.get(), &peer->network.getData()[offset], AESIVSize);
			AESIV.get()[AESIVSize] = '\0';

			offset += AESIVSize;
		}

		if (!peer->cryption.RSA.decryptBase64(AESKey.reference().get(), AESKeySize))
		{
			AESKey.free();
			AESIV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DecryptKeyBase64);
			goto loc_packet_free;
			return;
		}

		if (!peer->cryption.RSA.decryptBase64(AESIV.reference().get(), AESIVSize))
		{
			AESKey.free();
			AESIV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DecryptIVBase64);
			goto loc_packet_free;
			return;
		}

		NET_AES aes;
		if (!aes.init(reinterpret_cast<const char*>(AESKey.get()), reinterpret_cast<const char*>(AESIV.get())))
		{
			AESKey.free();
			AESIV.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_InitAES);
			goto loc_packet_free;
			return;
		}

		AESKey.free();
		AESIV.free();

		do
		{
			// look for raw data tag
			if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA_KEY, NET_RAW_DATA_KEY_LEN))
			{
				offset += NET_RAW_DATA_KEY_LEN;

				// read size
				NET_CPOINTER<BYTE> key;
				size_t KeySize = 0;
				{
					for (auto y = offset; y < peer->network.getDataSize(); ++y)
					{
						if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
						{
							const auto psize = y - offset - 1;
							NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
							memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
							dataSizeStr.get()[psize] = '\0';
							KeySize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
							dataSizeStr.free();

							offset += psize + 2;
							break;
						}
					}

					// read the data
					key = ALLOC<BYTE>(KeySize + 1);
					memcpy(key.get(), &peer->network.getData()[offset], KeySize);
					key.get()[KeySize] = '\0';

					offset += KeySize;
				}

				// looking for raw data original size tag
				/* Compression */
				size_t originalSize = 0;
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA_ORIGINAL_SIZE, NET_RAW_DATA_ORIGINAL_SIZE_LEN))
					{
						offset += NET_RAW_DATA_ORIGINAL_SIZE_LEN;

						// read original size
						for (auto y = offset; y < peer->network.getDataSize(); ++y)
						{
							if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
							{
								const auto psize = y - offset - 1;
								NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
								memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
								dataSizeStr.get()[psize] = '\0';
								originalSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
								dataSizeStr.free();

								offset += psize + 2;
								break;
							}
						}
					}
				}

				if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA, NET_RAW_DATA_LEN))
				{
					offset += NET_RAW_DATA_LEN;

					// read size
					size_t packetSize = 0;
					{
						for (auto y = offset; y < peer->network.getDataSize(); ++y)
						{
							if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
							{
								const auto psize = y - offset - 1;
								NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
								memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
								dataSizeStr.get()[psize] = '\0';
								packetSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
								dataSizeStr.free();

								offset += psize + 2;
								break;
							}
						}
					}

					Net::RawData_t entry = { (char*)key.get(), &peer->network.getData()[offset], packetSize, false };

					/* decrypt aes */
					if (!aes.decrypt(entry.value(), entry.size()))
					{
						DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DecryptAES);
						goto loc_packet_free;
						return;
					}

					/* Compression */
					if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
					{
						BYTE* copy = ALLOC<BYTE>(entry.size());
						memcpy(copy, entry.value(), entry.size());
						entry.set(copy);

						entry.set_original_size(originalSize);
						DecompressData(entry.value(), entry.size(), entry.original_size());
						entry.set_original_size(entry.size());
					}

					/* in seperate thread we need to create a copy of this data-set */
					if (Isset(NET_OPT_EXECUTE_PACKET_ASYNC) ? GetOption<bool>(NET_OPT_EXECUTE_PACKET_ASYNC) : NET_OPT_DEFAULT_EXECUTE_PACKET_ASYNC)
					{
						BYTE* copy = ALLOC<BYTE>(entry.size());
						memcpy(copy, entry.value(), entry.size());
						entry.set(copy);
					}

					pPacket.get()->AddRaw(entry);
					key.free();

					offset += packetSize;
				}
			}

			// look for data tag
			if (!memcmp(&peer->network.getData()[offset], NET_DATA, NET_DATA_LEN))
			{
				offset += NET_DATA_LEN;

				// read size
				size_t packetSize = 0;
				{
					for (auto y = offset; y < peer->network.getDataSize(); ++y)
					{
						if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
						{
							const auto psize = y - offset - 1;
							NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
							memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
							dataSizeStr.get()[psize] = '\0';
							packetSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
							dataSizeStr.free();

							offset += psize + 2;
							break;
						}
					}
				}

				// read the data
				size_t dataSize = packetSize;
				data = ALLOC<BYTE>(dataSize + 1);
				memcpy(data.get(), &peer->network.getData()[offset], dataSize);
				data.get()[dataSize] = '\0';

				offset += packetSize;

				/* decrypt aes */
				if (!aes.decrypt(data.get(), dataSize))
				{
					data.free();
					DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DecryptAES);
					goto loc_packet_free;
					return;
				}

				/* Compression */
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					DecompressData(data.reference().get(), packetSize, peer->network.getUncompressedSize());
				}
			}

			// we have reached the end of reading
			if (offset + NET_PACKET_FOOTER_LEN >= peer->network.getDataFullSize())
				break;

		} while (true);
	}
	else
	{
		auto offset = peer->network.getDataOffset() + 1;

		do
		{
			// look for raw data tag
			if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA_KEY, NET_RAW_DATA_KEY_LEN))
			{
				offset += NET_RAW_DATA_KEY_LEN;

				// read size
				NET_CPOINTER<BYTE> key;
				size_t KeySize = 0;
				{
					for (auto y = offset; y < peer->network.getDataSize(); ++y)
					{
						if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
						{
							const auto psize = y - offset - 1;
							NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
							memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
							dataSizeStr.get()[psize] = '\0';
							KeySize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
							dataSizeStr.free();

							offset += psize + 2;
							break;
						}
					}

					// read the data
					key = ALLOC<BYTE>(KeySize + 1);
					memcpy(key.get(), &peer->network.getData()[offset], KeySize);
					key.get()[KeySize] = '\0';

					offset += KeySize;
				}

				// looking for raw data original size tag
				/* Compression */
				size_t originalSize = 0;
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA_ORIGINAL_SIZE, NET_RAW_DATA_ORIGINAL_SIZE_LEN))
					{
						offset += NET_RAW_DATA_ORIGINAL_SIZE_LEN;

						// read original size
						for (auto y = offset; y < peer->network.getDataSize(); ++y)
						{
							if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
							{
								const auto psize = y - offset - 1;
								NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
								memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
								dataSizeStr.get()[psize] = '\0';
								originalSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
								dataSizeStr.free();

								offset += psize + 2;
								break;
							}
						}
					}
				}

				if (!memcmp(&peer->network.getData()[offset], NET_RAW_DATA, NET_RAW_DATA_LEN))
				{
					offset += NET_RAW_DATA_LEN;

					// read size
					size_t packetSize = 0;
					{
						for (auto y = offset; y < peer->network.getDataSize(); ++y)
						{
							if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
							{
								const auto psize = y - offset - 1;
								NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
								memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
								dataSizeStr.get()[psize] = '\0';
								packetSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
								dataSizeStr.free();

								offset += psize + 2;
								break;
							}
						}
					}

					Net::RawData_t entry = { (char*)key.get(), &peer->network.getData()[offset], packetSize, false };

					/* Compression */
					if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
					{
						BYTE* copy = ALLOC<BYTE>(entry.size());
						memcpy(copy, entry.value(), entry.size());
						entry.set(copy);

						entry.set_original_size(originalSize);
						DecompressData(entry.value(), entry.size(), entry.original_size());
						entry.set_original_size(entry.size());
					}

					/* in seperate thread we need to create a copy of this data-set */
					if (Isset(NET_OPT_EXECUTE_PACKET_ASYNC) ? GetOption<bool>(NET_OPT_EXECUTE_PACKET_ASYNC) : NET_OPT_DEFAULT_EXECUTE_PACKET_ASYNC)
					{
						BYTE* copy = ALLOC<BYTE>(entry.size());
						memcpy(copy, entry.value(), entry.size());
						entry.set(copy);
					}

					pPacket.get()->AddRaw(entry);
					key.free();

					offset += packetSize;
				}
			}

			// look for data tag
			if (!memcmp(&peer->network.getData()[offset], NET_DATA, NET_DATA_LEN))
			{
				offset += NET_DATA_LEN;

				// read size
				size_t packetSize = 0;
				{
					for (auto y = offset; y < peer->network.getDataSize(); ++y)
					{
						if (!memcmp(&peer->network.getData()[y], NET_PACKET_BRACKET_CLOSE, 1))
						{
							const auto psize = y - offset - 1;
							NET_CPOINTER<BYTE> dataSizeStr(ALLOC<BYTE>(psize + 1));
							memcpy(dataSizeStr.get(), &peer->network.getData()[offset + 1], psize);
							dataSizeStr.get()[psize] = '\0';
							packetSize = strtoull(reinterpret_cast<const char*>(dataSizeStr.get()), nullptr, 10);
							dataSizeStr.free();

							offset += psize + 2;
							break;
						}
					}
				}

				// read the data
				data = ALLOC<BYTE>(packetSize + 1);
				memcpy(data.get(), &peer->network.getData()[offset], packetSize);
				data.get()[packetSize] = '\0';

				offset += packetSize;

				/* Compression */
				if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
				{
					DecompressData(data.reference().get(), packetSize, peer->network.getUncompressedSize());
				}
			}

			// we have reached the end of reading
			if (offset + NET_PACKET_FOOTER_LEN >= peer->network.getDataFullSize())
				break;

		} while (true);
	}

	if (!data.valid())
	{
		DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DataInvalid);
		goto loc_packet_free;
		return;
	}

	/*
	* parse json
	* get packet id from it
	* and json content
	*
	* pass the json content into pPacket object
	*/
	int packetId = -1;
	{
		Net::Json::Document doc;
		if (!doc.Deserialize(reinterpret_cast<char*>(data.get())))
		{
			data.free();
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_DataInvalid);
			goto loc_packet_free;
			return;
		}

		data.free();

		if (!(doc[CSTRING("ID")] && doc[CSTRING("ID")]->is_int()))
		{
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_NoMemberID);
			goto loc_packet_free;
			return;
		}

		packetId = doc[CSTRING("ID")]->as_int();
		if (packetId < 0)
		{
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_MemberIDInvalid);
			goto loc_packet_free;
			return;
		}

		if (!(doc[CSTRING("CONTENT")] && doc[CSTRING("CONTENT")]->is_object())
			&& !(doc[CSTRING("CONTENT")] && doc[CSTRING("CONTENT")]->is_array()))
		{
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_NoMemberContent);
			goto loc_packet_free;
			return;
		}

		if (doc[CSTRING("CONTENT")]->is_object())
		{
			pPacket.get()->Data().Set(doc[CSTRING("CONTENT")]->as_object());
		}
		else if (doc[CSTRING("CONTENT")]->is_array())
		{
			pPacket.get()->Data().Set(doc[CSTRING("CONTENT")]->as_array());
		}
	}

	/*
	* check for option async to execute the callback in a seperate thread
	*/
	if (Isset(NET_OPT_EXECUTE_PACKET_ASYNC) ? GetOption<bool>(NET_OPT_EXECUTE_PACKET_ASYNC) : NET_OPT_DEFAULT_EXECUTE_PACKET_ASYNC)
	{
		TPacketExcecute* tpe = ALLOC<TPacketExcecute>();
		tpe->m_packet = pPacket.get();
		tpe->m_server = this;
		tpe->m_peer = peer;
		tpe->m_packetId = packetId;
		if (Net::Thread::Create(ThreadPacketExecute, tpe))
		{
			return;
		}

		FREE<TPacketExcecute>(tpe);
	}

	/*
	* execute in current thread
	*/
	if (!CheckDataN(peer, packetId, *pPacket.ref().get()))
		if (!CheckData(peer, packetId, *pPacket.ref().get()))
			DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_UndefinedFrame);

loc_packet_free:
	// if we use compression mode here, then we had to take a copy of the buffer to process the algo to decompress the block, now we have to handle the deletion of this block
	/* Compression */
	if (Isset(NET_OPT_USE_COMPRESSION) ? GetOption<bool>(NET_OPT_USE_COMPRESSION) : NET_OPT_DEFAULT_USE_COMPRESSION)
	{
		if (pPacket.get()->HasRawData())
		{
			std::vector<Net::RawData_t>& rawData = pPacket.get()->GetRawData();
			for (auto& data : rawData)
			{
				data.free();
			}
		}
	}

	pPacket.free();
}

void Net::Server::Server::CompressData(BYTE*& data, size_t& size)
{
#ifdef DEBUG
	const auto PrevSize = size;
#endif

	BYTE* m_pCompressed = 0;
	size_t m_iCompressedLen = 0;
	NET_ZLIB::Compress(data, size, m_pCompressed, m_iCompressedLen);
	FREE<BYTE>(data);
	data = m_pCompressed;
	size = m_iCompressedLen;

#ifdef DEBUG
	NET_LOG_DEBUG(CSTRING("'%s' => compressed data from size %llu to %llu"), SERVERNAME(this), PrevSize, size);
#endif
}

void Net::Server::Server::CompressData(BYTE*& data, BYTE*& out, size_t& size, const bool skip_free)
{
#ifdef DEBUG
	const auto PrevSize = size;
#endif

	BYTE* m_pCompressed = 0;
	size_t m_iCompressedLen = 0;
	NET_ZLIB::Compress(data, size, m_pCompressed, m_iCompressedLen);

	if (!skip_free)
	{
		FREE<BYTE>(data);
	}

	out = m_pCompressed;
	size = m_iCompressedLen;

#ifdef DEBUG
	NET_LOG_DEBUG(CSTRING("'%s' => compressed data from size %llu to %llu"), SERVERNAME(this), PrevSize, size);
#endif
}

void Net::Server::Server::DecompressData(BYTE*& data, size_t& size, size_t original_size)
{
#ifdef DEBUG
	const auto PrevSize = size;
#endif

	BYTE* m_pUnCompressed = 0;
	size_t m_iUncompressedLen = original_size;
	NET_ZLIB::Decompress(data, size, m_pUnCompressed, m_iUncompressedLen);
	FREE<BYTE>(data);
	data = m_pUnCompressed;
	size = m_iUncompressedLen;

#ifdef DEBUG
	NET_LOG_DEBUG(CSTRING("'%s' => decompressed data from size %llu to %llu"), SERVERNAME(this), PrevSize, size);
#endif
}

void Net::Server::Server::DecompressData(BYTE*& data, BYTE*& out, size_t& size, size_t original_size, const bool skip_free)
{
#ifdef DEBUG
	const auto PrevSize = size;
#endif

	BYTE* m_pUnCompressed = 0;
	size_t m_iUncompressedLen = original_size;
	NET_ZLIB::Decompress(data, size, m_pUnCompressed, m_iUncompressedLen);

	if (!skip_free)
	{
		FREE<BYTE>(data);
	}

	out = m_pUnCompressed;
	size = m_iUncompressedLen;

#ifdef DEBUG
	NET_LOG_DEBUG(CSTRING("'%s' => decompressed data from size %llu to %llu"), SERVERNAME(this), PrevSize, size);
#endif
}

NET_NATIVE_PACKET_DEFINITION_BEGIN(Net::Server::Server)
NET_DEFINE_PACKET(RSAHandshake, NET_NATIVE_PACKET_ID::PKG_RSAHandshake)
NET_DEFINE_PACKET(Version, NET_NATIVE_PACKET_ID::PKG_Version)
NET_PACKET_DEFINITION_END

NET_BEGIN_PACKET(Net::Server::Server, RSAHandshake)
if (!(Isset(NET_OPT_USE_CIPHER) ? GetOption<bool>(NET_OPT_USE_CIPHER) : NET_OPT_DEFAULT_USE_CIPHER))
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Handshake);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received rsa handshake frame altough cipher option is disabled"), SERVERNAME(this), peer->IPAddr().get());
	return;
}
if (peer->estabilished)
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Handshake);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received rsa handshake frame altough estabilished"), SERVERNAME(this), peer->IPAddr().get());
	return;
}
if (peer->cryption.getHandshakeStatus())
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Handshake);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received another rsa handshake frame"), SERVERNAME(this), peer->IPAddr().get());
	return;
}

if (!(PKG[CSTRING("PublicKey")] && PKG[CSTRING("PublicKey")]->is_string())) // empty
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Handshake);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received an invalid rsa handshake frame"), SERVERNAME(this), peer->IPAddr().get());
	return;
}

// from now we use the Cryption, synced with Server
{
	size_t b64len = strlen(pkg[CSTRING("PublicKey")]->as_string());
	BYTE* b64 = ALLOC<BYTE>(b64len + 1);
	memcpy(b64, pkg[CSTRING("PublicKey")]->as_string(), b64len);
	b64[b64len] = 0;

	Net::Coding::Base64::decode(b64, b64len);

	peer->cryption.RSA.setPublicKey(reinterpret_cast<char*>(b64));
	peer->cryption.setHandshakeStatus(true);
}

// RSA Handshake has been finished, keep going with normal process
NET_LOG_PEER(CSTRING("'%s' :: [%s] => succeeded rsa handshake"), SERVERNAME(this), peer->IPAddr().get());

// keep it empty, we get it filled back
Packet Version;
NET_SEND(peer, NET_NATIVE_PACKET_ID::PKG_Version, Version);
NET_END_PACKET

NET_BEGIN_PACKET(Net::Server::Server, Version)
if ((Isset(NET_OPT_USE_CIPHER) ? GetOption<bool>(NET_OPT_USE_CIPHER) : NET_OPT_DEFAULT_USE_CIPHER) && !peer->cryption.getHandshakeStatus())
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Version);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => have not received a rsa handshake yet"), SERVERNAME(this), peer->IPAddr().get());
	return;
}
if (peer->estabilished)
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Version);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received another version frame"), SERVERNAME(this), peer->IPAddr().get());
	return;
}

if (!(PKG[CSTRING("MajorVersion")] && PKG[CSTRING("MajorVersion")]->is_int())
	|| !(PKG[CSTRING("MinorVersion")] && PKG[CSTRING("MinorVersion")]->is_int())
	|| !(PKG[CSTRING("Revision")] && PKG[CSTRING("Revision")]->is_int())
	|| !(PKG[CSTRING("Key")] && PKG[CSTRING("Key")]->is_string()))
{
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Versionmismatch);
	NET_LOG_ERROR(CSTRING("'%s' :: [%s] => received an invalid version frame"), SERVERNAME(this), peer->IPAddr().get());
	return;
}

const auto majorVersion = PKG[CSTRING("MajorVersion")]->as_int();
const auto minorVersion = PKG[CSTRING("MinorVersion")]->as_int();
const auto revision = PKG[CSTRING("Revision")]->as_int();
const auto Key = PKG[CSTRING("Key")]->as_string();

if ((majorVersion == Version::Major())
	&& (minorVersion == Version::Minor())
	&& (revision == Version::Revision())
	&& strcmp(Key, Version::Key().data().data()) == 0)
{
	peer->NetVersionMatched = true;

	Packet estabilish;
	NET_SEND(peer, NET_NATIVE_PACKET_ID::PKG_Estabilish, estabilish);

	peer->estabilished = true;

	NET_LOG_PEER(CSTRING("'%s' :: [%s] => estabilished"), SERVERNAME(this), peer->IPAddr().get());

	Net::Timer::WaitSingleObjectStopped(peer->hWaitForNetProtocol);
	peer->hWaitForNetProtocol = nullptr;

	// callback
	OnPeerEstabilished(peer);
}
else
{
	NET_LOG_PEER(CSTRING("'%s' :: [%s] => sent different values in version frame:\n(%i.%i.%i-%s)"), SERVERNAME(this), peer->IPAddr().get(), majorVersion, minorVersion, revision, Key);

	// version or key missmatch, disconnect peer
	DisconnectPeer(peer, NET_ERROR_CODE::NET_ERR_Versionmismatch);
}
NET_END_PACKET

void Net::Server::Server::add_to_peer_threadpool(Net::PeerPool::peerInfo_t info)
{
	PeerPoolManager.add(info);
}

void Net::Server::Server::add_to_peer_threadpool(Net::PeerPool::peerInfo_t * pinfo)
{
	PeerPoolManager.add(pinfo);
}

size_t Net::Server::Server::count_peers_all()
{
	return PeerPoolManager.count_peers_all();
}

size_t Net::Server::Server::count_peers(Net::PeerPool::peer_threadpool_t * pool)
{
	return PeerPoolManager.count_peers(pool);
}

size_t Net::Server::Server::count_pools()
{
	return PeerPoolManager.count_pools();
}

bool Net::Server::Server::CreateTOTPSecret(NET_PEER peer)
{
	PEER_NOT_VALID(peer,
		return false;
	);

	if (!(Isset(NET_OPT_USE_TOTP) ? GetOption<bool>(NET_OPT_USE_TOTP) : NET_OPT_DEFAULT_USE_TOTP))
		return false;

#ifdef BUILD_LINUX
	struct tm* tm = nullptr;
	tm = gmtime(&curTime);
	tm->tm_hour = Net::Util::roundUp(tm->tm_hour, 10);
	tm->tm_min = Net::Util::roundUp(tm->tm_min, 10);
	tm->tm_sec = 0;
	const auto txTm = mktime(tm);
#else
	tm tm;
	gmtime_s(&tm, &curTime);
	tm.tm_hour = Net::Util::roundUp(tm.tm_hour, 10);
	tm.tm_min = Net::Util::roundUp(tm.tm_min, 10);
	tm.tm_sec = 0;
	const auto txTm = mktime(&tm);
#endif

	const auto strTime = ctime(&txTm);
	peer->totp_secret_len = strlen(strTime);

	FREE<byte>(peer->totp_secret);
	peer->totp_secret = ALLOC<byte>(peer->totp_secret_len + 1);
	memcpy(peer->totp_secret, strTime, peer->totp_secret_len);
	peer->totp_secret[peer->totp_secret_len] = '\0';
	Net::Coding::Base32::encode(peer->totp_secret, peer->totp_secret_len);

	peer->curToken = 0;
	peer->lastToken = 0;

	return true;
}
