// <copyright file="ValueTableWriter.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <iostream>

export module Soup.Core:ValueTableWriter;

import Opal;
import :Value;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The value table state writer
	/// </summary>
	export class ValueTableWriter
	{
	private:
		// Binary Value Table file format
		static constexpr uint32_t FileVersion = 2;

	public:
		static void Serialize(const ValueTable& state, std::ostream& stream)
		{
			// Write the File Header with version
			stream.write("BVT\0", 4);
			WriteValue(stream, FileVersion);

			// Write out the root table
			stream.write("TBL\0", 4);
			WriteValue(stream, state);
		}

	private:
		static void WriteValue(std::ostream& stream, const Value& value)
		{
			// Write the value type
			auto valueType = value.GetType();
			WriteValue(stream, static_cast<uint32_t>(valueType));

			switch (valueType)
			{
				case ValueType::Table:
					WriteValue(stream, value.AsTable());
					break;
				case ValueType::List:
					WriteValue(stream, value.AsList());
					break;
				case ValueType::String:
					WriteValue(stream, std::string_view(value.AsString()));
					break;
				case ValueType::Integer:
					WriteValue(stream, value.AsInteger());
					break;
				case ValueType::Float:
					WriteValue(stream, value.AsFloat());
					break;
				case ValueType::Boolean:
					WriteValue(stream, value.AsBoolean());
					break;
				case ValueType::Version:
					WriteValue(stream, std::string_view(value.AsVersion().ToString()));
					break;
				case ValueType::PackageReference:
					WriteValue(stream, std::string_view(value.AsPackageReference().ToString()));
					break;
				case ValueType::LanguageReference:
					WriteValue(stream, std::string_view(value.AsLanguageReference().ToString()));
					break;
				default:
					throw std::runtime_error("Write Unknown ValueType");
			}
		}

		static void WriteValue(std::ostream& stream, const ValueTable& table)
		{
			// Write the count of values
			WriteValue(stream, static_cast<uint32_t>(table.size()));

			for (const auto& [key, value] : table)
			{
				// Write the key
				WriteValue(stream, std::string_view(key));

				// Write the value
				WriteValue(stream, value);
			}
		}

		static void WriteValue(std::ostream& stream, const ValueList& value)
		{
			// Write the count of values
			WriteValue(stream, static_cast<uint32_t>(value.size()));

			for (auto& listValue : value)
			{
				WriteValue(stream, listValue);
			}
		}

		static void WriteValue(std::ostream& stream, uint32_t value)
		{
			stream.write(reinterpret_cast<char*>(&value), sizeof(uint32_t));
		}

		static void WriteValue(std::ostream& stream, int64_t value)
		{
			stream.write(reinterpret_cast<char*>(&value), sizeof(int64_t));
		}

		static void WriteValue(std::ostream& stream, double value)
		{
			stream.write(reinterpret_cast<char*>(&value), sizeof(double));
		}

		static void WriteValue(std::ostream& stream, bool value)
		{
			uint32_t integerValue = value ? 1u : 0u;
			stream.write(reinterpret_cast<char*>(&integerValue), sizeof(uint32_t));
		}

		static void WriteValue(std::ostream& stream, std::string_view value)
		{
			WriteValue(stream, static_cast<uint32_t>(value.size()));
			stream.write(value.data(), value.size());
		}
	};
}