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

#include <Net/Net/NetString.h>

NET_IGNORE_CONVERSION_NULL
namespace Net
{
	namespace Pointer
	{
		UniquePointer::UniquePointer(void* in)
		{
			_pointer = in;
		}

		UniquePointer::~UniquePointer()
		{
			FREE<UniquePointer>(_pointer);
		}
	}

	ViewString::ViewString()
	{
		this->m_ptr_original = nullptr;
		this->m_ref = {};
		this->m_start = INVALID_SIZE;
		this->m_size = INVALID_SIZE;
		this->m_valid = false;
	}

	ViewString::ViewString(void* m_ptr_original, Net::Cryption::XOR_UNIQUEPOINTER* m_ref, size_t m_start, size_t m_size)
	{
		this->m_ptr_original = m_ptr_original;
		this->m_ref = *m_ref;
		this->m_start = m_start;
		this->m_size = m_size;
		this->m_valid = true;

		/*
		* vs ptr moved
		*/
		m_ref->lost_reference();
	}

	ViewString& ViewString::operator=(const ViewString& vs)
	{
		// Guard self assignment
		if (this == &vs)
			return *this;

		this->m_ptr_original = vs.m_ptr_original;
		this->m_ref = vs.m_ref;
		this->m_start = vs.m_start;
		this->m_size = vs.m_size;
		this->m_valid = vs.m_valid;

		vs.m_ref.lost_reference();
		return *this;
	}

	char ViewString::operator[](size_t i)
	{
		if (!valid()) return '\0';
		return this->m_ref.get()[i];
	}

	ViewString::ViewString(ViewString& vs)
	{
		this->m_ptr_original = vs.m_ptr_original;
		this->m_ref = vs.m_ref;
		this->m_start = vs.m_start;
		this->m_size = vs.m_size;
		this->m_valid = vs.m_valid;

		/*
		* vs ptr moved
		*/
		vs.m_ref.lost_reference();
	}

	ViewString::ViewString(ViewString&& vs) NOEXCEPT
	{
		this->m_ptr_original = vs.m_ptr_original;
		this->m_ref = vs.m_ref;
		this->m_start = vs.m_start;
		this->m_size = vs.m_size;
		this->m_valid = vs.m_valid;

		/*
		* vs ptr moved
		*/
		vs.m_ref.lost_reference();
	}

	size_t ViewString::start() const
	{
		if (!valid()) return INVALID_SIZE;
		return m_start;
	}

	size_t ViewString::end() const
	{
		if (!valid()) return INVALID_SIZE;
		return start() + size();
	}

	size_t ViewString::size() const
	{
		if (!valid()) return INVALID_SIZE;
		return m_size;
	}

	size_t ViewString::length() const
	{
		if (!valid()) return INVALID_SIZE;
		return m_size - 1;
	}

	char* ViewString::get() const
	{
		if (!valid()) return nullptr;
		return m_ref.get();
	}

	bool ViewString::valid() const
	{
		return m_valid;
	}

	void* ViewString::original()
	{
		return m_ptr_original;
	}

	bool ViewString::refresh()
	{
		if (!valid()) return false;
		if (!original()) return false;

		auto tmp = reinterpret_cast<Net::String*>(original())->get();
		this->m_ref = tmp;
		tmp.lost_reference();

		if (this->m_start != 0)
		{
			if (this->m_start - (this->m_size - tmp.size()) == INVALID_SIZE)
				this->m_start = 0;
			else
				this->m_start -= this->m_size - tmp.size();
		}

		this->m_size = tmp.size();

		return true;
	}

	ViewString ViewString::sub_view(size_t m_start, size_t m_size)
	{
		if (!valid())
			return {};

		if (size() == INVALID_SIZE || size() == 0)
			return {};

		ViewString vs;
		vs.m_ptr_original = this->m_ptr_original;
		vs.m_ref = this->m_ref;

		/*
		* ok, so on a sub_view we gotta make sure that it will not free on death
		*/
		vs.m_ref.lost_reference();

		vs.m_start = m_start;
		vs.m_size = m_size;

		vs.m_valid = true;
		return vs;
	}

	String::String()
	{
		_string = RUNTIMEXOR();
		_free_size = INVALID_SIZE;
	}

