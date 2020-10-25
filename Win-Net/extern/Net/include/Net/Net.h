#pragma once
#pragma warning(disable: 4996)
#pragma warning(disable: 4006)
#pragma warning(disable: 4081)
#define _WINSOCKET_DEPRECATED_NO_WARNINGS

#undef NET_TEST_MEMORY_LEAKS

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <thread>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <intrin.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include <windows.h>
#include <wincrypt.h>

#include <OpenSSL/ssl.h>

///////////////////////////////////////////////////////////
//    SECTION - Library Version & Version Key     //
//////////////////////////////////////////////////////////
/* Net Key to compare on which version we are running */
#define NetMajorVersion 3 // Re-Code - Library Changes
#define NetMinorVersion 4 // Function extension
#define NetRevision 4 // Issue fixing
#define NetKey "1MFOm3a9as-xieg1iEMIf-pgKHPNlSMP-pgKHPNlSMP"

static short NET_MAJOR_VERSION()
{
	return NetMajorVersion;
}

static short NET_MINOR_VERSION()
{
	return NetMinorVersion;
}

static short NET_REVISION()
{
	return NetRevision;
}

static const char* NET_KEY()
{
	return NetKey;
}

static const char* NET_VERSION()
{
	static char version[MAX_PATH];
	sprintf_s(version, "%i.%i.%i-%s", NET_MAJOR_VERSION(), NET_MINOR_VERSION(), NET_REVISION(), NET_KEY());
	return version;
}

///////////////////////////////////
//    SECTION - DEFINES     //
//////////////////////////////////
#ifdef DLL
#define NET_API __declspec(dllexport)
#else
#define NET_API
#endif

#define NET_NAMESPACE_BEGIN(n) namespace n {
#define NET_NAMESPACE_END }

#define NET_CLASS_BEGIN(c) class NET_API c {
#define NET_ABSTRAC_CLASS_BEGIN(c, d) class NET_API c : public d {
#define NET_CLASS_END };
#define NET_CLASS_CONSTRUCTUR(a, ...) a(__VA_ARGS__);
#define NET_CLASS_CONSTRUCTUR_NOEXCEPT(a, ...) a(__VA_ARGS__) noexcept;
#define NET_CLASS_VCONSTRUCTUR(a, ...) virtual a(__VA_ARGS__);
#define NET_CLASS_DESTRUCTUR(d) ~d();
#define NET_CLASS_VDESTRUCTUR(d) virtual ~d();
#define NET_CLASS_BEGIN_CONSTRUCTUR(a, ...) a(__VA_ARGS__) {
#define NET_CLASS_BEGIN_VCONSTRUCTUR(a, ...) virtual a(__VA_ARGS__) {
#define NET_CLASS_BEGIN_DESTRUCTUR(d) ~d() {
#define NET_CLASS_BEGIN_VDESTRUCTUR(d) virtual ~d() {
#define NET_CLASS_END_CONTRUCTION }
#define NET_CLASS_END_DESTRUCTUR }
#define NET_CLASS_PRIVATE private:
#define NET_CLASS_PUBLIC public:
#define NET_CLASS_PROTECTED protected:

#define NET_STRUCT_BEGIN(s) struct s {
#define NET_ABSTRACT_STRUCT(s, t) struct s : t {
#define NET_STRUCT_END };
#define NET_STRUCT_CONSTRUCTUR(c) c();
#define NET_STRUCT_VCONSTRUCTUR(c) virtual c();
#define NET_STRUCT_DESTRUCTUR(d) ~d();
#define NET_STRUCT_VDESTRUCTUR(d) virtual ~d();
#define NET_STRUCT_BEGIN_CONSTRUCTUR(c) c() {
#define NET_STRUCT_BEGIN_VCONSTRUCTUR(c) virtual c() {
#define NET_STRUCT_BEGIN_DESTRUCTUR(d) ~d() {
#define NET_STRUCT_BEGIN_VDESTRUCTUR(d) virtual ~d() {
#define NET_STRUCT_END_CONTRUCTION }

#define NET_CALLBACK(type, name, ...) \
type name(__VA_ARGS__) override;

#define NET_DEFINE_CALLBACK(type, name, ...) \
virtual type name(__VA_ARGS__)

#define DEBUG_BREAK __debugbreak();

