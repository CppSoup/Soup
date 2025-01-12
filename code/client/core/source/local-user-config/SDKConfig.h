// <copyright file="SDKConfig.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "recipe/RecipeValue.h"

namespace Soup::Core
{
	/// <summary>
	/// The SDK config container
	/// </summary>
	export class SDKConfig
	{
	private:
		static constexpr const char* Property_Name = "Name";
		static constexpr const char* Property_SourceDirectories = "SourceDirectories";
		static constexpr const char* Property_Properties = "Properties";

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="SDKConfig"/> class.
		/// </summary>
		SDKConfig() :
			_table()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="SDKConfig"/> class.
		/// </summary>
		SDKConfig(RecipeTable table) :
			_table(std::move(table))
		{
		}

		/// <summary>
		/// Gets or sets the Name
		/// </summary>
		bool HasName() const
		{
			return HasValue(Property_Name);
		}

		std::string GetName() const
		{
			if (!HasName())
				throw std::runtime_error("No Name.");

			return GetValue(Property_Name).AsString();
		}

		/// <summary>
		/// Gets or sets the list of Source Directories
		/// </summary>
		bool HasSourceDirectories()
		{
			return HasValue(Property_SourceDirectories);
		}

		std::vector<Path> GetSourceDirectories()
		{
			if (!HasSourceDirectories())
				throw std::runtime_error("No SDKs.");

			auto& values = GetValue(Property_SourceDirectories).AsList();
			auto result = std::vector<Path>();
			for (auto& value : values)
			{
				result.push_back(Path(value.AsString()));
			}

			return result;
		}

		/// <summary>
		/// Gets or sets the properties table
		/// </summary>
		bool HasProperties()
		{
			return HasValue(Property_Properties);
		}

		RecipeTable GetProperties()
		{
			if (!HasSourceDirectories())
				throw std::runtime_error("No Properties.");

			return GetValue(Property_Properties).AsTable();
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
		bool operator ==(const SDKConfig& rhs) const
		{
			return _table == rhs._table;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const SDKConfig& rhs) const
		{
			return !(*this == rhs);
		}

	private:
		bool HasValue(std::string_view key) const
		{
			return _table.Contains(key.data());
		}

		const RecipeValue& GetValue(std::string_view key) const
		{
			const RecipeValue* value;
			if ( _table.TryGet(key.data(), value))
			{
				return *value;
			}
			else
			{
				throw std::runtime_error("Requested recipe value does not exist in the root table.");
			}
		}

	private:
		RecipeTable _table;
	};
}
