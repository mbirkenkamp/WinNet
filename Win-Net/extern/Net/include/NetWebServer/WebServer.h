#pragma once
/*
Legend of Variable Names:
 s => Settings
 h => Handshake
*/

#include <Net/Net.h>
#include <Net/Package.h>
#include <Net/NetCodes.hpp>

NET_DSA_BEGIN

/* Websocket frame protocol operationcodes */
constexpr auto OPCODE_CONTINUE = 0x0;
constexpr auto OPCODE_TEXT = 0x1;
constexpr auto OPCODE_BINARY = 0x2;
constexpr auto OPCODE_CLOSE = 0x8;
constexpr auto OPCODE_PING = 0x9;
constexpr auto OPCODE_PONG = 0xA;

constexpr auto WS_CONTROL_PACKAGE = -1; // used to send a ping or pong frame
constexpr auto WS_FIN = 0x80;
constexpr auto WS_OPCODE = 0xF;
constexpr auto WS_MASK = 0x7F;
constexpr auto WS_PAYLOADLENGTH = 0x7F;
constexpr auto WS_CONTROLFRAME = 8;
constexpr auto WS_PAYLOAD_LENGTH_16 = 126;
constexpr auto WS_PAYLOAD_LENGTH_63 = 127;

/* DEFAULT SETTINGS AS MACRO */
constexpr auto DEFAULT_WEBSERVER_SERVERNAME = "UNKNOWN";
constexpr auto DEFAULT_WEBSERVER_SERVERPORT = NULL;
constexpr auto DEFAULT_WEBSERVER_SHUTDOWN_TIMER = 10.0f;
constexpr auto DEFAULT_WEBSERVER_FREQUENZ = 30;
constexpr auto DEFAULT_WEBSERVER_SPAM_PROTECTION_TIMER = 0.5f;
constexpr auto DEFAULT_WEBSERVER_MAX_PEERS = 256;
constexpr auto DEFAULT_WEBSERVER_MAX_THREADS = 3;
constexpr auto DEFAULT_WEBSERVER_SSL = false;
constexpr auto DEFAULT_WEBSERVER_CertFileName = "cert.pem";
constexpr auto DEFAULT_WEBSERVER_KeyFileName = "key.pem";
constexpr auto DEFAULT_WEBSERVER_CaFileName = "ca.pem";
constexpr auto DEFAULT_WEBSERVER_CustomHandshake = false;
constexpr auto DEFAULT_WEBSERVER_COMPRESS_PACKAGES = true;
constexpr auto DEFAULT_WEBSERVER_MAX_PACKET_SIZE = 65535;
constexpr auto DEFAULT_WEBSERVER_SHUTDOWN_KEY = KEYBOARD::F4;
constexpr auto DEFAULT_WEBSERVER_TCP_READ_TIMEOUT = 10; // Seconds
constexpr auto DEFAULT_WEBSERVER_WITHOUT_HANDSHAKE = false;

#define NET_WEB_SERVER Net::WebServer::Server

#define NET_IPEER peerInfo
#define NET_PEER peerInfo&

#include <Cryption/AES.h>
#include <Cryption/RSA.h>
#include <Coding/MD5.h>
#include <Coding/BASE64.h>
#include <Coding/SHA1.h>
#include <Compression/Compression.h>

#include <OpenSSL/ssl.h>
#include <OpenSSL/err.h>

NET_NAMESPACE_BEGIN(Net)
NET_NAMESPACE_BEGIN(WebServer)
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
explicit operator bool() const
{
	return pSocket != INVALID_SOCKET;
}

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

NET_STRUCT_BEGIN_CONSTRUCTUR(peerInfo)
UniqueID = INVALID_UID;
pSocket = INVALID_SOCKET;
client_addr = sockaddr_in();
estabilished = false;
isAsync = false;
ssl = nullptr;
handshake = false;
NET_STRUCT_END_CONTRUCTION

void clear();
void setAsync(bool);
const char* getIPAddr() const;
NET_STRUCT_END

void DisconnectPeer(NET_PEER, int);
#pragma endregion

NET_CLASS_PRIVATE
size_t _CounterPeersTable;
void IncreasePeersCounter();
void DecreasePeersCounter();
NET_IPEER InsertPeer(sockaddr_in, SOCKET);
bool ErasePeer(NET_PEER);
void UpdatePeer(NET_PEER);

