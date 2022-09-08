/*
	Author: Tobias Staack
*/
#include <Net/Net/NetJson.h>
#include <Net/Cryption/XOR.h>

int Net::Json::Convert::ToInt32(Net::String& str)
{
	return std::stoi(str.get().get());
}

float Net::Json::Convert::ToFloat(Net::String& str)
{
	return std::stof(str.get().get());
}

double Net::Json::Convert::ToDouble(Net::String& str)
{
	return std::stod(str.get().get());
}

bool Net::Json::Convert::ToBoolean(Net::String& str)
{
	if (!strcmp(CSTRING("true"), str.get().get()))
		return true;
	else if (!strcmp(CSTRING("false"), str.get().get()))
		return false;

	/* todo: throw error */
	return false;
}

bool Net::Json::Convert::is_float(Net::String& str)
{
	char* end = nullptr;
	double val = strtof(str.get().get(), &end);
	return end != str.get().get() && *end == '\0' && val != HUGE_VALF;
}

bool Net::Json::Convert::is_double(Net::String& str)
{
	char* end = nullptr;
	double val = strtod(str.get().get(), &end);
	return end != str.get().get() && *end == '\0' && val != HUGE_VAL;
}

bool Net::Json::Convert::is_boolean(Net::String& str)
{
	if (!strcmp(CSTRING("true"), str.get().get()))
		return true;
	else if (!strcmp(CSTRING("false"), str.get().get()))
		return true;

	return false;
}

Net::Json::BasicObject::BasicObject(bool bSharedMemory)
{
	this->m_type = Type::OBJECT;
	this->value = {};
	this->bSharedMemory = bSharedMemory;
}

Net::Json::BasicObject::~BasicObject()
{
	this->m_type = Type::OBJECT;
	this->value = {};
	this->bSharedMemory = bSharedMemory;
}

void Net::Json::BasicObject::__push(void* ptr)
{
	value.push_back(ptr);
}

vector<void*> Net::Json::BasicObject::Value()
{
	return this->value;
}

void Net::Json::BasicObject::Set(vector<void*> value)
{
	this->value = value;
}

Net::Json::BasicArray::BasicArray(bool bSharedMemory)
{
	this->m_type = Type::ARRAY;
	this->value = {};
	this->bSharedMemory = bSharedMemory;
}

Net::Json::BasicArray::~BasicArray()
{
	this->m_type = Type::ARRAY;
	this->value = {};
	this->bSharedMemory = false;
}

void Net::Json::BasicArray::__push(void* ptr)
{
	value.push_back(ptr);
}

vector<void*> Net::Json::BasicArray::Value()
{
	return this->value;
}

void Net::Json::BasicArray::Set(vector<void*> value)
{
	this->value = value;
}

template <typename T>
Net::Json::BasicValue<T>::BasicValue()
{
	this->value = {};
	this->key = nullptr;
	this->m_type = Type::NULL;
}

template <typename T>
Net::Json::BasicValue<T>::BasicValue(const char* key, T value, Net::Json::Type type)
{
	this->SetKey(key);
	this->SetValue(value, type);
}

template <typename T>
Net::Json::BasicValue<T>::~BasicValue()
{
	/* free string from heap */
	if (this->m_type == Type::STRING)
	{
		auto cast = (BasicValue<char*>*)this;
		if (cast
			&& cast->Type() == Type::STRING
			&& cast->Value())
		{
			delete[] cast->Value();
			cast->Value() = nullptr;
		}
	}

	this->value = {};

	if (this->key)
	{
		delete[] this->key;
		this->key = nullptr;
	}

	this->m_type = Type::NULL;
}

template <typename T>
void Net::Json::BasicValue<T>::operator=(const int& value)
{
	((BasicValue<int>*)this)->SetValue(value, Type::INTEGER);
}

template <typename T>
void Net::Json::BasicValue<T>::operator=(const float& value)
{
	((BasicValue<float>*)this)->SetValue(value, Type::FLOAT);
}

template <typename T>
void Net::Json::BasicValue<T>::operator=(const double& value)
{
	((BasicValue<double>*)this)->SetValue(value, Type::DOUBLE);
}