#define NET_SERVER_BEGIN_DATA_PACKAGE_NATIVE(classname) \
bool classname::CheckDataN(NET_PEER peer, const int id, NET_PACKAGE pkg) \
{ \
if(!peer) \
	return false; \
switch (id) \
{

#define NET_SERVER_BEGIN_DATA_PACKAGE(classname) \
bool classname::CheckData(NET_PEER peer, const int id, NET_PACKAGE pkg) \
{ \
if(!peer || !peer->estabilished) \
	return false; \
switch (id) \
{

#define NET_SERVER_DEFINE_PACKAGE(xxx, yyy) \
    case yyy: \
    { \
      On##xxx(peer, pkg); \
      break; \
    }

#define NET_SERVER_END_DATA_PACKAGE \
	default: \
		return false; \
} \
return true; \
}

#define NET_CLIENT_BEGIN_DATA_PACKAGE_NATIVE(classname) \
bool classname::CheckDataN(const int id, NET_PACKAGE pkg) \
{ \
switch (id) \
{

#define NET_CLIENT_BEGIN_DATA_PACKAGE(classname) \
bool classname::CheckData(const int id, NET_PACKAGE pkg) \
{ \
switch (id) \
{

#define NET_CLIENT_DEFINE_PACKAGE(xxx, yyy) \
    case yyy: \
    { \
      On##xxx(pkg); \
      break; \
    } \

#define NET_CLIENT_END_DATA_PACKAGE \
	default: \
		return false; \
} \
return true; \
}

#define NET_ASSOCIATION(NAME, CLASS) class NAME final : public CLASS

/* DATA STRUCTURE ALIGNEMNT */
#define NET_DSA_BEGIN __pragma("pack(push)") \
 __pragma("pack(1)") \

#define NET_DSA_END __pragma("pack(pop)")

#define ALLOC Alloc
#define FREE(data) Free(data)

////////////////////////////////////////////////
//    SECTION - TYPE DEFENITIONS     //
///////////////////////////////////////////////
typedef unsigned int INDEX;

#ifdef _WIN64
typedef DWORD64 typeLatency;
#else
typedef DWORD typeLatency;
#endif

//////////////////////////////////////////////////////
//    SECTION - Allocation & Deallocation     //
////////////////////////////////////////////////////
#ifdef NET_TEST_MEMORY_LEAKS
static std::vector<void*> NET_TEST_MEMORY_LEAKS_POINTER_LIST;

__forceinline void NET_TEST_MEMORY_SHOW_DIAGNOSTIC()
{
	printf("----- POINTER INSTANCE(s) -----\n");
	for(const auto entry : NET_TEST_MEMORY_LEAKS_POINTER_LIST)
		printf("Allocated Instance: %p\n", entry);
	printf("----------------------------------------\n");
}
#endif

template <typename T>
T* NET_ALLOC_MEM(const size_t n)
{
	try
	{
		T* pointer = new T[n];
		if (pointer)
		{
#ifdef NET_TEST_MEMORY_LEAKS
			printf("Allocated: %llu Byte(s) ; %p\n", n, pointer);
			NET_TEST_MEMORY_LEAKS_POINTER_LIST.emplace_back(pointer);
#endif

			return pointer;
		}

		throw std::bad_alloc();
	}
	catch (...)
	{
		MessageBoxA(nullptr, "The Program seems to run out of memory!", "Memory Failure", MB_OK | MB_ICONERROR);
		return nullptr;
	}
}

__forceinline void NET_FREE_MEM(void* pointer)
{
	if (!pointer)
		return;

#ifdef NET_TEST_MEMORY_LEAKS
	printf("Deallocated: %p\n", pointer);
	for (auto it = NET_TEST_MEMORY_LEAKS_POINTER_LIST.begin(); it != NET_TEST_MEMORY_LEAKS_POINTER_LIST.end(); ++it)
	{
		if (*it == pointer)
		{
			NET_TEST_MEMORY_LEAKS_POINTER_LIST.erase(it);
			break;
		}
	}
#endif
	
	free(pointer);
	pointer = nullptr;
}

template<class T>
T* Alloc(const size_t size = 1)
{
	T* object = NET_ALLOC_MEM<T>(size);
	return object;
}

template <typename T>
static void Free(T*& data)
{
	NET_FREE_MEM(data);
	data = nullptr;
}

////////////////////////////////////////////////////
//    SECTION - Package Prefix & Suffix     //
//////////////////////////////////////////////////
#define NET_PACKAGE_BRACKET_OPEN "{"
#define NET_PACKAGE_BRACKET_CLOSE "}"

#define NET_RAW_DATA_KEY "{RAW DATA KEY}"
#define NET_RAW_DATA "{RAW DATA}"

#define NET_DATA "{DATA}"

#define NET_PACKAGE_HEADER "{BEGIN PACKAGE}"
#define NET_PACKAGE_FOOTER "{END PACKAGE}"

#define NET_PACKAGE_SIZE "{PACKAGE SIZE}"

// Key is crypted using RSA
#define NET_AES_KEY "{KEY}"

// IV is crypted using RSA
#define NET_AES_IV "{IV}"

#define NET_UID size_t
#define INVALID_UID  (size_t)(~0)
#define INVALID_SIZE (size_t)(~0)

#define SOCKET_VALID(socket) if(socket != INVALID_SOCKET)
#define SOCKET_NOT_VALID(socket) if(socket == INVALID_SOCKET)

#define PEER_VALID(peer) if(peer)
#define PEER_NOT_VALID(peer, stuff) if(!peer) \
	{ \
		LOG_ERROR(CSTRING("[%s] - Peer has no instance!"), GetServerName()); \
		stuff \
	}

/////////////////////////////////////////////////////
//    SECTION - SSL Methode Definitions     //
////////////////////////////////////////////////////
struct NET_SSL_METHOD_T
{
	int method;
	char name[32];
};

enum NET_SSL_METHOD
{
	NET_SSL_METHOD_TLS,
	NET_SSL_METHOD_TLS_SERVER,
	NET_SSL_METHOD_TLS_CLIENT,
	NET_SSL_METHOD_SSLv23,
	NET_SSL_METHOD_SSLv23_SERVER,
	NET_SSL_METHOD_SSLv23_CLIENT,
#ifndef OPENSSL_NO_SSL3_METHOD
	NET_SSL_METHOD_SSLv3,
	NET_SSL_METHOD_SSLv3_SERVER,
	NET_SSL_METHOD_SSLv3_CLIENT,
#endif
#ifndef OPENSSL_NO_TLS1_METHOD
	NET_SSL_METHOD_TLSv1,
	NET_SSL_METHOD_TLSv1_SERVER,
	NET_SSL_METHOD_TLSv1_CLIENT,
#endif
#ifndef OPENSSL_NO_TLS1_1_METHOD
	NET_SSL_METHOD_TLSv1_1,
	NET_SSL_METHOD_TLSv1_1_SERVER,
	NET_SSL_METHOD_TLSv1_1_CLIENT,
#endif
#ifndef OPENSSL_NO_TLS1_2_METHOD
	NET_SSL_METHOD_TLSv1_2,
	NET_SSL_METHOD_TLSv1_2_SERVER,
	NET_SSL_METHOD_TLSv1_2_CLIENT,
#endif
	NET_SSL_METHOD_DTLS,
	NET_SSL_METHOD_DTLS_SERVER,
	NET_SSL_METHOD_DTLS_CLIENT,
#ifndef OPENSSL_NO_DTLS1_METHOD
	NET_SSL_METHOD_DTLSv1,
	NET_SSL_METHOD_DTLSv1_SERVER,
	NET_SSL_METHOD_DTLSv1_CLIENT,
#endif
#ifndef OPENSSL_NO_DTLS1_2_METHOD
	NET_SSL_METHOD_DTLSv1_2,
	NET_SSL_METHOD_DTLSv1_2_SERVER,
	NET_SSL_METHOD_DTLSv1_2_CLIENT,
#endif

	NET_LAST_NET_SSL_METHOD
};

static NET_SSL_METHOD_T NET_SSL_METHOD_L[] =
{
	{NET_SSL_METHOD::NET_SSL_METHOD_TLS, "TLS"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLS_SERVER, "TLS Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLS_CLIENT, "TLS Client"},
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv23, "SSLv23"},
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv23_SERVER, "SSLv23 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv23_CLIENT, "SSLv23 Client"},
#ifndef OPENSSL_NO_SSL3_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv3, "SSLv3"},
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv3_SERVER, "SSLv3 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_SSLv3_CLIENT, "SSLv3 Client"},
#endif
#ifndef OPENSSL_NO_TLS1_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1, "TLSv1"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_SERVER, "TLSv1 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_CLIENT,"TLSv1 Client"},
#endif
#ifndef OPENSSL_NO_TLS1_1_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1,  "TLSv1_1"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1_SERVER,  "TLSv1_1 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1_CLIENT, "TLSv1_1 Client"},
#endif
#ifndef OPENSSL_NO_TLS1_2_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2, "TLSv1_2"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2_SERVER,  "TLSv1_2 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2_CLIENT, "TLSv1_2 Client"},
#endif
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLS,  "DTLS"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLS_SERVER, "DTLS Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLS_CLIENT, "DTLS Client"},
#ifndef OPENSSL_NO_DTLS1_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1,  "DTLSv1"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_SERVER, "DTLSv1 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_CLIENT, "DTLSv1 Client"},
#endif
#ifndef OPENSSL_NO_DTLS1_2_METHOD
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2, "DTLSv1_2"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2_SERVER, "DTLSv1_2 Server"},
	{NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2_CLIENT, "DTLSv1_2 Client"}
#endif
};

inline const SSL_METHOD* NET_CREATE_SSL_OBJECT(const int method)
{
	for (auto& val : NET_SSL_METHOD_L)
	{
		if (val.method == method)
		{
			switch (val.method)
			{
			case NET_SSL_METHOD::NET_SSL_METHOD_TLS:
				return TLS_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLS_SERVER:
				return TLS_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLS_CLIENT:
				return TLS_client_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv23:
				return SSLv23_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv23_SERVER:
				return SSLv23_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv23_CLIENT:
				return SSLv23_client_method();

#ifndef OPENSSL_NO_SSL3_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv3:
				return SSLv3_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv3_SERVER:
				return SSLv3_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_SSLv3_CLIENT:
				return SSLv3_client_method();
#endif

#ifndef OPENSSL_NO_TLS1_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1:
				return TLSv1_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_SERVER:
				return TLSv1_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_CLIENT:
				return TLSv1_client_method();
#endif

#ifndef OPENSSL_NO_TLS1_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1:
				return TLSv1_1_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1_SERVER:
				return TLSv1_1_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_1_CLIENT:
				return TLSv1_1_client_method();
#endif

#ifndef OPENSSL_NO_TLS1_2_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2:
				return TLSv1_2_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2_SERVER:
				return TLSv1_2_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_TLSv1_2_CLIENT:
				return TLSv1_2_client_method();
#endif

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLS:
				return DTLS_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLS_SERVER:
				return DTLS_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLS_CLIENT:
				return DTLS_client_method();

#ifndef OPENSSL_NO_DTLS1_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1:
				return DTLSv1_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_SERVER:
				return DTLSv1_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_CLIENT:
				return DTLSv1_client_method();
#endif

#ifndef OPENSSL_NO_DTLS1_2_METHOD
			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2:
				return DTLSv1_2_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2_SERVER:
				return DTLSv1_2_server_method();

			case NET_SSL_METHOD::NET_SSL_METHOD_DTLSv1_2_CLIENT:
				return DTLSv1_2_client_method();
#endif
			default:
				break;
			}
		}
	}

	return nullptr;
}

static char* NET_GET_SSL_METHOD_NAME(const int method)
{
	for (auto& val : NET_SSL_METHOD_L)
	{
		if (val.method == method)
			return val.name;
	}

	return (char*)"UNKNOWN";
}

////////////////////////////////////
//    USEFULL FUNCTIONS    //
///////////////////////////////////
static bool NET_STRING_IS_NUMBER(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

NET_NAMESPACE_BEGIN(ServerHandshake)
enum Server_HandshakeRet_t
{
	is_not_websocket = 0,
	is_websocket,
	error,
	peer_not_valid,
	would_block
};
NET_NAMESPACE_END

NET_NAMESPACE_BEGIN(WebServerHandshake)
enum HandshakeRet_t
{
	success = 0,
	missmatch,
	error,
	peer_not_valid,
	would_block
};
NET_NAMESPACE_END

template <class T>
NET_STRUCT_BEGIN(SocketOption_t)
DWORD opt;
T type;
size_t len;

SocketOption_t()
{
	this->opt = NULL;
}

SocketOption_t(const DWORD opt, const T type)
{
	this->opt = opt;
	this->type = type;
	this->len = sizeof(type);
}
NET_STRUCT_END

static int _SetSocketOption(const SOCKET socket, const SocketOption_t<char*> opt)
{
	const auto result = setsockopt(socket,
		IPPROTO_TCP,
		opt.opt,
		(char*)&opt.type,
		opt.len);

	return result;
}