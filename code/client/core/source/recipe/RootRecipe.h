// <copyright file="RootRecipe.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "recipe/RecipeValue.h"

namespace Soup::Core
{
	/// <summary>
	/// The root recipe container
	/// </summary>
	export class RootRecipe
	{
	private:
		static constexpr const char* Property_OutputRoot = "OutputRoot";

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="RootRecipe"/> class.
		/// </summary>
		RootRecipe() :
			_table()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="Recipe"/> class.
		/// </summary>
		RootRecipe(RecipeTable table) :
			_table(std::move(table))
		{
		}

		/// <summary>
		/// Gets or sets the output root
		/// </summary>
		bool HasOutputRoot() const
		{
			return HasValue(Property_OutputRoot);
		}

		Path GetOutputRoot() const
		{
			if (!HasOutputRoot())
				throw std::runtime_error("No OutputRoot.");

			return Path(
				GetValue(Property_OutputRoot).AsString());
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
		bool operator ==(const RootRecipe& rhs) const
		{
			return _table == rhs._table;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const RootRecipe& rhs) const
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
			if (_table.TryGet(key.data(), value))
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
