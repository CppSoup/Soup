// <copyright file="PackageIdentifier.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <optional>
#include <regex>
#include <sstream>
#include <string>

export module Soup.Core:PackageIdentifier;

import Opal;
import :PackageName;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// A package language/owner/name that uniquely identifies a package in a single build graph
	/// </summary>
	export class PackageIdentifier
	{
	private:
		std::optional<std::string> _language;
		PackageName _name;

	public:
		/// <summary>
		/// Try parse a package identifier from the provided string
		/// </summary>
		static bool TryParse(const std::string& value, PackageIdentifier& result)
		{
			// Reuse regex between runs
			static auto nameRegex = std::regex(R"(^(?:\[([\w#+]+)\])?(?:([A-Za-z][\w.]*)\|)?([A-Za-z][\w.]*)$)");

			// Attempt to parse Named package
			auto nameMatch = std::smatch();
			if (std::regex_match(value, nameMatch, nameRegex))
			{
				std::optional<std::string> language = std::nullopt;
				auto languageMatch = nameMatch[1];
				if (languageMatch.matched)
				{
					language = languageMatch.str();
				}

				std::optional<std::string> owner = std::nullopt;
				auto ownerMatch = nameMatch[2];
				if (ownerMatch.matched)
				{
					owner = ownerMatch.str();
				}

				auto name = nameMatch[3].str();

				result = PackageIdentifier(std::move(language), std::move(owner), std::move(name));
				return true;
			}
			else
			{
				result = PackageIdentifier();
				return false;
			}
		}

		/// <summary>
		/// Parse a package identifier from the provided string.
		/// </summary>
		static PackageIdentifier Parse(const std::string& value)
		{
			PackageIdentifier result;
			if (TryParse(value, result))
			{
				return result;
			}
			else
			{
				throw std::runtime_error("Invalid package identifier");
			}
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="PackageIdentifier"/> class.
		/// </summary>
		PackageIdentifier() :
			_language(std::nullopt),
			_name()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="PackageIdentifier"/> class.
		/// </summary>
		PackageIdentifier(
			std::optional<std::string> language,
			std::optional<std::string> owner,
			std::string name) :
			_language(std::move(language)),
			_name(std::move(owner), std::move(name))
		{
		}

		/// <summary>
		/// Gets or sets a value indicating if there is a language.
		/// </summary>
		bool HasLanguage() const
		{
			return _language.has_value();
		}

		/// <summary>
		/// Gets or sets the language.
		/// </summary>
		const std::string& GetLanguage() const
		{
			return _language.value();
		}

		/// <summary>
		/// Gets or sets a value indicating if there is an owner.
		/// </summary>
		bool HasOwner() const
		{
			return _name.HasOwner();
		}

		/// <summary>
		/// Gets or sets the Package Name.
		/// </summary>
		const PackageName& GetPackageName() const
		{
			return _name;
		}

		/// <summary>
		/// Gets or sets the Owner.
		/// </summary>
		const std::string& GetOwner() const
		{
			return _name.GetOwner();
		}

		/// <summary>
		/// Gets or sets the Name.
		/// </summary>
		const std::string& GetName() const
		{
			return _name.GetName();
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageIdentifier& rhs) const
		{
			return _language == rhs._language &&
				_name == rhs._name;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageIdentifier& rhs) const
		{
			return _language != rhs._language ||
				_name != rhs._name;
		}

		/// <summary>
		/// Less Than operator
		/// </summary>
		bool operator<(const PackageIdentifier& other) const
		{
			return std::tie(_language, _name) < std::tie(other._language, other._name);
		}

		/// <summary>
		/// Convert to string
		/// </summary>
		std::string ToString() const
		{
			std::stringstream stringBuilder;

			if (_language.has_value())
			{
				stringBuilder << '[' << _language.value() << ']';
			}

			stringBuilder << _name.ToString();

			return stringBuilder.str();
		}
	};
}