template <typename T>
void Net::Json::BasicValue<T>::operator=(const BasicObject& value)
{
	((BasicValue<BasicObject>*)this)->SetValue(value, Type::OBJECT);
}

template <typename T>
void Net::Json::BasicValue<T>::SetKey(const char* key)
{
	auto len = strlen(key);
	this->key = new char[len + 1];
	if (!this->key) return;
	memcpy(this->key, key, len);
	this->key[len] = 0;
}

template <typename T>
void Net::Json::BasicValue<T>::SetValue(T value, Net::Json::Type type)
{
	this->value = value;
	this->m_type = type;
}

template <typename T>
void Net::Json::BasicValue<T>::SetType(Net::Json::Type type)
{
	this->m_type = type;
}


template <typename T>
char* Net::Json::BasicValue<T>::Key()
{
	return this->key;
}

template <typename T>
T& Net::Json::BasicValue<T>::Value()
{
	return this->value;
}

template <typename T>
Net::Json::Type Net::Json::BasicValue<T>::Type()
{
	return this->m_type;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_null()
{
	return Type() == Type::NULL;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_object()
{
	return Type() == Type::OBJECT;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_array()
{
	return Type() == Type::ARRAY;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_integer()
{
	return Type() == Type::INTEGER;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_int()
{
	return Type() == Type::INTEGER;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_float()
{
	return Type() == Type::FLOAT;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_double()
{
	return Type() == Type::DOUBLE;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_boolean()
{
	return Type() == Type::BOOLEAN;
}

template <typename T>
bool Net::Json::BasicValue<T>::is_string()
{
	return Type() == Type::STRING;
}

template <typename T>
Net::Json::Object* Net::Json::BasicValue<T>::as_object()
{
	if (!is_object()) return {};
	return (Object*)&((BasicValue<BasicObject>*)this)->Value();
}

template <typename T>
Net::Json::Array* Net::Json::BasicValue<T>::as_array()
{
	if (!is_array()) return {};
	return (Array*)&((BasicValue<BasicArray>*)this)->Value();
}

template <typename T>
int Net::Json::BasicValue<T>::as_int()
{
	if (!is_integer()) return 0;
	return ((BasicValue<int>*)this)->Value();
}

template <typename T>
float Net::Json::BasicValue<T>::as_float()
{
	if (!is_float()) return 0.0f;
	return ((BasicValue<float>*)this)->Value();
}

template <typename T>
double Net::Json::BasicValue<T>::as_double()
{
	if (!is_double()) return 0;
	return ((BasicValue<double>*)this)->Value();
}

template <typename T>
bool Net::Json::BasicValue<T>::as_boolean()
{
	if (!is_boolean()) return false;
	return ((BasicValue<bool>*)this)->Value();
}

template <typename T>
char* Net::Json::BasicValue<T>::as_string()
{
	if (!is_string()) return (char*)"";
	return ((BasicValue<char*>*)this)->Value();
}

Net::Json::NullValue::NullValue()
{
	this->SetType(Type::NULL);
}

Net::Json::NullValue::NullValue(int i)
{
	this->SetType(Type::NULL);
}

Net::Json::BasicValueRead::BasicValueRead(void* ptr)
{
	this->ptr = ptr;
}

Net::Json::BasicValue<Net::Json::Object>* Net::Json::BasicValueRead::operator->() const
{
	return (BasicValue<Object>*)this->ptr;
}

static Net::Json::BasicValueRead object_to_BasicValueRead(void* ptr, const char* key)
{
	if (!ptr) return { nullptr };
	auto cast = (Net::Json::BasicValueRead*)ptr;
	if (!cast) return { nullptr };
	auto cast2 = (Net::Json::BasicValue<Net::Json::Object>*)cast->operator->();
	if (!cast2) return { nullptr };
	if (cast2->Type() != Net::Json::Type::OBJECT) return { nullptr };
	return cast2->Value()[key];
}

static Net::Json::BasicValueRead object_to_BasicValueRead(void* ptr, int idx)
{
	if (!ptr) return { nullptr };
	auto cast = (Net::Json::BasicValueRead*)ptr;
	if (!cast) return { nullptr };
	auto cast2 = (Net::Json::BasicValue<Net::Json::Array>*)cast->operator->();
	if (!cast2) return { nullptr };
	if (cast2->Type() != Net::Json::Type::ARRAY) return { nullptr };
	return cast2->Value()[idx];
}

Net::Json::BasicValueRead Net::Json::BasicValueRead::operator[](const char* key)
{
	return object_to_BasicValueRead(this, key);
}

Net::Json::BasicValueRead Net::Json::BasicValueRead::operator[](char* key)
{
	return object_to_BasicValueRead(this, key);
}

Net::Json::BasicValueRead Net::Json::BasicValueRead::operator[](int idx)
{
	return object_to_BasicValueRead(this, idx);
}

Net::Json::BasicValueRead::operator bool()
{
	if (!this->ptr) return false;

	auto cast = (Net::Json::BasicValueRead*)ptr;
	if (!cast) return false;

	return true;
}

void Net::Json::BasicValueRead::operator=(const NullValue& value)
{
	if (!this->ptr) return;
	((BasicValue<NullValue>*)this->ptr)->SetValue(value, Type::NULL);
}

void Net::Json::BasicValueRead::operator=(const int& value)
{
	if (!this->ptr) return;
	((BasicValue<int>*)this->ptr)->SetValue(value, Type::INTEGER);
}

void Net::Json::BasicValueRead::operator=(const float& value)
{
	if (!this->ptr) return;
	((BasicValue<float>*)this->ptr)->SetValue(value, Type::FLOAT);
}

void Net::Json::BasicValueRead::operator=(const double& value)
{
	if (!this->ptr) return;
	((BasicValue<double>*)this->ptr)->SetValue(value, Type::DOUBLE);
}

void Net::Json::BasicValueRead::operator=(const bool& value)
{
	if (!this->ptr) return;
	((BasicValue<bool>*)this->ptr)->SetValue(value, Type::BOOLEAN);
}

void Net::Json::BasicValueRead::operator=(const char* value)
{
	if (!this->ptr) return;

	size_t len = strlen(value);
	char* ptr = new char[len + 1];
	memcpy(ptr, value, len);
	ptr[len] = 0;

	auto cast = ((BasicValue<char*>*)this->ptr);
	if (cast->Type() == Type::STRING
		&& cast->Value())
	{
		delete[] cast->Value();
		cast->Value() = nullptr;
	}
	cast->SetValue(ptr, Type::STRING);
}

void Net::Json::BasicValueRead::operator=(BasicObject& value)
{
	if (!this->ptr) return;
	((BasicValue<BasicObject>*)this->ptr)->SetValue(value, Type::OBJECT);
}

void Net::Json::BasicValueRead::operator=(BasicArray& value)
{
	if (!this->ptr) return;
	((BasicValue<BasicArray>*)this->ptr)->SetValue(value, Type::ARRAY);
}

Net::Json::Object::Object(bool bSharedMemory)
	: Net::Json::BasicObject::BasicObject(bSharedMemory)
{
}

Net::Json::Object::~Object()
{
	if (!bSharedMemory) this->Free();
}

void Net::Json::Object::Free()
{
	this->value.clear();
}

template<typename T>
bool Net::Json::Object::__append(const char* key, T value, Type type)
{
	BasicValue<T>* heap = new BasicValue<T>();
	if (!heap) return false;
	heap->SetKey(key);
	heap->SetValue(value, type);
	this->__push(heap);
	return true;
}

template <typename T>
Net::Json::BasicValue<T>* Net::Json::Object::__get(const char* key)
{
	for (size_t i = 0; i < value.size(); ++i)
	{
		BasicValue<T>* tmp = (BasicValue<T>*)value[i];
		if (strcmp(tmp->Key(), key) != 0) continue;
		return (BasicValue<T>*)value[i];
	}

	return nullptr;
}

Net::Json::BasicValueRead Net::Json::Object::operator[](const char* key)
{
	return this->At(key);
}

Net::Json::BasicValueRead Net::Json::Object::At(const char* key)
{
	auto ptr = this->__get<Object>(key);
	if (!ptr)
	{
		BasicValue<Object>* heap = new BasicValue<Object>();
		if (!heap) return { nullptr };
		heap->SetType(Type::OBJECT);
		heap->SetKey(key);
		this->__push(heap);
		return { heap };
	}

	return { ptr };
}

template<typename T>
Net::Json::BasicValue<T>* Net::Json::Object::operator=(BasicValue<T>* value)
{
	this->__push(value);
	return;
}

bool Net::Json::Object::Append(const char* key, int value)
{
	return __append(key, value, Type::INTEGER);
}

bool Net::Json::Object::Append(const char* key, float value)
{
	return __append(key, value, Type::FLOAT);
}

bool Net::Json::Object::Append(const char* key, double value)
{
	return __append(key, value, Type::DOUBLE);
}

bool Net::Json::Object::Append(const char* key, bool value)
{
	return __append(key, value, Type::BOOLEAN);
}

bool Net::Json::Object::Append(const char* key, const char* value)
{
	size_t len = strlen(value);
	char* ptr = new char[len + 1];
	memcpy(ptr, value, len);
	ptr[len] = 0;
	if (!__append(key, ptr, Type::STRING))
	{
		delete[] ptr;
		ptr = nullptr;
		return false;
	}

	return true;
}

bool Net::Json::Object::Append(const char* key, Object value)
{
	return __append(key, value, Type::OBJECT);
}

Net::String Net::Json::Object::Serialize(SerializeType type, size_t iterations)
{
	Net::String out(CSTRING(""));

	out += (type == SerializeType::FORMATTED ? CSTRING("{\n") : CSTRING("{"));

	for (size_t i = 0; i < value.size(); ++i)
	{
		auto tmp = (BasicValue<void*>*)value[i];
		if (tmp->Type() == Type::NULL)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : %s)"), tmp->Key(), CSTRING("null"));
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::STRING)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : "%s")"), tmp->Key(), tmp->as_string());
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::INTEGER)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : %i)"), tmp->Key(), tmp->as_int());
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::FLOAT)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : %F)"), tmp->Key(), tmp->as_float());
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::DOUBLE)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : %F)"), tmp->Key(), tmp->as_double());
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::BOOLEAN)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : %s)"), tmp->Key(), tmp->as_boolean() ? CSTRING("true") : CSTRING("false"));
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::OBJECT)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : )"), tmp->Key());
			out += tmp->as_object()->Serialize(type, (iterations + 1));
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::ARRAY)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out.append(CSTRING(R"("%s" : )"), tmp->Key());
			out += tmp->as_array()->Serialize(type, (iterations + 1));
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
	}

	size_t it = 0;
	if ((it = out.findLastOf(CSTRING(","))) != NET_STRING_NOT_FOUND)
		out.erase(it, 1);

	if (type == SerializeType::FORMATTED) out += CSTRING("\n");

	if (type == SerializeType::FORMATTED)
	{
		for (size_t it = 0; it < iterations; ++it)
		{
			out += CSTRING("\t");
		}
	}

	out += CSTRING("}");

	return out;
}

