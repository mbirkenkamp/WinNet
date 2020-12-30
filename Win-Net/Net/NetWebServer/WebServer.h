#pragma once
#define NET_WEB_SERVER Net::WebServer::Server

#define NET_IPEER peerInfo
#define NET_PEER peerInfo*

#define PEER peer
#define PKG pkg
#define FUNCTION_NAME NET_FUNCTIONNAME
#define NET_BEGIN_FUNC_PACKAGE(cs, fnc) void cs::On##fnc(NET_PEER PEER, NET_PACKAGE PKG) { \
	const char* NET_FUNCTIONNAME = CASTRING("On"#fnc);

#define NET_END_FUNC_PACKAGE }
#define NET_BEGIN_FNC_PKG NET_BEGIN_FUNC_PACKAGE
#define NET_END_FNC_PKG NET_END_FUNC_PACKAGE
#define NET_DEF_FUNC_PACKAGE(fnc) void On##fnc(NET_PEER, NET_PACKAGE)
#define NET_DEF_FNC_PKG NET_DEF_FUNC_PACKAGE

#define NET_SEND DoSend

#define NET_PEER_WAIT_LOCK(peer) while (peer && peer->bQueueLock) {};

#include <Net/Net/Net.h>
#include <Net/Net/Package.h>
#include <Net/Net/NetCodes.h>

/* Websocket frame protocol operationcodes */
CONSTEXPR auto OPCODE_CONTINUE = 0x0;
CONSTEXPR auto OPCODE_TEXT = 0x1;
CONSTEXPR auto OPCODE_BINARY = 0x2;
CONSTEXPR auto OPCODE_CLOSE = 0x8;
CONSTEXPR auto OPCODE_PING = 0x9;
CONSTEXPR auto OPCODE_PONG = 0xA;

CONSTEXPR auto WS_CONTROL_PACKAGE = -1; // used to send a ping or pong frame
CONSTEXPR auto WS_FIN = 0x80;
CONSTEXPR auto WS_OPCODE = 0xF;
CONSTEXPR auto WS_MASK = 0x7F;
CONSTEXPR auto WS_PAYLOADLENGTH = 0x7F;
CONSTEXPR auto WS_CONTROLFRAME = 8;
CONSTEXPR auto WS_PAYLOAD_LENGTH_16 = 126;
CONSTEXPR auto WS_PAYLOAD_LENGTH_63 = 127;

/* DEFAULT SETTINGS AS MACRO */
CONSTEXPR auto DEFAULT_WEBSERVER_SERVERNAME = "UNKNOWN";
CONSTEXPR auto DEFAULT_WEBSERVER_SERVERPORT = NULL;
CONSTEXPR auto DEFAULT_WEBSERVER_FREQUENZ = 30;
CONSTEXPR auto DEFAULT_WEBSERVER_SPAM_PROTECTION_TIMER = 0.5f;
CONSTEXPR auto DEFAULT_WEBSERVER_MAX_PEERS = 256;
CONSTEXPR auto DEFAULT_WEBSERVER_SSL = false;
CONSTEXPR auto DEFAULT_WEBSERVER_CertFileName = "cert.pem";
CONSTEXPR auto DEFAULT_WEBSERVER_KeyFileName = "key.pem";
CONSTEXPR auto DEFAULT_WEBSERVER_CaFileName = "ca.pem";
CONSTEXPR auto DEFAULT_WEBSERVER_CustomHandshake = false;
CONSTEXPR auto DEFAULT_WEBSERVER_COMPRESS_PACKAGES = true;
CONSTEXPR auto DEFAULT_WEBSERVER_MAX_PACKET_SIZE = 4096;
CONSTEXPR auto DEFAULT_WEBSERVER_TCP_READ_TIMEOUT = 10;
CONSTEXPR auto DEFAULT_WEBSERVER_WITHOUT_HANDSHAKE = false;
CONSTEXPR auto DEFAULT_WEBSERVER_CALC_LATENCY_INTERVAL = 1000;

#include <Net/Cryption/AES.h>
#include <Net/Cryption/RSA.h>
#include <Net/Coding/MD5.h>
#include <Net/Coding/BASE64.h>
#include <Net/Coding/SHA1.h>
#include <Net/Compression/Compression.h>

#include <OpenSSL/ssl.h>
#include <OpenSSL/err.h>

#include <Net/Protocol/ICMP.h>

#include <Net/assets/thread.h>
#include <Net/assets/timer.h>

NET_NAMESPACE_BEGIN(Net)
NET_NAMESPACE_BEGIN(WebServer)
NET_DSA_BEGIN
NET_CLASS_BEGIN(IPRef)
char* pointer;

NET_CLASS_PUBLIC
NET_CLASS_CONSTRUCTUR(IPRef, PCSTR)
NET_CLASS_DESTRUCTUR(IPRef)

PCSTR get() const;
NET_CLASS_END

NET_ABSTRAC_CLASS_BEGIN(Server, Package)
NET_CLASS_PUBLIC
#pragma region PEERS TABLE
#pragma region Network Structure
NET_STRUCT_BEGIN(network_t)
byte _dataReceive[DEFAULT_WEBSERVER_MAX_PACKET_SIZE];
CPOINTER<byte> _data;
size_t _data_size;
CPOINTER<byte> _dataFragment;
size_t _data_sizeFragment;

NET_STRUCT_BEGIN_CONSTRUCTUR(network_t)
reset();
clear();
NET_STRUCT_END_CONTRUCTION

void setData(byte*);
void setDataFragmented(byte*);

void allocData(size_t);
void deallocData();

void allocDataFragmented(size_t);
void deallocDataFragmented();

byte* getData() const;
byte* getDataFragmented() const;

void reset();
void clear();

void setDataSize(size_t);
size_t getDataSize() const;

void setDataFragmentSize(size_t);
size_t getDataFragmentSize() const;

bool dataValid() const;
bool dataFragmentValid() const;

byte* getDataReceive();
NET_STRUCT_END
#pragma endregion

// table to keep track of each client's socket
NET_STRUCT_BEGIN(NET_IPEER)
NET_UID UniqueID;
SOCKET pSocket;
struct sockaddr_in client_addr;
float lastaction;

bool estabilished;

/* network data */
network_t network;

/* Async Handler */
bool isAsync;

/* SSL */
SSL* ssl;

/* Handshake */
bool handshake;

typeLatency latency;
bool bLatency;
NET_HANDLE_TIMER hCalcLatency;

bool bHasBeenErased;
bool bQueueLock;

NET_STRUCT_BEGIN_CONSTRUCTUR(peerInfo)
UniqueID = INVALID_UID;
pSocket = INVALID_SOCKET;
client_addr = sockaddr_in();
estabilished = false;
isAsync = false;
ssl = nullptr;
handshake = false;
latency = -1;
bLatency = false;
hCalcLatency = nullptr;
bHasBeenErased = false;
bQueueLock = false;
NET_STRUCT_END_CONTRUCTION

void clear();
void setAsync(bool);
IPRef IPAddr() const;

void lock();
void unlock();
NET_STRUCT_END

void DisconnectPeer(NET_PEER, int);
#pragma endregion

NET_CLASS_PRIVATE
size_t _CounterPeersTable;
void IncreasePeersCounter();
void DecreasePeersCounter();
NET_PEER CreatePeer(sockaddr_in, SOCKET);

char sServerName[128];
u_short sServerPort;
DWORD sfrequenz;
float sTimeSpamProtection;
unsigned int sMaxPeers;
bool sSSL;
char sCertFileName[MAX_PATH];
char sKeyFileName[MAX_PATH];
char sCaFileName[MAX_PATH];
bool hUseCustom;
CPOINTER<char> hOrigin;
bool sCompressPackage;
long sTCPReadTimeout;
bool bWithoutHandshake;
long sCalcLatencyInterval;
std::vector<SocketOption_t<void*>> socketoption;

NET_CLASS_PUBLIC
void SetAllToDefault();
void SetServerName(const char*);
void SetServerPort(u_short);
void SetFrequenz(DWORD);
void SetTimeSpamProtection(float);
void SetMaxPeers(unsigned int);
void SetSSL(bool);
void SetCertFileName(const char*);
void SetKeyFileName(const char*);
void SetCaFileName(const char*);
void SetCustomHandshakeMethode(bool);
void SetHandshakeOriginCompare(const char*);
void SetCompressPackage(bool);
void SetTCPReadTimeout(long);
void SetWithoutHandshake(bool);
void SetCalcLatencyInterval(long);

template <class T>
void SetSocketOption(const SocketOption_t<T> opt)
{
	SocketOption_t<void*> option;
	option.opt = opt.opt;
	option.type = reinterpret_cast<void*>(opt.type);
	option.len = opt.len;
	socketoption.emplace_back(option);
}

const char* GetServerName() const;
u_short GetServerPort() const;
DWORD GetFrequenz() const;
float GetTimeSpamProtection() const;
unsigned int GetMaxPeers() const;
bool GetSSL() const;
const char* GetCertFileName() const;
const char* GetKeyFileName() const;
const char* GetCaFileName() const;
bool GetCustomHandshakeMethode() const;
CPOINTER<char> GetHandshakeOriginCompare() const;
bool GetCompressPackage() const;
long GetTCPReadTimeout() const;
bool GetWithoutHandshake() const;
long GetCalcLatencyInterval() const;

NET_CLASS_PRIVATE
SOCKET ListenSocket;
SOCKET AcceptSocket;

bool bRunning;
bool bShuttingDown;

NET_CLASS_PUBLIC
void SetListenSocket(SOCKET);
void SetAcceptSocket(SOCKET);
void SetRunning(bool);

SOCKET GetListenSocket() const;
SOCKET GetAcceptSocket() const;
bool IsRunning() const;

bool ErasePeer(NET_PEER);

NET_CLASS_CONSTRUCTUR(Server)
NET_CLASS_VDESTRUCTUR(Server)
bool Start(const char*, u_short, ssl::NET_SSL_METHOD = ssl::NET_SSL_METHOD::NET_SSL_METHOD_TLS);
bool Close();

bool DoExit;

short Handshake(NET_PEER);
void Acceptor();

SSL_CTX* ctx;
void onSSLTimeout(NET_PEER);

bool CheckDataN(NET_PEER peer, int id, NET_PACKAGE pkg);

NET_CLASS_PUBLIC
NET_DEFINE_CALLBACK(void, Tick) {}
NET_DEFINE_CALLBACK(bool, CheckData, NET_PEER peer, const int id, NET_PACKAGE pkg) { return false; }
bool NeedExit() const;

void DoSend(NET_PEER, int, NET_PACKAGE, unsigned char = OPCODE_TEXT);

size_t getCountPeers() const;

DWORD DoReceive(NET_PEER);

/* CALLBACKS */
NET_DEFINE_CALLBACK(void, OnPeerUpdate, NET_PEER) {}
NET_DEFINE_CALLBACK(void, OnPeerEstabilished, NET_PEER) {}

NET_CLASS_PRIVATE
void DecodeFrame(NET_PEER);
void EncodeFrame(const char*, size_t, NET_PEER, unsigned char = OPCODE_TEXT);
bool ProcessPackage(NET_PEER, BYTE*, size_t);

NET_CLASS_PROTECTED
/* CALLBACKS */
NET_DEFINE_CALLBACK(void, OnPeerConnect, NET_PEER) {}
NET_DEFINE_CALLBACK(void, OnPeerDisconnect, NET_PEER) {}
NET_CLASS_END
NET_DSA_END
NET_NAMESPACE_END
NET_NAMESPACE_END