	String::String(const char in)
	{
		std::vector<char> str(2);
		str[0] = in;
		str[1] = '\0';

		_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		_free_size = INVALID_SIZE;
	}

	String::String(const char* in, ...)
	{
		va_list vaArgs;

#ifdef BUILD_LINUX
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		va_end(vaArgs);

		va_start(vaArgs, in);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#else
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#endif

		_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		_free_size = INVALID_SIZE;
	}

	String::String(char* str)
	{
		/*
		* instead of allocating new space and copy this string
		* we just move it's pointer into ours
		*/
		this->Destroy();
		_string = RUNTIMEXOR(str);
		_free_size = INVALID_SIZE;
	}

	String::String(const String& in)
	{
		move(in);
	}

	void String::Construct(const char in)
	{
		std::vector<char> str(2);
		str[0] = in;
		str[1] = '\0';

		this->_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		this->_free_size = INVALID_SIZE;
	}

	void String::Construct(const char* in, ...)
	{
		va_list vaArgs;

#ifdef BUILD_LINUX
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		va_end(vaArgs);

		va_start(vaArgs, in);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#else
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#endif

		this->_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		this->_free_size = INVALID_SIZE;
	}

	void Net::String::Destroy()
	{
		_string.free();
	}

	void String::copy(const String& in)
	{
		auto cast = const_cast<String*>(&in);

		auto ref = cast->get();
		auto pBuffer = ref.get();

		this->Destroy();
		this->_string = RUNTIMEXOR(reinterpret_cast<const char*>(pBuffer));
		this->_free_size = INVALID_SIZE;
	}

	void String::move(const String& in)
	{
		auto cast = const_cast<String*>(&in);
		_string = cast->_string;
		_free_size = cast->_free_size;

		/*
		* object moved
		*/
		cast->_string.lost_reference();
	}

	String::~String()
	{
		this->Destroy();
	}

	/*
	* this function will pre-allocate space
	* with finalize it will shrunk it by performing one another re-allocation
	* this function should be used anytime append will be called
	* just reserve enough space and finalize it
	*/
	void Net::String::reserve(size_t m_size)
	{
		return _string.reserve(m_size);
	}

	void Net::String::finalize()
	{
		return _string.finalize();
	}

	void Net::String::operator=(const char* in)
	{
		set(in);
	}

	void Net::String::operator=(char* in)
	{
		set(in);
	}

	void Net::String::operator=(const char in)
	{
		set(in);
	}

	void Net::String::operator=(const String& in)
	{
		move(in);
	}

	void Net::String::operator+=(const char* in)
	{
		if (actual_size() != INVALID_SIZE && actual_size() != 0)
			append(in);
		else
			set(in);
	}

	void Net::String::operator+=(char* in)
	{
		if (actual_size() != INVALID_SIZE && actual_size() != 0)
			append(in);
		else
			set(in);
	}

	void Net::String::operator+=(const char in)
	{
		if (actual_size() != INVALID_SIZE && actual_size() != 0)
			append(in);
		else
			set(in);
	}

	void Net::String::operator+=(const String& in)
	{
		auto cast = const_cast<String*>(&in);

		if (actual_size() != INVALID_SIZE && actual_size() != 0)
			append(*cast);
		else
			set(*cast);
	}

	void Net::String::operator-=(const char* in)
	{
		erase(in);
	}

	void Net::String::operator-=(char* in)
	{
		erase(in);
	}

	void Net::String::operator-=(const char in)
	{
		erase(in);
	}

	void Net::String::operator-=(const String& in)
	{
		auto cast = const_cast<String*>(&in);
		erase(*cast);
	}