Net::String Net::Json::Object::Stringify(SerializeType type, size_t iterations)
{
	return this->Serialize(type, iterations);
}

/* wrapper */
bool Net::Json::Object::Deserialize(Net::String json)
{
	vector<char*> object_chain = {};
	auto ret = this->Deserialize(json, object_chain);
	for (size_t i = 0; i < object_chain.size(); ++i)
	{
		if (!object_chain[i]) continue;
		delete[] object_chain[i];
		object_chain[i] = nullptr;
	}
	return ret;
}

bool Net::Json::Object::Parse(Net::String json)
{
	return this->Deserialize(json);
}

/* actual deserialization */
bool Net::Json::Object::Deserialize(Net::String json, vector<char*>& object_chain)
{
	auto plain = json.get().get();
	if (memcmp(&plain[0], CSTRING("{"), 1) != 0)
	{
		/* not an object */
		return false;
	}

	size_t k = 0;
	size_t v = 0;
	size_t t = 0;
	size_t p = 0;
	Net::String lastKey(CSTRING(""));
	Net::String lastValue(CSTRING(""));
	bool bReadValue = false;
	bool bReadAnotherObject = false;
	bool bReadArray = false;
	for (size_t i = 1; i < json.size(); ++i)
	{
		/* beginning of a key */
		if (!bReadAnotherObject
			&& !bReadArray
			&& !bReadValue
			&& !memcmp(&plain[i], CSTRING(R"(")"), 1))
		{
			if (k != 0)
			{
				lastKey = json.substr(k + 1, i - k - 1);
				k = 0;
				continue;
			}

			k = i;
		}
		else if (!bReadAnotherObject
			&& !bReadArray
			&& !memcmp(&plain[i], CSTRING(":"), 1))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			v = i;
			bReadValue = true;
			bReadAnotherObject = false;
			bReadArray = false;
		}
		else if (!bReadAnotherObject
			&& !bReadArray
			&& !memcmp(&plain[i], CSTRING("{"), 1))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			/* another object */
			v = i;
			bReadAnotherObject = true;
			bReadValue = false;
			bReadArray = false;
		}
		else if (bReadAnotherObject
			&& !bReadArray
			&& !memcmp(&plain[i], CSTRING("{"), 1))
		{
			++p;
		}
		else if (bReadAnotherObject
			&& !bReadArray
			&& !memcmp(&plain[i], CSTRING("}"), 1))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			if (t < p)
			{
				++t;
				continue;
			}

			lastValue = json.substr(v, i - v + 1);

			object_chain.push_back(_strdup(lastKey.get().get()));

			if (!this->Deserialize(lastValue, object_chain))
				return false;

			object_chain.erase(object_chain.size() - 1);

			bReadAnotherObject = false;
			t = 0;
			p = 0;
		}
		else if (!bReadArray
			&& !bReadAnotherObject
			&& !memcmp(&plain[i], CSTRING("["), 1))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			/* reading array */
			v = i;
			bReadArray = true;
			bReadAnotherObject = false;
			bReadValue = false;
		}
		else if (bReadArray
			&& !bReadAnotherObject
			&& !memcmp(&plain[i], CSTRING("]"), 1))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			lastValue = json.substr(v, i - v + 1);

			Net::Json::Array arr(true);
			if (!arr.Deserialize(lastValue))
			{
				/* error */
				return false;
			}

			Net::Json::BasicValueRead obj(nullptr);
			if (object_chain.size() > 0)
			{
				obj = { this->operator[](object_chain[0]) };
				for (size_t i = 1; i < object_chain.size(); ++i)
				{
					obj = obj[object_chain[i]];
				}

				obj[lastKey.get().get()] = arr;
			}
			else
			{
				this->operator[](lastKey.get().get()) = arr;
			}

			bReadArray = false;
		}
		else if (!bReadAnotherObject && !bReadArray && bReadValue && (!memcmp(&plain[i], CSTRING(","), 1) || !memcmp(&plain[i], CSTRING("}"), 1)))
		{
			/* reading key */
			if (k != 0)
			{
				/* error */
				return false;
			}

			lastValue = json.substr(v + 1, i - v - 1);

			size_t z = 0;

			Net::Json::Type type = Net::Json::Type::INTEGER;
			for (size_t j = 0; j < lastValue.length(); ++j)
			{
				if (!memcmp(&lastValue.get().get()[j], CSTRING(R"(")"), 1))
				{
					if (type == Net::Json::Type::STRING)
					{
						lastValue = lastValue.substr(z + 1, j - z - 1);
						break;
					}

					type = Net::Json::Type::STRING;
					z = j;
					continue;
				}
			}

			if (type != Net::Json::Type::STRING)
			{
				/* remove all whitespaces, breaks and tabulators */
				lastValue.eraseAll(CSTRING("\n"));
				lastValue.eraseAll(CSTRING("\t"));
				lastValue.eraseAll(CSTRING(" "));
			}

			if (type != Net::Json::Type::STRING)
			{
				/* check for boolean */
				if (Convert::is_boolean(lastValue))
				{
					type = Net::Json::Type::BOOLEAN;
				}
				else
				{
					for (size_t j = 0; j < lastValue.length(); ++j)
					{
						if (!memcmp(&lastValue.get().get()[j], CSTRING("."), 1))
						{
							/* check if is float or a double */
							if (Convert::is_float(lastValue))
							{
								type = Net::Json::Type::FLOAT;
							}
							else if (Convert::is_double(lastValue))
							{
								type = Net::Json::Type::DOUBLE;
							}
							else
							{
								/* error */
								DebugBreak();
								return false;
							}

							break;
						}

						if (!memcmp(&lastValue.get().get()[j], CSTRING("null"), 4))
						{
							type = Net::Json::Type::NULL;
							break;
						}
					}
				}
			}

			Net::Json::BasicValueRead obj(nullptr);
			if (object_chain.size() > 0)
			{
				obj = { this->operator[](object_chain[0]) };
				for (size_t i = 1; i < object_chain.size(); ++i)
				{
					obj = obj[object_chain[i]];
				}
			}

			switch (type)
			{
			case Net::Json::Type::STRING:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = lastValue.get().get();
					break;
				}

				this->operator[](lastKey.get().get()) = lastValue.get().get();
				break;

			case Net::Json::Type::INTEGER:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = Convert::ToInt32(lastValue);
					break;
				}

				this->operator[](lastKey.get().get()) = Convert::ToInt32(lastValue);
				break;

			case Net::Json::Type::FLOAT:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = Convert::ToFloat(lastValue);
					break;
				}

				this->operator[](lastKey.get().get()) = Convert::ToFloat(lastValue);
				break;

			case Net::Json::Type::DOUBLE:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = Convert::ToDouble(lastValue);
					break;
				}

				this->operator[](lastKey.get().get()) = Convert::ToDouble(lastValue);
				break;

			case Net::Json::Type::BOOLEAN:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = Convert::ToBoolean(lastValue);
					break;
				}

				this->operator[](lastKey.get().get()) = Convert::ToBoolean(lastValue);
				break;

			case Net::Json::Type::NULL:
				if (object_chain.size() > 0)
				{
					obj[lastKey.get().get()] = Net::Json::NullValue();
					break;
				}

				this->operator[](lastKey.get().get()) = Net::Json::NullValue();
				break;

			default:
				DebugBreak();
				return false;
			}

			bReadValue = false;
			v = 0;
		}
	}

	return true;
}

