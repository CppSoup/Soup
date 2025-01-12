// <copyright file="RecipeBuildStateConverter.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "RecipeValue.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe build state converter that converts between a recipe table and a value table
	/// </summary>
	export class RecipeBuildStateConverter
	{
	public:
		/// <summary>
		/// Convert the recipe internal representation to initial build state
		/// </summary>
		static ValueTable ConvertToBuildState(const RecipeTable& table)
		{
			auto result = ValueTable();
			for (const auto& [key, value] : table)
			{
				auto buildValue = ConvertToBuildState(value);
				result.emplace(key, std::move(buildValue));
			}

			return result;
		}

	private:
		/// <summary>
		/// Convert the recipe internal representation to initial build state
		/// </summary>
		static ValueList ConvertToBuildState(const RecipeList& list)
		{
			auto result = ValueList();
			for (auto& value : list)
			{
				auto buildValue = ConvertToBuildState(value);
				result.push_back(std::move(buildValue));
			}

			return result;
		}

		/// <summary>
		/// Convert the recipe internal representation to initial build state
		/// </summary>
		static Value ConvertToBuildState(const RecipeValue& value)
		{
			switch (value.GetType())
			{
				case RecipeValueType::Table:
					return Value(ConvertToBuildState(value.AsTable()));
				case RecipeValueType::List:
					return Value(ConvertToBuildState(value.AsList()));
				case RecipeValueType::String:
					return Value(value.AsString());
				case RecipeValueType::Integer:
					return Value(value.AsInteger());
				case RecipeValueType::Float:
					return Value(value.AsFloat());
				case RecipeValueType::Boolean:
					return Value(value.AsBoolean());
				case RecipeValueType::Version:
					return Value(value.AsVersion());
				case RecipeValueType::PackageReference:
					return Value(value.AsPackageReference());
				case RecipeValueType::LanguageReference:
					return Value(value.AsLanguageReference());
				default:
					throw std::runtime_error("Unknown value type.");
			}
		}
	};
}