	bool Net::String::operator==(const char in)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		return this->compare(in);
	}

	bool Net::String::operator==(const char* in)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		return this->compare(in);
	}

	bool Net::String::operator==(const String& in)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		return this->compare(in);
	}

	char Net::String::operator[](size_t i)
	{
		return this->_string.operator[](i);
	}

	size_t String::size() const
	{
		return _string.size();
	}

	size_t String::actual_size() const
	{
		return _string.actual_size();
	}

	size_t String::length() const
	{
		return _string.length();
	}

	void String::set(const char in, ...)
	{
		std::vector<char> str(2);
		str[0] = in;
		str[1] = '\0';

		this->Destroy();
		this->_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		this->_free_size = INVALID_SIZE;
	}

	void String::set(const char* in, ...)
	{
		va_list vaArgs;

#ifdef BUILD_LINUX
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		va_end(vaArgs);

		va_start(vaArgs, in);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#else
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#endif

		this->Destroy();
		_string = RUNTIMEXOR(reinterpret_cast<const char*>(str.data()));
		_free_size = INVALID_SIZE;
	}

	void String::set(const String& in, ...)
	{
		auto cast = const_cast<String*>(&in);

		auto ref = cast->get();
		auto ptr = ref.get();

		this->Destroy();
		this->_string = RUNTIMEXOR(reinterpret_cast<const char*>(ptr));
		this->_free_size = INVALID_SIZE;
	}

	void String::append(const char in)
	{
		if (actual_size() == INVALID_SIZE || actual_size() == 0)
		{
			Construct(in);
			return;
		}

		if (actual_size() > size())
		{
			auto prevLen = size();
			_string.set_size(size() + 1);
			_string.set(prevLen, in);
			_string.set(size(), '\0');
		}
		else
		{
			if (_free_size != INVALID_SIZE && _free_size >= 1)
			{
				/*
				* use _free_size to fill up the free space in our current buffer
				*/
				auto prevLen = size();
				_string.set_size(size() + 1);
				_string.set(prevLen, in);
				_string.set(size(), '\0');
				_free_size -= 1;
			}
			else
			{
				size_t newLen = _string.size() + 1;
				NET_CPOINTER<byte> data(ALLOC<byte>(newLen + 1));
				for (size_t i = 0; i < _string.size(); ++i)
				{
					data.get()[i] = _string.operator[](i);
				}
				data.get()[_string.size()] = in;
				data.get()[newLen] = '\0';

				this->Destroy();
				_string = RUNTIMEXOR(reinterpret_cast<char*>(data.get()), true);
			}
		}
	}

	void Net::String::append(char* buffer)
	{
		if (actual_size() == INVALID_SIZE || actual_size() == 0)
		{
			Construct(buffer);
			return;
		}

		auto buffer_len = strlen(buffer);

		if (actual_size() > size())
		{
			auto prevLen = size();
			_string.set_size(size() + buffer_len);
			for (size_t i = prevLen, j = 0; j < buffer_len; ++j)
			{
				_string.set(i + j, buffer[j]);
			}
			_string.set(size(), '\0');
		}
		else
		{
			if (_free_size == INVALID_SIZE || buffer_len > _free_size)
			{
				size_t newSize = _string.size() + buffer_len;
				NET_CPOINTER<byte> data(ALLOC<byte>(newSize + 1));
				for (size_t i = 0; i < _string.size(); ++i)
				{
					data.get()[i] = _string.operator[](i);
				}
				memcpy(&data.get()[_string.size()], buffer, buffer_len);
				data.get()[newSize] = '\0';

				this->Destroy();
				_string = RUNTIMEXOR(reinterpret_cast<char*>(data.get()), true);
				_free_size = INVALID_SIZE;

				return;
			}

			/*
			* use _free_size to fill up the free space in our current buffer
			*/
			auto prevLen = size();
			_string.set_size(size() + buffer_len);
			for (size_t i = prevLen, j = 0; j < buffer_len; ++j)
			{
				_string.set(i + j, buffer[j]);
			}
			_string.set(size(), '\0');
			_free_size -= buffer_len;
		}
	}

	void String::append(const char* in, ...)
	{
		if (actual_size() == INVALID_SIZE || actual_size() == 0)
		{
			Construct(in);
			return;
		}

		va_list vaArgs;

#ifdef BUILD_LINUX
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		va_end(vaArgs);

		va_start(vaArgs, in);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#else
		va_start(vaArgs, in);
		const size_t size = std::vsnprintf(nullptr, 0, in, vaArgs);
		std::vector<char> str(size + 1);
		std::vsnprintf(str.data(), str.size(), in, vaArgs);
		va_end(vaArgs);
#endif

		this->append(str.data());
	}

	void String::append(const String& in)
	{
		if (actual_size() == INVALID_SIZE || actual_size() == 0)
		{
			copy(in);
			return;
		}

		auto cast = const_cast<String*>(&in);

		auto ref = cast->get();
		auto ptr = ref.get();

		this->append(reinterpret_cast<char*>(ptr));
	}

	Net::Cryption::XOR_UNIQUEPOINTER String::str()
	{
		if (size() == INVALID_SIZE)
			return Net::Cryption::XOR_UNIQUEPOINTER(nullptr, NULL, false);

		return _string.revert();
	}

	Net::Cryption::XOR_UNIQUEPOINTER String::cstr()
	{
		if (size() == INVALID_SIZE)
			return Net::Cryption::XOR_UNIQUEPOINTER(nullptr, NULL, false);

		return _string.revert();
	}

	Net::Cryption::XOR_UNIQUEPOINTER String::get()
	{
		if (size() == INVALID_SIZE)
			return Net::Cryption::XOR_UNIQUEPOINTER(nullptr, NULL, false);

		return _string.revert();
	}

	Net::Cryption::XOR_UNIQUEPOINTER String::revert()
	{
		if (size() == INVALID_SIZE)
			return Net::Cryption::XOR_UNIQUEPOINTER(nullptr, NULL, false);

		return _string.revert();
	}

	Net::Cryption::XOR_UNIQUEPOINTER String::data()
	{
		if (size() == INVALID_SIZE)
			return Net::Cryption::XOR_UNIQUEPOINTER(nullptr, NULL, false);

		return _string.revert();
	}

	void String::clear()
	{
		this->Destroy();
	}

	bool String::empty()
	{
		if (size() == INVALID_SIZE || size() == 0)
			return true;

		return (this->compare(CSTRING("")));
	}

	char String::at(const size_t i)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return '\0';

		if (i > size())
			return '\0';

		return this->operator[](i);
	}

	char* String::substr(size_t length)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return nullptr;

		if (length > size())
			length = size();

		const auto sub = ALLOC<char>(length + 1);
		memset(sub, NULL, length);
		for (size_t i = 0, j = 0; i < length; ++i, ++j)
		{
			const auto tmp = at(i);
			memcpy(&sub[j], &tmp, 1);
		}
		sub[length] = '\0';

		return sub;
	}

	char* String::substr(const size_t start, size_t length)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return nullptr;

		if (length > size())
			length = size();

		if (start + length > size())
			length = size() - start;

		const auto sub = ALLOC<char>(length + 1);
		memset(sub, NULL, length);
		for (size_t i = start, j = 0; i < (start + length); ++i, ++j)
		{
			const auto tmp = at(i);
			memcpy(&sub[j], &tmp, 1);
		}
		sub[length] = '\0';

		return sub;
	}

	size_t String::find(const char c, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		for (size_t i = 0; i < size(); ++i)
		{
			const auto tmp = type & NOT_CASE_SENS ? (char)tolower((int)at(i)) : at(i);
			const auto comp = type & NOT_CASE_SENS ? (char)tolower((int)c) : c;
			if (tmp == comp)
				return i;
		}

		return NET_STRING_NOT_FOUND;
	}

	size_t String::find(const char* pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		const auto patternLen = strlen(pattern);

		size_t it = 0;
		for (size_t i = 0; i < size(); ++i)
		{
			const auto tmp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)at(i)) : at(i);
			const auto comp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)pattern[it]) : pattern[it];
			if (tmp == comp)
			{
				it++;
				if (it == patternLen)
				{
					return i - it + 1;
				}
			}
			else
			{
				it = 0;
			}
		}

		return NET_STRING_NOT_FOUND;
	}

	size_t String::find(const size_t start, char character, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		if (start > size())
			return NET_STRING_NOT_FOUND;

		if (type & NOT_CASE_SENS)
		{
			character = (char)tolower((int)character);
		}

		for (auto i = start; i < size(); ++i)
		{
			const auto tmp = (char)tolower((int)at(i));
			if (tmp == character)
				return i;
		}

		return NET_STRING_NOT_FOUND;
	}

	size_t String::find(const size_t start, const char* pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		if (start > size())
			return NET_STRING_NOT_FOUND;

		const auto patternLen = strlen(pattern);

		size_t it = 0;
		for (auto i = start; i < size(); ++i)
		{
			const auto tmp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)at(i)) : at(i);
			const auto comp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)pattern[it]) : pattern[it];
			if (tmp == comp)
			{
				it++;
				if (it == patternLen)
				{
					return i - it + 1;
				}
			}
			else
			{
				it = 0;
			}
		}

		return NET_STRING_NOT_FOUND;
	}

	std::vector<size_t> String::findAll(const char c, const char type)
	{
		std::vector<size_t> tmp;

		if (size() == INVALID_SIZE || size() == 0)
			return tmp;

		for (size_t i = 0; i < size(); ++i)
		{
			const auto res = find(i, c, type);
			if (res != NET_STRING_NOT_FOUND)
			{
				tmp.emplace_back(res);
				i = res;
			}
		}

		return tmp;
	}

	std::vector<size_t> String::findAll(const char* pattern, const char type)
	{
		std::vector<size_t> tmp;

		if (size() == INVALID_SIZE || size() == 0)
			return tmp;

		for (size_t i = 0; i < size(); ++i)
		{
			const auto res = find(i, pattern, type);
			if (res != NET_STRING_NOT_FOUND)
			{
				tmp.emplace_back(res);
				i = res;
			}
		}

		return tmp;
	}

	std::vector<size_t> String::findAll(size_t start, const char c, const char type)
	{
		std::vector<size_t> tmp;

		if (size() == INVALID_SIZE || size() == 0)
			return tmp;

		if (start > size())
			return tmp;

		for (size_t i = start; i < size(); ++i)
		{
			const auto res = find(i, c, type);
			if (res != NET_STRING_NOT_FOUND)
			{
				tmp.emplace_back(res);
				i = res;
			}
		}

		return tmp;
	}

	std::vector<size_t> String::findAll(size_t start, const char* pattern, const char type)
	{
		std::vector<size_t> tmp;

		if (size() == INVALID_SIZE || size() == 0)
			return tmp;

		if (start > size())
			return tmp;

		for (size_t i = start; i < size(); ++i)
		{
			const auto res = find(i, pattern, type);
			if (res != NET_STRING_NOT_FOUND)
			{
				tmp.emplace_back(res);
				i = res;
			}
		}

		return tmp;
	}

	size_t String::findLastOf(const char c, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		for (size_t i = size() - 1; i > 0; --i)
		{
			const auto tmp = type & NOT_CASE_SENS ? (char)tolower((int)at(i)) : at(i);
			const auto comp = type & NOT_CASE_SENS ? (char)tolower((int)c) : c;
			if (tmp == comp)
				return i;
		}

		return NET_STRING_NOT_FOUND;
	}

	size_t String::findLastOf(const char* pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return NET_STRING_NOT_FOUND;

		const auto patternLen = strlen(pattern);

		size_t it = patternLen - 1;
		for (size_t i = size() - 1; i > 0; --i)
		{
			const auto tmp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)at(i)) : at(i);
			const auto comp = (type & Net::String::type::NOT_CASE_SENS) ? (char)tolower((int)pattern[it]) : pattern[it];
			if (tmp == comp)
			{
				if (it == 0)
					it = INVALID_SIZE;
				else
					it--;

				if (it == INVALID_SIZE)
				{
					return i;
				}
			}
			else
			{
				it = patternLen - 1;
			}
		}

		return NET_STRING_NOT_FOUND;
	}

	bool String::compare(char match, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		if (size() > 1)
			return false;

		if (type & NOT_CASE_SENS)
			return (this->operator[](0) == match);

		return ((char)tolower(this->operator[](0)) == (char)tolower(match));
	}

	bool String::compare(const char* match, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		auto len = strlen(match);

		if (this->size() != len)
			return false;

		for (size_t i = 0; i < len; ++i)
		{
			if (type & NOT_CASE_SENS)
			{
				if ((char)tolower(this->operator[](i)) != (char)tolower(match[i]))
					return false;
			}
			else
			{
				if (this->operator[](i) != match[i])
					return false;
			}
		}

		return true;
	}

	bool Net::String::compare(const String& in, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		auto cast = const_cast<String*>(&in);

		auto len = cast->size();

		if (this->size() != len)
			return false;

		for (size_t i = 0; i < len; ++i)
		{
			if (type & NOT_CASE_SENS)
			{
				if ((char)tolower(this->operator[](i)) != (char)tolower(cast->operator[](i)))
					return false;
			}
			else
			{
				if (this->operator[](i) != cast->operator[](i))
					return false;
			}
		}

		return true;
	}

	bool String::erase(const size_t len)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		if (len > size())
		{
			clear();
			return true;
		}

		/*
		* instead of allocating a new buffer
		* we use the method to move the characters up
		* and repos the null-terminator
		*/
		char prevChar = at(size() - 1);
		for (size_t i = 0; i < len; ++i)
		{
			for (size_t j = size() - 1; j > 0; --j)
			{
				char rem = at(j - 1);
				_string.set(j - 1, prevChar);
				prevChar = rem;
			}
		}
		_string.set(size() - len, '\0');
		_string.set_size(size() - len);

		/*
		* ok if we removed then we add the amount to _free_size
		* later we will use it for the append function
		*/
		if (_free_size == INVALID_SIZE)
		{
			_free_size = len;
		}
		else
		{
			_free_size += len;
		}

		return true;
	}

	bool String::erase(const size_t start, size_t len)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		if (start + len > size())
		{
			clear();
			return true;
		}

		/*
		* instead of allocating a new buffer
		* we use the method to move the characters up
		* and repos the null-terminator
		*/
		char prevChar = at(size() - 1);
		for (size_t i = 0; i < len; ++i)
		{
			for (size_t j = size() - 1; j > start; --j)
			{
				char rem = at(j - 1);
				_string.set(j - 1, prevChar);
				prevChar = rem;
			}
		}
		_string.set(size() - len, '\0');
		_string.set_size(size() - len);

		/*
		* ok if we removed then we add the amount to _free_size
		* later we will use it for the append function
		*/
		if (_free_size == INVALID_SIZE)
		{
			_free_size = len;
		}
		else
		{
			_free_size += len;
		}

		return true;
	}

	bool String::erase(const char c, const size_t start)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, c);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, 0);
	}

	bool String::erase(const char c, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(c, type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, 0);
	}

	bool String::erase(const char c, const size_t start, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, c, type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, 0);
	}

	bool String::erase(const char* pattern, const size_t start)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, pattern);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, strlen(pattern));
	}

	bool String::erase(const char* pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(pattern, type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, strlen(pattern));
	}

	bool String::erase(const char* pattern, const size_t start, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, pattern, type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, strlen(pattern));
	}


	bool String::erase(String& pattern, const size_t start)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, pattern.get().data());
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, pattern.length());
	}

	bool String::erase(String& pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(pattern.get().data(), type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, pattern.length());
	}

	bool String::erase(String& pattern, const size_t start, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		const auto it = find(start, pattern.get().data(), type);
		if (it == NET_STRING_NOT_FOUND)
			return false;

		return erase(it, pattern.length());
	}

	bool String::eraseAll(const char c, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		auto match = findAll(c, type);
		return eraseAll(match);
	}

	bool String::eraseAll(const char* pattern, const char type)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		auto match = findAll(pattern, type);
		return eraseAll(match);
	}

	bool Net::String::eraseAll(std::vector<size_t> m_IndexCharacterToSkip)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return false;

		auto replaceSize = size() - m_IndexCharacterToSkip.size();
		auto replace = ALLOC<char>(replaceSize + 1);
		memset(replace, 0, replaceSize);
		replace[replaceSize] = 0;

		auto ref = get();
		auto dec = ref.get();

		size_t j = 0;
		for (size_t i = 0; i < size(); ++i)
		{
			bool m_skip = false;
			for (auto& index : m_IndexCharacterToSkip)
			{
				if (i == index)
				{
					m_skip = true;
					break;
				}
			}
			if (m_skip) continue;

			replace[j] = dec[i];
			++j;
		}

		this->Destroy();
		_string = RUNTIMEXOR(replace, true);
		return true;
	}

	bool String::replace(const char c, const char r, const size_t start)
	{
		const auto i = find(start, c);
		if (i == NET_STRING_NOT_FOUND)
			return false;

		_string.set(i, r);
		return true;
	}

	bool String::replace(const char c, const char* r, const size_t start)
	{
		const auto i = find(start, c);
		if (i == NET_STRING_NOT_FOUND)
			return false;

		const auto str = revert();
		const auto rSize = strlen(r);
		const auto replaceSize = size() + rSize - 1;
		const auto replace = ALLOC<char>(replaceSize + 1);
		memcpy(replace, str.get(), size());
		memcpy(&replace[i], r, rSize);
		memcpy(&replace[i + rSize], &str.get()[i + 1], size() - i - 1);

		this->Destroy();
		_string = RUNTIMEXOR(replace, true);
		return true;
	}

	bool String::replace(const char* pattern, const char r, const size_t start)
	{
		const auto i = find(start, pattern);
		if (i == NET_STRING_NOT_FOUND)
			return false;

		const auto patternLen = strlen(pattern);
		const auto replaceSize = size() + 1;
		const auto replace = ALLOC<char>(replaceSize + 1);

		auto cinserted = false;
		size_t j = 0;
		for (size_t it = 0; it < size(); ++it)
		{
			if (it >= i && it < i + patternLen)
			{
				if (!cinserted)
				{
					memcpy(&replace[j], &r, 1);
					++j;
					cinserted = true;
				}
				continue;
			}

			auto tmp = at(it);
			memcpy(&replace[j], &tmp, 1);
			++j;
		}
		replace[replaceSize] = '\0';

		this->Destroy();
		_string = RUNTIMEXOR(replace, true);
		return true;
	}

	bool String::replace(const char* pattern, const char* r, const size_t start)
	{
		const auto i = find(start, pattern);
		if (i == NET_STRING_NOT_FOUND)
			return false;

		const auto patternLen = strlen(pattern);
		const auto rLen = strlen(r);
		const auto replaceSize = size() + rLen;
		const auto replace = ALLOC<char>(replaceSize + 1);

		auto cinserted = false;
		size_t j = 0;
		for (size_t it = 0; it < size(); ++it)
		{
			if (it >= i && it < i + patternLen)
			{
				if (!cinserted)
				{
					for (size_t x = 0; x < rLen; ++x)
					{
						memcpy(&replace[j], &r[x], 1);
						++j;
					}

					cinserted = true;
				}
				continue;
			}

			auto tmp = at(it);
			memcpy(&replace[j], &tmp, 1);
			++j;
		}
		replace[replaceSize] = '\0';

		this->Destroy();
		_string = RUNTIMEXOR(replace, true);
		return true;
	}

	bool String::replaceAll(const char c, const char r)
	{
		const auto found = findAll(c);
		if (found.empty())
			return false;

		for (auto& index : found)
			_string.set(index, r);

		return true;
	}

	bool String::replaceAll(const char c, const char* r)
	{
		const auto found = findAll(c);
		if (found.empty())
			return false;

		auto affectedOnce = false;
		for (size_t i = 0; i < size(); ++i)
		{
			const auto res = replace(c, r, i);
			if (!affectedOnce && res)
				affectedOnce = true;
		}

		return affectedOnce;
	}

	bool String::replaceAll(const char* pattern, const char r)
	{
		const auto found = findAll(pattern);
		if (found.empty())
			return false;

		auto affectedOnce = false;
		for (size_t i = 0; i < size(); ++i)
		{
			const auto res = replace(pattern, r, i);
			if (!affectedOnce && res)
				affectedOnce = true;
		}

		return affectedOnce;
	}

	bool String::replaceAll(const char* pattern, const char* r)
	{
		const auto found = findAll(pattern);
		if (found.empty())
			return false;

		auto affectedOnce = false;
		for (size_t i = 0; i < size(); ++i)
		{
			const auto res = replace(pattern, r, i);
			if (!affectedOnce && res)
				affectedOnce = true;
		}

		return affectedOnce;
	}

	ViewString Net::String::view_string(size_t m_start, size_t m_size)
	{
		if (size() == INVALID_SIZE || size() == 0)
			return {};

		/*
		* if m_size is zero
		* then we return the entire size of the string
		*/
        auto m_ref = this->get();

		if (m_size == 0)
			return { this, &m_ref, m_start, size() };

		return { this, &m_ref, m_start, m_size };
	}

	std::ostream& operator<<(std::ostream& os, const Net::ViewString& vs)
	{
		/*
		* better iterate through it and append each character
		* rather than heap allocating another space for the decrypted text
		*/
		for (size_t i = vs.start(); i < vs.end(); ++i)
		{
			os << const_cast<Net::ViewString*>(&vs)->operator[](i);
		}

		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Net::String& ns)
	{
		/*
		* better iterate through it and append each character
		* rather than heap allocating another space for the decrypted text
		*/
		for (size_t i = 0; i < ns.size(); ++i)
		{
			os << const_cast<Net::String*>(&ns)->operator[](i);
		}

		return os;
	}
}
NET_POP