Net::Json::Array::Array(bool bSharedMemory)
	: Net::Json::BasicArray::BasicArray(bSharedMemory)
{
}

Net::Json::Array::~Array()
{
	if (!bSharedMemory) this->Free();
}

void Net::Json::Array::Free()
{
	this->value.clear();
}

template <typename T>
bool Net::Json::Array::emplace_back(T value, Type type)
{
	BasicValue<T>* heap = new BasicValue<T>();
	if (!heap) return false;
	heap->SetValue(value, type);
	this->__push(heap);
	return true;
}

Net::Json::BasicValueRead Net::Json::Array::operator[](int idx)
{
	if (idx >= value.size()) return { nullptr };
	return { this->value[idx] };
}

Net::Json::BasicValueRead Net::Json::Array::at(int idx)
{
	return this->operator[](idx);
}

bool Net::Json::Array::push(int value)
{
	return this->emplace_back(value, Type::INTEGER);
}

bool Net::Json::Array::push(float value)
{
	return this->emplace_back(value, Type::FLOAT);
}

bool Net::Json::Array::push(double value)
{
	return this->emplace_back(value, Type::DOUBLE);
}

bool Net::Json::Array::push(bool value)
{
	return this->emplace_back(value, Type::BOOLEAN);
}

bool Net::Json::Array::push(const char* value)
{
	size_t len = strlen(value);
	char* ptr = new char[len + 1];
	memcpy(ptr, value, len);
	ptr[len] = 0;

	return this->emplace_back(ptr, Type::STRING);
}

