// <copyright file="Recipe.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "RecipeValue.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe container
	/// </summary>
	export class Recipe
	{
	private:
		static constexpr const char* Property_Dependencies = "Dependencies";
		static constexpr const char* Property_Language = "Language";
		static constexpr const char* Property_Name = "Name";
		static constexpr const char* Property_Version = "Version";
		static constexpr const char* Property_Reference = "Reference";

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="Recipe"/> class.
		/// </summary>
		Recipe() :
			_table()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Recipe"/> class.
		/// </summary>
		Recipe(RecipeTable table) :
			_table(std::move(table))
		{
		}

		/// <summary>
		/// Gets or sets the package name
		/// </summary>
		bool HasName() const
		{
			return HasValue(_table, Property_Name);
		}

		const std::string& GetName() const
		{
			auto& nameValue = GetValue(_table, Property_Name);
			if (nameValue.IsString())
				return nameValue.AsString();
			else
				throw std::runtime_error("The Recipe name must be of type String");
		}

		/// <summary>
		/// Gets or sets the package language
		/// </summary>
		bool HasLanguage() const
		{
			return HasValue(_table, Property_Language);
		}

		LanguageReference GetLanguage() const
		{
			auto& languageValue = GetValue(_table, Property_Language);
			if (languageValue.IsString())
			{
				LanguageReference result;
				if (LanguageReference::TryParse(languageValue.AsString(), result))
				{
					return result;
				}
				else
				{
					throw std::runtime_error(
						std::format("Invalid Language format in Recipe: {}", languageValue.AsString()));
				}
			}
			else if (languageValue.IsLanguageReference())
			{
				return languageValue.AsLanguageReference();
			}
			else
			{
				throw std::runtime_error("The Recipe language must be of type String or LanguageReference");
			}
		}

		/// <summary>
		/// Gets or sets the package version
		/// </summary>
		bool HasVersion() const
		{
			return HasValue(_table, Property_Version);
		}

		SemanticVersion GetVersion() const
		{
			if (!HasVersion())
				throw std::runtime_error("No version.");

			auto& versionValue = GetValue(_table, Property_Version);
			if (versionValue.IsString())
				return SemanticVersion::Parse(versionValue.AsString());
			else if (versionValue.IsVersion())
				return versionValue.AsVersion();
			else
				throw std::runtime_error("The Recipe version must be of type String or Version");
		}

		/// <summary>
		/// Gets or sets the list of named dependency packages
		/// </summary>
		std::vector<std::string> GetDependencyTypes() const
		{
			auto result = std::vector<std::string>();
			if (HasDependencies())
			{
				for (const auto& [key, value] : GetDependencies())
				{
					result.push_back(key);
				}
			}

			return result;
		}

		bool HasNamedDependencies(std::string_view name) const
		{
			return HasDependencies() && HasValue(GetDependencies(), name);
		}

		std::vector<PackageReference> GetNamedDependencies(std::string_view name) const
		{
			if (!HasNamedDependencies(name))
				throw std::runtime_error("No named dependencies.");

			auto& values = GetValue(GetDependencies(), name).AsList();
			auto result = std::vector<PackageReference>();
			for (auto& value : values)
			{
				// A dependency can either be a string or a table with reference key
				if (value.IsString())
				{
					result.push_back(PackageReference::Parse(value.AsString()));
				}
				else if (value.IsPackageReference())
				{
					result.push_back(value.AsPackageReference());
				}
				else if (value.IsTable())
				{
					auto& valueTable = value.AsTable();
					if (!HasValue(valueTable, Property_Reference))
						throw std::runtime_error("Recipe dependency table missing required Reference value.");
					auto& referenceValue = GetValue(valueTable, Property_Reference);
					if (referenceValue.IsString())
					{
						result.push_back(PackageReference::Parse(referenceValue.AsString()));
					}
					else if (referenceValue.IsPackageReference())
					{
						result.push_back(referenceValue.AsPackageReference());
					}
					else
					{
						throw std::runtime_error("Recipe dependency Reference must be type String.");
					}
				}
				else
				{
					throw std::runtime_error("Unknown Recipe dependency type.");
				}
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
		bool operator ==(const Recipe& rhs) const
		{
			return _table == rhs._table;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const Recipe& rhs) const
		{
			return !(*this == rhs);
		}

	private:
		/// <summary>
		/// Gets or sets the table of dependency packages
		/// </summary>
		bool HasDependencies() const
		{
			return HasValue(_table, Property_Dependencies);
		}

		const RecipeTable& GetDependencies() const
		{
			if (!HasDependencies())
				throw std::runtime_error("No dependencies.");

			auto& values = GetValue(_table, Property_Dependencies).AsTable();
			return values;
		}

		RecipeTable& EnsureDependencies()
		{
			auto& value = EnsureTableValue(_table, Property_Dependencies);
			return value.AsTable();
		}

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
				throw std::runtime_error("Requested recipe value does not exist in the table.");
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

		RecipeValue& SetValue(RecipeTable& table, std::string_view key, RecipeValue&& value)
		{
			auto [wasInserted, valueReference] = table.TryInsert(key.data(), std::move(value));
			if (wasInserted)
			{
				return *valueReference;
			}
			else
			{
				throw std::runtime_error("Failed to insert a recipe value.");
			}
		}

		RecipeValue& EnsureTableValue(RecipeTable& table, std::string_view key)
		{
			RecipeValue* value;
			if (table.TryGet(key.data(), value))
			{
				if (value->GetType() != RecipeValueType::Table)
				{
					throw std::runtime_error("The recipe already has a non-table dependencies property");
				}

				return *value;
			}
			else
			{
				auto [wasInserted, valueReference] = table.TryInsert(key.data(), RecipeValue(RecipeTable()));
				if (wasInserted)
				{
					return *valueReference;
				}
				else
				{
					throw std::runtime_error("Failed to insert a recipe value.");
				}
			}
		}

	private:
		RecipeTable _table;
	};
}
