// <copyright file="PackageReference.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <optional>
#include <format>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

export module Soup.Core:PackageReference;

import Opal;
import :PackageIdentifier;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// A package reference object which will consist of a name version pair that
	/// refers to a published package or a path to a local recipe
	/// </summary>
	export class PackageReference
	{
	private:
		std::optional<PackageIdentifier> _identifier;
		std::optional<SemanticVersion> _version;
		std::optional<Path> _path;

	public:
		/// <summary>
		/// Try parse a package reference from the provided string
		/// </summary>
		static bool TryParse(const std::string& value, PackageReference& result)
		{
			// Reuse regex between runs
			static auto nameRegex = std::regex(R"(^(?:\[([\w#+]+)\])?(?:([A-Za-z][\w.]*)\|)?([A-Za-z][\w.]*)(?:@(\d+(?:.\d+)?(?:.\d+)?))?$)");

			auto parseValue = value;
			if (parseValue.starts_with("C++|"))
			{
				parseValue = std::format("[C++]Soup|{}", parseValue.substr(4));
				Log::Info("Replace C++| -> {}", parseValue);
			}

			// Attempt to parse Named reference
			auto nameMatch = std::smatch();
			if (std::regex_match(parseValue, nameMatch, nameRegex))
			{
				// The package is a published reference
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

				std::optional<SemanticVersion> version = std::nullopt;
				auto versionMatch = nameMatch[4];
				if (versionMatch.matched)
				{
					version = SemanticVersion::Parse(versionMatch.str());
				}

				result = PackageReference(std::move(language), std::move(owner), std::move(name), version);
				return true;
			}
			else
			{
				try
				{
					// Assume that this package is a relative path reference
					// TODO: Add a try parse Path
					result = PackageReference(Path(value));
					return true;
				}
				catch (const std::runtime_error&)
				{
					result = PackageReference();
					return false;
				}
			}
		}

		/// <summary>
		/// Parse a package reference from the provided string.
		/// </summary>
		static PackageReference Parse(const std::string& value)
		{
			PackageReference result;
			if (TryParse(value, result))
			{
				return result;
			}
			else
			{
				throw std::runtime_error("Invalid package reference");
			}
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="PackageReference"/> class.
		/// </summary>
		PackageReference() :
			_identifier(std::nullopt),
			_version(std::nullopt),
			_path(std::nullopt)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="PackageReference"/> class.
		/// </summary>
		PackageReference(
			std::optional<std::string> language,
			std::optional<std::string> owner,
			std::string name,
			std::optional<SemanticVersion> version) :
			_identifier(PackageIdentifier(std::move(language), std::move(owner), std::move(name))),
			_version(version),
			_path(std::nullopt)
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="PackageReference"/> class.
		/// </summary>
		PackageReference(Path path) :
			_identifier(std::nullopt),
			_version(std::nullopt),
			_path(std::move(path))
		{
		}

		/// <summary>
		/// Gets a value indicating whether the reference is local or not
		/// </summary>
		bool IsLocal() const
		{
			return _path.has_value();
		}

		/// <summary>
		/// Gets or sets the Language.
		/// </summary>
		bool HasLanguage() const
		{
			return _identifier.has_value() && _identifier.value().HasLanguage();
		}

		/// <summary>
		/// Gets or sets the Language.
		/// </summary>
		const std::string& GetLanguage() const
		{
			if (!_identifier.has_value())
				throw std::runtime_error("PackageReference does not have an identifier.");
			return _identifier.value().GetLanguage();
		}

		/// <summary>
		/// Gets or sets the Owner.
		/// </summary>
		bool HasOwner() const
		{
			return _identifier.has_value() && _identifier.value().HasOwner();
		}

		/// <summary>
		/// Gets or sets the Owner.
		/// </summary>
		const std::string& GetOwner() const
		{
			if (!_identifier.has_value())
				throw std::runtime_error("PackageReference does not have an identifier.");
			return _identifier.value().GetOwner();
		}

		/// <summary>
		/// Gets or sets the Name.
		/// </summary>
		const std::string& GetName() const
		{
			if (!_identifier.has_value())
				throw std::runtime_error("PackageReference does not have an identifier.");
			return _identifier.value().GetName();
		}

		/// <summary>
		/// Gets or sets the Version.
		/// </summary>
		const SemanticVersion& GetVersion() const
		{
			if (!_version.has_value())
				throw std::runtime_error("PackageReference does not have a Version value.");
			return _version.value();
		}

		/// <summary>
		/// Gets or sets the Path.
		/// </summary>
		const Path& GetPath() const
		{
			if (!_path.has_value())
				throw std::runtime_error("Cannot get the path of a non-local reference.");
			return _path.value();
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageReference& rhs) const
		{
			return _identifier == rhs._identifier &&
				_version == rhs._version &&
				_path == rhs._path;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageReference& rhs) const
		{
			return _identifier != rhs._identifier ||
				_version != rhs._version ||
				_path != rhs._path;
		}

		/// <summary>
		/// Convert to string
		/// </summary>
		std::string ToString() const
		{
			// If the reference is a path then just return that
			if (IsLocal())
			{
				return _path.value().ToString();
			}
			else
			{
				// Build up the language/name/version reference
				std::stringstream stringBuilder;

				stringBuilder << _identifier.value().ToString();

				if (_version.has_value())
				{
					stringBuilder << '@' << _version.value().ToString();
				}

				return stringBuilder.str();
			}
		}
	};
}