bool Net::Json::Array::push(Object value)
{
	return this->emplace_back(value, Type::OBJECT);
}

bool Net::Json::Array::push(Array value)
{
	return this->emplace_back(value, Type::ARRAY);
}

bool Net::Json::Array::push(Net::Json::NullValue value)
{
	return this->emplace_back(value, Type::NULL);
}

size_t Net::Json::Array::size() const
{
	return this->value.size();
}

Net::String Net::Json::Array::Serialize(SerializeType type, size_t iterations)
{
	Net::String out(CSTRING(""));

	out += (type == SerializeType::FORMATTED ? CSTRING("[\n") : CSTRING("["));

	for (size_t i = 0; i < value.size(); ++i)
	{
		auto tmp = (BasicValue<void*>*)value[i];
		if (tmp->Type() == Type::NULL)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"(%s)"), CSTRING("null"));

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::STRING)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"("%s")"), tmp->as_string());

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::INTEGER)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"(%i)"), tmp->as_int());

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::FLOAT)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"(%F)"), tmp->as_float());

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::DOUBLE)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"(%F)"), tmp->as_double());

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::BOOLEAN)
		{
			char str[255];
			sprintf_s(str, CSTRING(R"(%s)"), tmp->as_boolean() ? CSTRING("true") : CSTRING("false"));

			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += str;
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
		else if (tmp->Type() == Type::OBJECT)
		{
			if (type == SerializeType::FORMATTED)
			{
				for (size_t it = 0; it < iterations + 1; ++it)
				{
					out += CSTRING("\t");
				}
			}

			out += tmp->as_object()->Serialize(type, (iterations + 1));
			out += (type == SerializeType::FORMATTED) ? CSTRING(",\n") : CSTRING(",");
		}
	}

	size_t it = 0;
	if ((it = out.findLastOf(CSTRING(","))) != NET_STRING_NOT_FOUND)
		out.erase(it, 1);

	if (type == SerializeType::FORMATTED) out += CSTRING("\n");

	if (type == SerializeType::FORMATTED)
	{
		for (size_t it = 0; it < iterations; ++it)
		{
			out += CSTRING("\t");
		}
	}

	out += CSTRING("]");

	return out;
}

