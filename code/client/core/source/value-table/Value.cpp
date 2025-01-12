// <copyright file="Value.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <map>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

export module Soup.Core:Value;

import Opal;
import :LanguageReference;
import :PackageReference;

using namespace Opal;

export namespace Soup::Core
{
	class Value;
	using ValueList = std::vector<Value>;
	using ValueTable = std::map<std::string, Value>;

	enum class ValueType : uint64_t
	{
		Table = 1,
		List = 2,
		String = 3,
		Integer = 4,
		Float = 5,
		Boolean = 6,
		Version = 7,
		PackageReference = 8,
		LanguageReference = 9,
	};

	/// <summary>
	/// Build State Extension interface
	/// </summary>
	class Value
	{
	private:
		std::variant<
			ValueTable,
			ValueList,
			std::string,
			int64_t,
			double,
			bool,
			SemanticVersion,
			PackageReference,
			LanguageReference> _value;

	public:
		/// <summary>
		/// Initializes a new instance of the Value class
		/// </summary>
		Value(ValueTable table) :
			_value(std::move(table))
		{
		}

		Value(ValueList list) :
			_value(std::move(list))
		{
		}

		Value(std::string value) :
			_value(std::move(value))
		{
		}

		Value(int64_t value) :
			_value(value)
		{
		}

		Value(double value) :
			_value(value)
		{
		}

		Value(bool value) :
			_value(value)
		{
		}

		Value(SemanticVersion value) :
			_value(value)
		{
		}

		Value(LanguageReference value) :
			_value(std::move(value))
		{
		}

		Value(PackageReference value) :
			_value(std::move(value))
		{
		}

		/// <summary>
		/// Type checker methods
		/// </summary>
		ValueType GetType() const
		{
			switch (_value.index())
			{
				case 0:
					return ValueType::Table;
				case 1:
					return ValueType::List;
				case 2:
					return ValueType::String;
				case 3:
					return ValueType::Integer;
				case 4:
					return ValueType::Float;
				case 5:
					return ValueType::Boolean;
				case 6:
					return ValueType::Version;
				case 7:
					return ValueType::PackageReference;
				case 8:
					return ValueType::LanguageReference;
				default:
					throw std::runtime_error("Unknown value type.");
			}
		}

		bool IsTable() const
		{
			return GetType() == ValueType::Table;
		}

		bool IsList() const
		{
			return GetType() == ValueType::List;
		}

		bool IsString() const
		{
			return GetType() == ValueType::String;
		}

		bool IsInteger() const
		{
			return GetType() == ValueType::Integer;
		}

		bool IsFloat() const
		{
			return GetType() == ValueType::Float;
		}

		bool IsBoolean() const
		{
			return GetType() == ValueType::Boolean;
		}


		/// <summary>
		/// Internal accessors
		/// </summary>
		std::string ToString();

		ValueTable& AsTable()
		{
			if (GetType() == ValueType::Table)
			{
				return std::get<ValueTable>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Table with incorrect type.");
			}
		}

		ValueList& AsList()
		{
			if (GetType() == ValueType::List)
			{
				return std::get<ValueList>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as List with incorrect type.");
			}
		}

		const ValueTable& AsTable() const
		{
			if (GetType() == ValueType::Table)
			{
				return std::get<ValueTable>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Table with incorrect type.");
			}
		}

		const ValueList& AsList() const
		{
			if (GetType() == ValueType::List)
			{
				return std::get<ValueList>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as List with incorrect type.");
			}
		}

		const std::string& AsString() const
		{
			if (GetType() == ValueType::String)
			{
				return std::get<std::string>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as String with incorrect type.");
			}
		}

		int64_t AsInteger() const
		{
			if (GetType() == ValueType::Integer)
			{
				return std::get<int64_t>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Integer with incorrect type.");
			}
		}

		double AsFloat() const
		{
			if (GetType() == ValueType::Float)
			{
				return std::get<double>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Float with incorrect type.");
			}
		}

		bool AsBoolean() const
		{
			if (GetType() == ValueType::Boolean)
			{
				return std::get<bool>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Boolean with incorrect type.");
			}
		}

		SemanticVersion AsVersion() const
		{
			if (GetType() == ValueType::Version)
			{
				return std::get<SemanticVersion>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as Version with incorrect type.");
			}
		}

		PackageReference AsPackageReference() const
		{
			if (GetType() == ValueType::PackageReference)
			{
				return std::get<PackageReference>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as PackageReference with incorrect type.");
			}
		}

		LanguageReference AsLanguageReference() const
		{
			if (GetType() == ValueType::LanguageReference)
			{
				return std::get<LanguageReference>(_value);
			}
			else
			{
				// Wrong type
				throw std::runtime_error("Attempt to access value as LanguageReference with incorrect type.");
			}
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const Value& rhs) const
		{
			if (GetType() == rhs.GetType())
			{
				switch (GetType())
				{
					case ValueType::Table:
						return std::get<ValueTable>(_value) == std::get<ValueTable>(rhs._value);
					case ValueType::List:
						return std::get<ValueList>(_value) == std::get<ValueList>(rhs._value);
					case ValueType::String:
						return std::get<std::string>(_value) == std::get<std::string>(rhs._value);
					case ValueType::Integer:
						return std::get<int64_t>(_value) == std::get<int64_t>(rhs._value);
					case ValueType::Float:
						return std::get<double>(_value) == std::get<double>(rhs._value);
					case ValueType::Boolean:
						return std::get<bool>(_value) == std::get<bool>(rhs._value);
					default:
						throw std::runtime_error("Unkown ValueType for comparison.");
				}
			}
			else
			{
				return false;
			}
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const Value& rhs) const
		{
			return !(*this == rhs);
		}
	};
}