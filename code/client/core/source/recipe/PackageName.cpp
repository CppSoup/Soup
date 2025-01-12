// <copyright file="PackageName.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <optional>
#include <regex>
#include <sstream>
#include <string>

export module Soup.Core:PackageName;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// A package owner/name that uniquely identifies a package in a single language
	/// </summary>
	export class PackageName
	{
	private:
		std::optional<std::string> _owner;
		std::string _name;

	public:
		/// <summary>
		/// Try parse a package name from the provided string
		/// </summary>
		static bool TryParse(const std::string& value, PackageName& result)
		{
			// Reuse regex between runs
			static auto nameRegex = std::regex(R"(^(?:([A-Za-z][\w.]*)\|)?([A-Za-z][\w.]*)$)");

			// Attempt to parse Named package
			auto nameMatch = std::smatch();
			if (std::regex_match(value, nameMatch, nameRegex))
			{
				// The package is a valid name
				std::optional<std::string> owner = std::nullopt;
				auto ownerMatch = nameMatch[1];
				if (ownerMatch.matched)
				{
					owner = ownerMatch.str();
				}

				auto name = nameMatch[2].str();

				result = PackageName(std::move(owner), std::move(name));
				return true;
			}
			else
			{
				result = PackageName();
				return false;
			}
		}

		/// <summary>
		/// Parse a package name from the provided string.
		/// </summary>
		static PackageName Parse(const std::string& value)
		{
			PackageName result;
			if (TryParse(value, result))
			{
				return result;
			}
			else
			{
				throw std::runtime_error("Invalid package name");
			}
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="PackageName"/> class.
		/// </summary>
		PackageName() :
			_owner(std::nullopt),
			_name()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="PackageName"/> class.
		/// </summary>
		PackageName(
			std::optional<std::string> owner,
			std::string name) :
			_owner(std::move(owner)),
			_name(std::move(name))
		{
		}

		/// <summary>
		/// Gets or sets a value indicating if there is an owner.
		/// </summary>
		bool HasOwner() const
		{
			return _owner.has_value();
		}

		/// <summary>
		/// Gets or sets the Owner.
		/// </summary>
		const std::string& GetOwner() const
		{
			if (!_owner.has_value())
				throw std::runtime_error("Package name does not have an owner.");
			return _owner.value();
		}

		/// <summary>
		/// Gets or sets the Name.
		/// </summary>
		const std::string& GetName() const
		{
			return _name;
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageName& rhs) const
		{
			return _owner == rhs._owner &&
				_name == rhs._name;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageName& rhs) const
		{
			return _owner != rhs._owner ||
				_name != rhs._name;
		}

		/// <summary>
		/// Less Than operator
		/// </summary>
		bool operator<(const PackageName& other) const
		{
			return std::tie(_owner, _name) < std::tie(other._owner, other._name);
		}

		/// <summary>
		/// Convert to string
		/// </summary>
		std::string ToString() const
		{
			std::stringstream stringBuilder;

			if (_owner.has_value())
			{
				stringBuilder << _owner.value() << '|';
			}

			stringBuilder << _name;

			return stringBuilder.str();
		}
	};
}
