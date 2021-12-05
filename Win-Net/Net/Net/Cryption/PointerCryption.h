#pragma once
#define CPOINTER Net::Cryption::Pointer

#include <Net/Net/Net.h>

NET_DSA_BEGIN

#define RAND_NUMBER 1337

NET_NAMESPACE_BEGIN(Net)
NET_NAMESPACE_BEGIN(Cryption)
template <typename T>
class Cryption
{
protected:
	uintptr_t _key;

public:
	T* encode(T* pointer)
	{
		pointer = (T*)((uintptr_t)pointer ^ (uintptr_t)_key);
		return pointer;
	}

	T* encode(const T*& pointer)
	{
		pointer = (T*)((uintptr_t)pointer ^ (uintptr_t)_key);
		return pointer;
	}

	T* decode(T* pointer) const
	{
		pointer = (T*)((uintptr_t)pointer ^ (uintptr_t)_key);
		return pointer;
	}

	T*& decodeRef(T*& pointer)
	{
		pointer = (T*)((uintptr_t)pointer ^ (uintptr_t)_key);
		return pointer;
	}
};

template <typename T>
class UniquePointer : public Cryption<T>
{
	T** _pointer;

public:
	explicit UniquePointer(T** pointer, const uintptr_t key)
	{
		this->_key = key;
		_pointer = pointer;
	}

	~UniquePointer()
	{
		if (*_pointer) *_pointer = this->encode(*_pointer);
	}

	T*& get()
	{
		return this->decodeRef(*_pointer);
	}
};

template <typename T>
class Pointer : public Cryption<T>
{
	T* _pointer;

public:
	Pointer()
	{
		this->_key = 0;
		_pointer = nullptr;
	}

	explicit Pointer(const T*& pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
	}

	explicit Pointer(T*&& pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
	}

	Pointer& operator=(T* pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
		return *this;
	}

	Pointer& operator=(const T*& pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
		return *this;
	}

	void Set(T* pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
	}

	void Set(const T*& pointer)
	{
		this->_key = pointer == nullptr ? 0 : RAND_NUMBER;
		_pointer = pointer == nullptr ? nullptr : this->encode(pointer);
	}

	T value() const { return *this->decode(_pointer); }
	T value() { return *this->decode(_pointer); }
	T* get() const { return this->decode(_pointer); }
	T* get() { return this->decode(_pointer); }
	UniquePointer<T> ref() { return UniquePointer<T>(&_pointer, this->_key); }
	UniquePointer<T> reference() { return ref(); }

	bool valid() const { return (_pointer != nullptr); }
	bool valid() { return (_pointer != nullptr); }

	void free()
	{
		if (_pointer == nullptr) return;
		_pointer = this->decode(_pointer);
		FREE(_pointer);
		_pointer = nullptr;
		this->_key = 0;
	}
};
NET_NAMESPACE_END
NET_NAMESPACE_END
NET_DSA_END
