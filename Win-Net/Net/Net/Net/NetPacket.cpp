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

#include <Net/Net/NetPacket.h>

Net::RawData_t::RawData_t()
{
	memset(this->_key, 0, 256);
	this->_data = nullptr;
	this->_size = 0;
	this->_original_size = 0;
	this->_free_after_sent = false;
	this->_valid = false;
}

Net::RawData_t::RawData_t(const char* name, byte* pointer, const size_t size)
{
	strcpy(this->_key, name);
	this->_data = pointer;
	this->_size = size;
	this->_original_size = size;
	this->_free_after_sent = true;
	this->_valid = true;
}

Net::RawData_t::RawData_t(const char* name, byte* pointer, const size_t size, const bool free_after_sent)
{
	strcpy(this->_key, name);
	this->_data = pointer;
	this->_size = size;
	this->_original_size = size;
	this->_free_after_sent = free_after_sent;
	this->_valid = true;
}

bool Net::RawData_t::valid() const
{
	return _valid;
}

byte* Net::RawData_t::value() const
{
	return _data;
}

byte*& Net::RawData_t::value()
{
	return _data;
}

size_t Net::RawData_t::size() const
{
	return _size;
}

size_t& Net::RawData_t::size()
{
	return _size;
}

void Net::RawData_t::set_free(bool free)
{
	_free_after_sent = free;

}

bool Net::RawData_t::do_free() const
{
	return _free_after_sent;
}

const char* Net::RawData_t::key() const
{
	if (_key == nullptr) return CSTRING("");
	return _key;
}

void Net::RawData_t::set(byte* pointer)
{
	if (do_free()) FREE<byte>(_data);
	_data = pointer;
}

void Net::RawData_t::free()
{
	if (!this->_valid) return;

	// only free the passed reference if we allow it
	if (do_free()) FREE<byte>(this->_data);

	this->_data = nullptr;
	this->_size = 0;

	this->_valid = false;
}

void Net::RawData_t::set_original_size(size_t size)
{
	this->_original_size = size;
}

size_t Net::RawData_t::original_size() const
{
	return _original_size;
}

size_t& Net::RawData_t::original_size()
{
	return _original_size;
}

Net::Packet::Packet::Packet()
{
	this->json = {};
	this->raw = {};
	this->freeRaw = true;
}

Net::Packet::Packet::~Packet()
{
	if (this->freeRaw)
	{
		for (auto& entry : this->raw)
		{
			entry.free();
		}
	}
}

Net::Json::Document& Net::Packet::Data()
{
	return this->json;
}

void Net::Packet::AddRaw(const char* Key, BYTE* data, const size_t size, const bool free_after_sent)
{
	for (auto& entry : this->raw)
	{
		if (!strcmp(entry.key(), Key))
		{
			if (free_after_sent)
			{
				NET_LOG_ERROR(CSTRING("Duplicated Key, buffer gets automaticly deleted from heap to avoid memory leaks"));
				FREE<byte>(data);
				return;
			}

			NET_LOG_ERROR(CSTRING("Duplicated Key, buffer has not been deleted from heap"));
			return;
		}
	}

	this->raw.emplace_back(Net::RawData_t(Key, data, size, free_after_sent));
}

void Net::Packet::AddRaw(Net::RawData_t& raw)
{
	for (auto& entry : this->raw)
	{
		if (!strcmp(entry.key(), raw.key()))
		{
			if (raw.do_free())
			{
				NET_LOG_ERROR(CSTRING("Duplicated Key, buffer gets automaticly deleted from heap to avoid memory leaks"));
				raw.free();
				return;
			}

			NET_LOG_ERROR(CSTRING("Duplicated Key, buffer has not been deleted from heap"));
			return;
		}
	}

	this->raw.emplace_back(raw);
}

bool Net::Packet::Deserialize(char* data)
{
	return this->json.Deserialize(data);
}

bool Net::Packet::Deserialize(const char* data)
{
	return this->json.Deserialize(data);
}

void Net::Packet::SetJson(Net::Json::Document& doc)
{
	this->json = doc;
}

void Net::Packet::SetRaw(const std::vector<Net::RawData_t>& raw)
{
	this->raw = raw;
}

void Net::Packet::SetFreeRaw(bool freeRaw)
{
	this->freeRaw = freeRaw;
}

std::vector<Net::RawData_t>& Net::Packet::Packet::GetRawData()
{
	return this->raw;
}

bool Net::Packet::Packet::HasRawData() const
{
	return !this->raw.empty();
}

size_t Net::Packet::Packet::GetRawDataFullSize(bool bCompression) const
{
	size_t size = 0;
	for (auto& entry : this->raw)
	{
		size += strlen(NET_RAW_DATA_KEY);
		size += 1;
		const auto KeyLengthStr = std::to_string(strlen(entry.key()) + 1);
		size += KeyLengthStr.size();
		size += 1;

		if (bCompression)
		{
			size += strlen(NET_RAW_DATA_ORIGINAL_SIZE);
			size += 1;
			const auto rawDataOriginalLengthStr = std::to_string(entry.original_size());
			size += rawDataOriginalLengthStr.size();
			size += 1;
		}

		size += strlen(NET_RAW_DATA);
		size += 1;
		const auto rawDataLengthStr = std::to_string(entry.size());
		size += rawDataLengthStr.size();
		size += 1;
		size += strlen(entry.key()) + 1;
		size += entry.size();
	}

	return size;
}

Net::RawData_t* Net::Packet::GetRaw(const char* Key)
{
	for (auto& raw : this->raw)
	{
		if (!strcmp(raw.key(), Key))
		{
			return &raw;
		}
	}

	return nullptr;
}

Net::String Net::Packet::Stringify()
{
	return this->json.Serialize(Net::Json::SerializeType::UNFORMATTED);
}
