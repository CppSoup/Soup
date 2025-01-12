// <copyright file="SML.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

export namespace Soup::Core
{
	class SMLValue;

	enum class SMLValueType
	{
		Table,
		Array,
		String,
		Integer,
		Float,
		Boolean,
		Version,
		PackageReference,
		LanguageReference,
	};

	class SMLTable
	{
	private:
		SequenceMap<std::string, SMLValue> _value;

	public:
		SMLTable() :
			_value()
		{
		}

		SMLTable(SequenceMap<std::string, SMLValue> value) :
			_value(std::move(value))
		{
		}

		bool Contains(const std::string& key) const
		{
			return _value.Contains(key);
		}

		const SMLValue& operator[](const std::string& key) const
		{
			return _value[key];
		}

		const SequenceMap<std::string, SMLValue>& GetValue() const
		{
			return _value;
		}
	};

	class SMLArray
	{
	public:
		SMLArray();

		SMLArray(std::vector<SMLValue> value);

		const SMLValue& operator[](size_t key) const;

		const size_t GetSize() const;

		const std::vector<SMLValue>& GetValue() const;

	private:
		std::vector<SMLValue> _value;
	};

	/// <summary>
	/// The SML Document
	/// </summary>
	class SMLDocument
	{
	public:
		/// <summary>
		/// Load from stream
		/// </summary>
		static SMLDocument Parse(std::istream& stream);
		static SMLDocument Parse(const char* data, size_t size);

	public:
		SMLDocument(SMLTable root) :
			_root(std::move(root))
		{
		}

		const SMLTable& GetRoot() const
		{
			return _root;
		}

	private:
		SMLTable _root;
	};

	class SMLValue
	{
	public:
		SMLValue(SMLTable value) :
			_value(std::move(value))
		{
		}

		SMLValue(SMLArray value) :
			_value(std::move(value))
		{
		}

		SMLValue(std::string value) :
			_value(std::move(value))
		{
		}

		SMLValue(int64_t value) :
			_value(value)
		{
		}

		SMLValue(double value) :
			_value(value)
		{
		}

		SMLValue(bool value) :
			_value(value)
		{
		}

		SMLValue(SemanticVersion value) :
			_value(value)
		{
		}

		SMLValue(PackageReference value) :
			_value(value)
		{
		}

		SMLValue(LanguageReference value) :
			_value(value)
		{
		}

		SMLValueType GetType() const
		{
			switch (_value.index())
			{
				case 0:
					return SMLValueType::Table;
				case 1:
					return SMLValueType::Array;
				case 2:
					return SMLValueType::String;
				case 3:
					return SMLValueType::Integer;
				case 4:
					return SMLValueType::Float;
				case 5:
					return SMLValueType::Boolean;
				case 6:
					return SMLValueType::Version;
				case 7:
					return SMLValueType::PackageReference;
				case 8:
					return SMLValueType::LanguageReference;
				default:
					throw std::runtime_error("Unknown SML value type.");
			}
		}

		const SMLTable& AsTable() const
		{
			if (GetType() != SMLValueType::Table)
				throw std::runtime_error("Incorrect access type: Value is not Table");
			else
				return std::get<SMLTable>(_value);
		}

		const SMLArray& AsArray() const
		{
			if (GetType() != SMLValueType::Array)
				throw std::runtime_error("Incorrect access type: Value is not Array");
			else
				return std::get<SMLArray>(_value);
		}

		const std::string& AsString() const
		{
			if (GetType() != SMLValueType::String)
				throw std::runtime_error("Incorrect access type: Value is not String");
			else
				return std::get<std::string>(_value);
		}

		int64_t AsInteger() const
		{
			if (GetType() != SMLValueType::Integer)
				throw std::runtime_error("Incorrect access type: Value is not Integer");
			else
				return std::get<int64_t>(_value);
		}

		double AsFloat() const
		{
			if (GetType() != SMLValueType::Float)
				throw std::runtime_error("Incorrect access type: Value is not Float");
			else
				return std::get<double>(_value);
		}

