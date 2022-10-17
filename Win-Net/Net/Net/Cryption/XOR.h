#pragma once
#include <Net/Net/Net.h>
#include <Net/Cryption/PointerCryption.h>
#include <Net/Cryption/CXOR.hpp>

#define RUNTIMEXOR Net::Cryption::XOR

#ifdef BUILD_LINUX
#define CASTRING(string) string
#define CWSTRING(string) L##string
#else
#define COMPILETIME_XOR(string) xorstr_(string)
#define WCOMPILETIME_XOR(string) xorstr_(string)
#define CASTRING(string) COMPILETIME_XOR(string)
#define CWSTRING(string) WCOMPILETIME_XOR(L##string)
#endif

#ifdef NET_DISABLE_XOR_STRING_COMPILETIME
#define CSTRING(string) string
#else
#ifdef UNICODE
#define CSTRING(string) CWSTRING(string)
#else
#define CSTRING(string) CASTRING(string)
#endif
#endif

NET_DSA_BEGIN
namespace Net
{
	namespace Cryption
	{
		class XOR_UNIQUEPOINTER
		{
			NET_CPOINTER<char> buffer;
			size_t _length;
			bool bFree;

		public:
			XOR_UNIQUEPOINTER(char*, size_t, bool);
			~XOR_UNIQUEPOINTER();

			char* get() const;
			char* data()  const;
			char* str() const;
			size_t length() const;
			size_t size() const;

			void free();
			void lost_reference();
		};

		class XOR
		{
			NET_CPOINTER<char> _buffer;
			size_t _length;
			NET_CPOINTER<size_t> _Key;

			char* encrypt();
			char* decrypt() const;

		public:
			XOR();
			XOR(char*);

			/*
			* Return the character in the buffer located at index i decrypted
			*/
			char operator[](size_t i)
			{
				auto buffer_ptr = this->_buffer.get();
				auto key_ptr = this->_Key.get();
				return static_cast<char>(buffer_ptr[i] ^ key_ptr[i]);
			}

			void init(char*);
			void init(const char*);
			size_t size() const;
			size_t length() const;
			XOR_UNIQUEPOINTER revert(bool = true);
			void free();
		};
	}
}
NET_DSA_END