// <copyright file="PackageLock.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "recipe/RecipeValue.h"

export namespace Soup::Core
{
	class PackageClosureValue
	{
	public:
		PackageClosureValue(
			PackageReference reference,
			std::optional<std::string> buildValue,
			std::optional<std::string> toolValue) :
			Reference(std::move(reference)),
			BuildValue(std::move(buildValue)),
			ToolValue(std::move(toolValue))
		{
		}

		PackageReference Reference;
		std::optional<std::string> BuildValue;
		std::optional<std::string> ToolValue;
	};

	/// <summary>
	/// The package lock container
	/// </summary>
	using PackageClosure = std::map<std::string, std::map<PackageName, PackageClosureValue>>;
	using PackageClosures = std::map<std::string, PackageClosure>;
	class PackageLock
	{
	private:
		static constexpr const char* Property_Closures = "Closures";
		static constexpr const char* Property_Version = "Version";
		static constexpr const char* Property_Build = "Build";
		static constexpr const char* Property_Tool = "Tool";

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="PackageLock"/> class.
		/// </summary>
		PackageLock() :
			_table()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="PackageLock"/> class.
		/// </summary>
		PackageLock(RecipeTable table) :
			_table(std::move(table))
		{
		}

		/// <summary>
		/// Gets or sets the version
		/// </summary>
		bool HasVersion()
		{
			return HasValue(_table, Property_Version);
		}

		int64_t GetVersion()
		{
			if (!HasVersion())
				throw std::runtime_error("No version.");

			auto value = GetValue(_table, Property_Version).AsInteger();
			return value;
		}

		/// <summary>
		/// Gets or sets the table of closures
		/// </summary>
		bool HasClosures()
		{
			return HasValue(_table, Property_Closures);
		}

		PackageClosures GetClosures()
		{
			if (!HasClosures())
				throw std::runtime_error("No closures.");

			auto& values = GetValue(_table, Property_Closures).AsTable();
			auto result = PackageClosures();
			for (const auto& [closureKey, closureValue] : values)
			{
				auto closureLock = PackageClosure();
				for (const auto& [languageKey, languageValue] : closureValue.AsTable())
				{
					auto languageLock = std::map<PackageName, PackageClosureValue>();
					for (auto& [projectUniqueName, projectValue] : languageValue.AsTable())
					{
						auto& projectTable = projectValue.AsTable();

						auto projectName = PackageName::Parse(projectUniqueName);

						if (!HasValue(projectTable, Property_Version))
							throw std::runtime_error("No Version on project table.");
						auto& versionValue = GetValue(projectTable, Property_Version);

						PackageReference reference;
						if (versionValue.IsVersion())
						{
							auto version = versionValue.AsVersion();
							reference = PackageReference(std::nullopt, projectName.GetOwner(), projectName.GetName(), version);
						}
						else if (versionValue.IsString())
						{
							SemanticVersion version;
							if (SemanticVersion::TryParse(versionValue.AsString(), version))
							{
								reference = PackageReference(std::nullopt, projectName.GetOwner(), projectName.GetName(), version);
							}
							else
							{
								// Assume that the version value is a path
								reference = PackageReference(Path(versionValue.AsString()));
							}
						}
						else
						{
							throw std::runtime_error("Package version must be a Version or String.");
						}

						std::optional<std::string> buildValue = std::nullopt;
						if (HasValue(projectTable, Property_Build))
							buildValue = GetValue(projectTable, Property_Build).AsString();

						std::optional<std::string> toolValue = std::nullopt;
						if (HasValue(projectTable, Property_Tool))
							toolValue = GetValue(projectTable, Property_Tool).AsString();

						auto lockValue = PackageClosureValue(std::move(reference), std::move(buildValue), std::move(toolValue));
						languageLock.emplace(projectName, std::move(lockValue));
					}

					closureLock.emplace(languageKey, std::move(languageLock));
				}

				result.emplace(closureKey, std::move(closureLock));
			}

			return result;
		}

		/// <summary>
		/// Raw access
		/// </summary>
		RecipeTable& GetTable()
		{
			return _table;
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageLock& rhs) const
		{
			return _table == rhs._table;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageLock& rhs) const
		{
			return !(*this == rhs);
		}

	private:
		bool HasValue(const RecipeTable& table, std::string_view key) const
		{
			return table.Contains(key.data());
		}

		const RecipeValue& GetValue(const RecipeTable& table, std::string_view key) const
		{
			const RecipeValue* value;
			if (table.TryGet(key.data(), value))
			{
				return *value;
			}
			else
			{
				throw std::runtime_error("Requested package lock value does not exist in the table.");
			}
		}

		RecipeValue& GetValue(RecipeTable& table, std::string_view key)
		{
			RecipeValue* value;
			if (table.TryGet(key.data(), value))
			{
				return *value;
			}
			else
			{
				throw std::runtime_error("Requested recipe value does not exist in the table.");
			}
		}

	private:
		RecipeTable _table;
	};
}