Net::String Net::Json::Array::Stringify(SerializeType type, size_t iterations)
{
	return this->Serialize(type, iterations);
}

bool Net::Json::Array::Deserialize(Net::String json)
{
	if (json.get().get()[0] != '[')
	{
		/* not an array */
		return false;
	}

	size_t v = 0;
	bool bReadObject = false;
	for (size_t i = 1; i < json.length(); ++i)
	{
		if (!bReadObject
			&& json.get().get()[i] == '{')
		{
			v = i;
			bReadObject = true;
		}
		else if (bReadObject
			&& json.get().get()[i] == '}')
		{
			auto lastValue = json.substr(v, i - v + 1);

			Net::Json::Object obj(true);
			if (!obj.Deserialize(lastValue))
			{
				/* error */
				return false;
			}
			this->push(obj);

			bReadObject = false;
			v = i + 2;
			i = v + 1;
		}
		else if (!bReadObject
			&& (json.get().get()[i] == ',' || json.get().get()[i] == ']'))
		{
			Net::String value = json.substr(v + 1, i - v - 1);

			Net::Json::Type type = Net::Json::Type::INTEGER;

			size_t z = 0;
			for (size_t j = 0; j < value.length(); ++j)
			{
				if (value.get().get()[j] == '"')
				{
					if (type == Net::Json::Type::STRING)
					{
						value = value.substr(z + 1, j - z - 1);
						break;
					}

					type = Net::Json::Type::STRING;
					z = j;
					continue;
				}
			}

			if (type != Net::Json::Type::STRING)
			{
				/* remove all whitespaces, breaks and tabulators */
				value.eraseAll(CSTRING("\n"));
				value.eraseAll(CSTRING("\t"));
				value.eraseAll(CSTRING(" "));
			}

			/* check for boolean */
			if (Convert::is_boolean(value))
			{
				type = Net::Json::Type::BOOLEAN;
			}
			else
			{
				for (size_t j = 0; j < value.length(); ++j)
				{
					if (value.get().get()[j] == '.')
					{
						/* check if is float or a double */
						if (Convert::is_float(value))
						{
							type = Net::Json::Type::FLOAT;
						}
						else if (Convert::is_double(value))
						{
							type = Net::Json::Type::DOUBLE;
						}
						else
						{
							/* error */
							DebugBreak();
							return false;
						}

						break;
					}

					if (type != Net::Json::Type::STRING)
					{
						if (value.get().get()[j] == 'n')
						{
							if (!memcmp(&value.get().get()[j], CSTRING("null"), 4))
							{
								type = Net::Json::Type::NULL;
								break;
							}
						}
					}
				}
			}

			switch (type)
			{
			case Net::Json::Type::STRING:
				this->push(value.get().get());
				break;

			case Net::Json::Type::INTEGER:
				this->push(Convert::ToInt32(value));
				break;

			case Net::Json::Type::FLOAT:
				this->push(Convert::ToFloat(value));
				break;

			case Net::Json::Type::DOUBLE:
				this->push(Convert::ToDouble(value));
				break;

			case Net::Json::Type::BOOLEAN:
				this->push(Convert::ToBoolean(value));
				break;

			case Net::Json::Type::NULL:
				this->push(Net::Json::NullValue());
				break;

			default:
				DebugBreak();
				return false;
			}

			v = i + 1;
		}
	}

	return true;
}

