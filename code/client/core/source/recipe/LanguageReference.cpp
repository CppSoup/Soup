// <copyright file="LanguageReference.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

export module Soup.Core:LanguageReference;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// A language reference object which will consist of a name version pair that
	/// refers to a published package
	/// </summary>
	export class LanguageReference
	{
	public:
		/// <summary>
		/// Try parse a language reference from the provided string
		/// </summary>
		static bool TryParse(const std::string& value, LanguageReference& result);

		/// <summary>
		/// Parse a language reference from the provided string.
		/// </summary>
		static LanguageReference Parse(const std::string& value)
		{
			LanguageReference result;
			if (TryParse(value, result))
			{
				return result;
			}
			else
			{
				throw std::runtime_error(
					std::format("Invalid language reference: {}", value));
			}
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="LanguageReference"/> class.
		/// </summary>
		LanguageReference() :
			_name(""),
			_version()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="LanguageReference"/> class.
		/// </summary>
		LanguageReference(
			std::string name,
			SemanticVersion version) :
			_name(std::move(name)),
			_version(version)
		{
		}

		/// <summary>
		/// Gets or sets the Name.
		/// </summary>
		const std::string& GetName() const
		{
			return _name;
		}

		/// <summary>
		/// Gets or sets the Version.
		/// </summary>
		const SemanticVersion& GetVersion() const
		{
			return _version;
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const LanguageReference& rhs) const
		{
			return _name == rhs. _name &&
				_version == rhs. _version;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const LanguageReference& rhs) const
		{
			return _name != rhs. _name ||
				_version != rhs. _version;
		}

		/// <summary>
		/// Convert to string
		/// </summary>
		std::string ToString() const
		{
			// Build up the name/version reference
			std::stringstream stringBuilder;

			stringBuilder << _name << '|' << _version.ToString();

			return stringBuilder.str();
		}

	private:
		static bool IsCharacter(const char value)
		{
			return value >='a' && value <='z' ||
				value >='A' && value <='Z';
		} 

	private:
		std::string _name;
		SemanticVersion _version;
	};
}