		bool AsBoolean() const
		{
			if (GetType() != SMLValueType::Boolean)
				throw std::runtime_error("Incorrect access type: Value is not Boolean");
			else
				return std::get<bool>(_value);
		}

		SemanticVersion AsVersion() const
		{
			if (GetType() != SMLValueType::Version)
				throw std::runtime_error("Incorrect access type: Value is not Version");
			else
				return std::get<SemanticVersion>(_value);
		}

		PackageReference AsPackageReference() const
		{
			if (GetType() != SMLValueType::PackageReference)
				throw std::runtime_error("Incorrect access type: Value is not PackageReference");
			else
				return std::get<PackageReference>(_value);
		}

		LanguageReference AsLanguageReference() const
		{
			if (GetType() != SMLValueType::LanguageReference)
				throw std::runtime_error("Incorrect access type: Value is not LanguageReference");
			else
				return std::get<LanguageReference>(_value);
		}

	private:
		std::variant<
			SMLTable,
			SMLArray,
			std::string,
			int64_t,
			double,
			bool,
			SemanticVersion,
			PackageReference,
			LanguageReference> _value;
	};

	std::ostream& operator<<(std::ostream& stream, const SMLValue& value);
	std::ostream& operator<<(std::ostream& stream, const SMLDocument& value);

	SMLArray::SMLArray() :
		_value()
	{
	}

	SMLArray::SMLArray(std::vector<SMLValue> value) :
		_value(std::move(value))
	{
	}

	const SMLValue& SMLArray::operator[](size_t key) const
	{
		return _value.at(key);
	}

	const size_t SMLArray::GetSize() const
	{
		return _value.size();
	}

	const std::vector<SMLValue>& SMLArray::GetValue() const
	{
		return _value;
	}

#ifdef CLIENT_CORE_IMPLEMENTATION
	std::ostream& operator<<(std::ostream& stream, const SMLDocument& document)
	{
		for (const auto& [key, value] : document.GetRoot().GetValue())
		{
			stream << key << ": " << value << "\n";
		}

		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const SMLTable& table)
	{
		stream << '{';
		for (const auto& [key, value] : table.GetValue())
		{
			stream << key << ": " << value << "\n";
		}

		stream << '}';

		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const SMLArray& value)
	{
		stream << '[';
		for (const auto& arrayValue : value.GetValue())
		{
			stream << arrayValue << '\n';
		}

		stream << ']';

		return stream;
	}

	std::ostream& operator<<(std::ostream& stream, const SMLValue& value)
	{
		switch (value.GetType())
		{
			case SMLValueType::Boolean:
				if (value.AsBoolean())
					stream << "true";
				else
					stream << "false";
				break;
			case SMLValueType::Integer:
				stream << value.AsInteger();
				break;
			case SMLValueType::Float:
				stream << value.AsFloat();
				break;
			case SMLValueType::String:
				// TODO: Escape characters
				stream << "'" << value.AsString() << "'";
				break;
			case SMLValueType::Array:
				stream <<  value.AsArray();
				break;
			case SMLValueType::Table:
				stream <<  value.AsTable();
				break;
			case SMLValueType::Version:
				stream << value.AsVersion().ToString();
				break;
			case SMLValueType::PackageReference:
				{
					const auto& packageReference = value.AsPackageReference();
					stream << "<";
					if (packageReference.HasLanguage())
					{
						stream << "(" << packageReference.GetLanguage() << ")";
					}

					stream << packageReference.GetOwner() << "|" << packageReference.GetName() << "@" << packageReference.GetVersion().ToString() << ">";
				}
				break;
			case SMLValueType::LanguageReference:
				{
					const auto& languageReference = value.AsLanguageReference();
					stream << "(" << languageReference.GetName() << "@" << languageReference.GetVersion().ToString() << ")";
				}
				break;
			default:
				throw std::runtime_error("Unknown SML type.");
		}

		return stream;
	}
#endif
}
