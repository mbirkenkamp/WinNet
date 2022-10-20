#pragma once
#define NET_STRING_NOT_FOUND (size_t)(~0)

#include <Net/Net/Net.h>
#include <Net/Cryption/XOR.h>

NET_DSA_BEGIN
namespace Net
{
	namespace Pointer
	{
		class UniquePointer
		{
			void* _pointer;

		public:
			UniquePointer(void*);
			~UniquePointer();

			template<typename T>
			T get() const
			{
				return (T)_pointer;
			}

			template<typename T>
			T pointer() const
			{
				return (T)_pointer;
			}

			template<typename T>
			T data() const
			{
				return (T)_pointer;
			}
		};
	}

	class String
	{
		RUNTIMEXOR _string;
		Net::Cryption::XOR_UNIQUEPOINTER revert();

		void Construct(const char);
		void Construct(const char*, ...);

	public:
		String();
		String(char);
		String(const char*, ...);
		String(char*);
		String(String&);
		String(String&&) NOEXPECT;
		~String();

		enum type
		{
			NONE = 0x0,
			NOT_CASE_SENS,
			IN_LEN
		};

		void operator=(const char* in)
		{
			set(in);
		}

		void operator=(char* in)
		{
			set(in);
		}

		void operator=(const char in)
		{
			set(in);
		}

		void operator=(String in)
		{
			copy(in);
		}

		void operator+=(const char* in)
		{
			if (size() != INVALID_SIZE && size() != 0)
				append(in);
			else
				set(in);
		}

		void operator+=(char* in)
		{
			if (size() != INVALID_SIZE && size() != 0)
				append(in);
			else
				set(in);
		}

		void operator+=(const char in)
		{
			if (size() != INVALID_SIZE && size() != 0)
				append(in);
			else
				set(in);
		}

		void operator+=(String in)
		{
			if (size() != INVALID_SIZE && size() != 0)
				append(in);
			else
				set(in);
		}

		void operator-=(const char* in)
		{
			erase(in);
		}

		void operator-=(char* in)
		{
			erase(in);
		}

		void operator-=(const char in)
		{
			erase(in);
		}

		void operator-=(String& in)
		{
			erase(in);
		}

		char operator[](size_t i)
		{
			return this->_string.operator[](i);
		}

		size_t size() const;
		size_t length() const;
		void set(char, ...);
		void append(char);
		void set(const char*, ...);
		void append(const char*, ...);
		void set(String&, ...);
		void append(String&, ...);
		void copy(String&);
		void move(String&&);
		Net::Cryption::XOR_UNIQUEPOINTER str();
		Net::Cryption::XOR_UNIQUEPOINTER cstr();
		Net::Cryption::XOR_UNIQUEPOINTER get();
		Net::Cryption::XOR_UNIQUEPOINTER data();
		void clear();
		bool empty();
		char at(size_t);
		char* substr(size_t);
		char* substr(size_t, size_t);
		size_t find(const char, const char = NONE);
		size_t find(const char*, const char = NONE);
		size_t find(size_t, char, const char = NONE);
		size_t find(size_t, const char*, const char = NONE);
		std::vector<size_t> findAll(char, const char = NONE);
		std::vector<size_t> findAll(const char*, const char = NONE);
		std::vector<size_t> findAll(size_t, char, const char = NONE);
		std::vector<size_t> findAll(size_t, const char*, const char = NONE);
		size_t findLastOf(char, const char = NONE);
		size_t findLastOf(const char*, const char = NONE);
		bool compare(const char, const char = NONE);
		bool compare(const char*, const char = NONE);
		bool erase(size_t);
		bool erase(size_t, size_t);
		bool erase(char, size_t = 0);
		bool erase(char, char);
		bool erase(char, size_t, char);
		bool erase(const char*, size_t = 0);
		bool erase(const char*, char);
		bool erase(const char*, size_t, char);
		bool erase(String&, size_t = 0);
		bool erase(String&, char);
		bool erase(String&, size_t, char);
		bool eraseAll(char, const char = NONE);
		bool eraseAll(const char*, const char = NONE);
		bool eraseAll(std::vector<size_t>);
		bool replace(char, char, size_t = 0);
		bool replace(char, const char*, size_t = 0);
		bool replace(const char*, char, size_t = 0);
		bool replace(const char*, const char*, size_t = 0);
		bool replaceAll(char, char);
		bool replaceAll(char, const char*);
		bool replaceAll(const char*, char);
		bool replaceAll(const char*, const char*);
	};
}
NET_DSA_END