bool Net::Json::Array::Parse(Net::String json)
{
	return this->Deserialize(json);
}

Net::Json::Document::Document()
{
	this->Init();
}

Net::Json::Document::~Document()
{
	this->Clear();
}

void Net::Json::Document::Init()
{
	/* by default its an object */
	this->root_obj = { true };
	this->root_array = { true };
	this->m_type = Type::OBJECT;
}

void Net::Json::Document::Clear()
{
	this->root_obj.Free();
	this->root_array.Free();
}

Net::Json::BasicValueRead Net::Json::Document::operator[](const char* key)
{
	if (this->m_type != Net::Json::Type::OBJECT) return { nullptr };
	return this->root_obj.At(key);
}

Net::Json::BasicValueRead Net::Json::Document::operator[](int idx)
{
	if (this->m_type != Net::Json::Type::ARRAY) return { nullptr };
	return this->root_array.at(idx);
}

Net::Json::BasicValueRead Net::Json::Document::At(const char* key)
{
	if (this->m_type != Net::Json::Type::OBJECT) return { nullptr };
	return this->root_obj.At(key);
}

Net::Json::BasicValueRead Net::Json::Document::At(int idx)
{
	if (this->m_type != Net::Json::Type::ARRAY) return { nullptr };
	return this->root_array.at(idx);
}

