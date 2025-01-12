// <copyright file="ValueTableReader.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <array>
#include <iostream>
#include <sstream>

export module Soup.Core:ValueTableReader;

import Opal;
import :Value;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The value table state reader
	/// </summary>
	export class ValueTableReader
	{
	private:
		// Binary Value Table file format
		static constexpr uint32_t FileVersion = 2;

	public:
		static ValueTable Deserialize(std::istream& stream)
		{
			// Read the entire file for fastest read operation
			stream.seekg(0, std::ios_base::end);
			auto size = stream.tellg();
			stream.seekg(0, std::ios_base::beg);

			auto contentBuffer = std::vector<char>(size);
			stream.read(contentBuffer.data(), size);
			auto data = contentBuffer.data();
			size_t offset = 0;
			
			auto result = Deserialize(data, size, offset);

			if (offset != contentBuffer.size())
			{
				throw std::runtime_error("Value Table file corrupted - Did not read the entire file");
			}

			return result;
		}

	private:
		static ValueTable Deserialize(
			char* data, size_t size, size_t& offset)
		{
			// Read the File Header with version
			auto headerBuffer = std::array<char, 4>();
			Read(data, size, offset, headerBuffer.data(), 4);
			if (headerBuffer[0] != 'B' ||
				headerBuffer[1] != 'V' ||
				headerBuffer[2] != 'T' ||
				headerBuffer[3] != '\0')
			{
				throw std::runtime_error("Invalid Value Table file header");
			}

			auto fileVersion = ReadUInt32(data, size, offset);
			if (fileVersion != FileVersion)
			{
				throw std::runtime_error("Value Table file version does not match expected");
			}

			// Read the root table
			Read(data, size, offset, headerBuffer.data(), 4);
			if (headerBuffer[0] != 'T' ||
				headerBuffer[1] != 'B' ||
				headerBuffer[2] != 'L' ||
				headerBuffer[3] != '\0')
			{
				throw std::runtime_error("Invalid Value Table table header");
			}

			auto rootTable = ReadValueTable(data, size, offset);

			return rootTable;
		}

		static Value ReadValue(char* data, size_t size, size_t& offset)
		{
			// Read the value type
			auto valueType = static_cast<ValueType>(ReadUInt32(data, size, offset));

			switch (valueType)
			{
				case ValueType::Table:
					return Value(ReadValueTable(data, size, offset));
				case ValueType::List:
					return Value(ReadValueList(data, size, offset));
				case ValueType::String:
					return Value(ReadString(data, size, offset));
				case ValueType::Integer:
					return Value(ReadInt64(data, size, offset));
				case ValueType::Float:
					return Value(ReadDouble(data, size, offset));
				case ValueType::Boolean:
					return Value(ReadBoolean(data, size, offset));
				case ValueType::Version:
					return Value(SemanticVersion::Parse(ReadString(data, size, offset)));
				case ValueType::PackageReference:
					return Value(PackageReference::Parse(ReadString(data, size, offset)));
				case ValueType::LanguageReference:
					return Value(LanguageReference::Parse(ReadString(data, size, offset)));
				default:
					throw std::runtime_error("Read Unknown ValueType");
			}
		}

		static ValueTable ReadValueTable(char* data, size_t size, size_t& offset)
		{
			// Write out the table size
			auto tableSize = ReadUInt32(data, size, offset);

			auto table = ValueTable();
			for (auto i = 0u; i < tableSize; i++)
			{
				// Read the key
				auto key = ReadString(data, size, offset);

				// Read the value
				auto value = ReadValue(data, size, offset);

				table.emplace(std::move(key), std::move(value));
			}

			return table;
		}

		static ValueList ReadValueList(char* data, size_t size, size_t& offset)
		{
			// Write out the list size
			auto listSize = ReadUInt32(data, size, offset);

			auto list = ValueList();
			for (auto i = 0u; i < listSize; i++)
			{
				// Read the value
				auto value = ReadValue(data, size, offset);

				list.push_back(std::move(value));
			}

			return list;
		}

		static int64_t ReadInt64(char* data, size_t size, size_t& offset)
		{
			int64_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(int64_t));

			return result;
		}

		static uint32_t ReadUInt32(char* data, size_t size, size_t& offset)
		{
			uint32_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(uint32_t));

			return result;
		}

		static double ReadDouble(char* data, size_t size, size_t& offset)
		{
			double result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(double));

			return result;
		}

		static bool ReadBoolean(char* data, size_t size, size_t& offset)
		{
			uint32_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(uint32_t));

			return result != 0;
		}

		static std::string ReadString(char* data, size_t size, size_t& offset)
		{
			auto stringLength = ReadUInt32(data, size, offset);
			auto result = std::string(stringLength, '\0');
			Read(data, size, offset, result.data(), stringLength);

			return result;
		}

		static void Read(char* data, size_t size, size_t& offset, char* buffer, size_t count)
		{
			if (offset + count > size)
				throw new std::runtime_error("Tried to read past end of data");
			memcpy(buffer, data + offset, count);
			offset += count;
		}
	};
}