char sServerName[128];
u_short sServerPort;
long long sfrequenz;
float sShutdownTimer;
u_short sMaxThreads;
float sTimeSpamProtection;
unsigned int sMaxPeers;
bool sSSL;
char sCertFileName[MAX_PATH];
char sKeyFileName[MAX_PATH];
char sCaFileName[MAX_PATH];
bool hUseCustom;
CPOINTER<char> hOrigin;
bool sCompressPackage;
u_short sShutdownKey;
long sTCPReadTimeout;
bool bWithoutHandshake;

NET_CLASS_PUBLIC
void SetAllToDefault();
void SetServerName(const char*);
void SetServerPort(u_short);
void SetFrequenz(long long);
void SetShutdownTimer(float);
void SetMaxThreads(u_short);
void SetTimeSpamProtection(float);
void SetMaxPeers(unsigned int);
void SetSSL(bool);
void SetCertFileName(const char*);
void SetKeyFileName(const char*);
void SetCaFileName(const char*);
void SetCustomHandshakeMethode(bool);
void SetHandshakeOriginCompare(const char*);
void SetCompressPackage(bool);
void SetShutdownKey(u_short);
void SetTCPReadTimeout(long);
void SetWithoutHandshake(bool);

const char* GetServerName() const;
u_short GetServerPort() const;
long long GetFrequenz() const;
float GetShutdownTimer() const;
u_short GetMaxThreads() const;
float GetTimeSpamProtection() const;
unsigned int GetMaxPeers() const;
bool GetSSL() const;
const char* GetCertFileName() const;
const char* GetKeyFileName() const;
const char* GetCaFileName() const;
bool GetCustomHandshakeMethode() const;
CPOINTER<char> GetHandshakeOriginCompare() const;
bool GetCompressPackage() const;
u_short GetShutdownKey() const;
long GetTCPReadTimeout() const;
bool GetWithoutHandshake() const;

NET_CLASS_PRIVATE
SOCKET ListenSocket;
SOCKET AcceptSocket;

bool DoExit;
bool bRunning;
bool bShuttingDown;

NET_CLASS_PUBLIC
void SetListenSocket(SOCKET);
void SetAcceptSocket(SOCKET);
void SetRunning(bool);
void SetShutdown(bool);

SOCKET GetListenSocket() const;
SOCKET GetAcceptSocket() const;
bool IsRunning() const;
bool IsShutdown() const;
bool DoShutdown;

NET_CLASS_CONSTRUCTUR(Server)
NET_CLASS_VDESTRUCTUR(Server)
bool Start(const char*, u_short, NET_SSL_METHOD = NET_SSL_METHOD::NET_SSL_METHOD_TLS);
bool Close();
void Terminate();

NET_CLASS_PRIVATE
void Acceptor();

SSL_CTX* ctx;
short Handshake(NET_PEER);
void onSSLTimeout(NET_PEER);

bool CheckDataN(NET_PEER peer, int id, NET_PACKAGE pkg);

NET_CLASS_PUBLIC
NET_DEFINE_CALLBACK(void, Tick) {}
NET_DEFINE_CALLBACK(bool, CheckData, NET_PEER peer, const int id, NET_PACKAGE pkg) { return false; }
bool NeedExit() const;
void Shutdown();

void DoSend(NET_PEER, int, NET_PACKAGE, unsigned char = OPCODE_TEXT);

size_t getCountPeers() const;

NET_CLASS_PRIVATE
short ThreadsRunning;
void ReceiveThread(sockaddr_in, SOCKET);
void TickThread();
void AcceptorThread();

void DoReceive(NET_PEER);
void DecodeFrame(NET_PEER);
void EncodeFrame(const char*, size_t, NET_PEER, unsigned char = OPCODE_TEXT);
bool ProcessPackage(NET_PEER, BYTE*, size_t);

NET_CLASS_PROTECTED
/* CALLBACKS */
NET_DEFINE_CALLBACK(void, OnPeerConnect, NET_PEER) {}
NET_DEFINE_CALLBACK(void, OnPeerDisconnect, NET_PEER) {}
NET_DEFINE_CALLBACK(void, OnPeerEstabilished, NET_PEER) {}
NET_DEFINE_CALLBACK(void, OnPeerUpdate, NET_PEER) {}
NET_CLASS_END
NET_NAMESPACE_END
NET_NAMESPACE_END

NET_DSA_END