void Net::Json::Document::set(Object obj)
{
	this->root_obj = obj;
}

Net::String Net::Json::Document::Serialize(SerializeType type)
{
	switch (this->m_type)
	{
	case Type::OBJECT:
		return this->root_obj.Serialize(type);

	case Type::ARRAY:
		return this->root_array.Serialize(type);

	default:
		break;
	}

	return "";
}

Net::String Net::Json::Document::Stringify(SerializeType type)
{
	return this->Serialize(type);
}

bool Net::Json::Document::Deserialize(Net::String json)
{
	/* re-init */
	this->Clear();
	this->Init();

	if (json.get().get()[0] == '{')
	{
		/* is object */
		this->m_type = Net::Json::Type::OBJECT;
		return this->root_obj.Deserialize(json);
	}
	else if (json.get().get()[0] == '[')
	{
		this->m_type = Net::Json::Type::ARRAY;
		return this->root_array.Deserialize(json);
	}

	/* error */
	return false;
}

bool Net::Json::Document::Parse(Net::String json)
{
	return this->Deserialize(json);
}

template class Net::Json::BasicValue<int>;
template class Net::Json::BasicValue<float>;
template class Net::Json::BasicValue<double>;
template class Net::Json::BasicValue<char*>;
template class Net::Json::BasicValue<const char*>;
template class Net::Json::BasicValue<Net::Json::Object>;
template class Net::Json::BasicValue<Net::Json::Array>;
template class Net::Json::BasicValue<Net::Json::NullValue>;