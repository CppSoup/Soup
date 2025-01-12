// <copyright file="RecipeCache.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "recipe/RecipeExtensions.h"
#include "recipe/RootRecipeExtensions.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe cache that maintains an in memory collection of recipes to prevent loading multiple instances from disk
	/// </summary>
	export class RecipeCache
	{
	private:
		std::map<std::string, Recipe> _knownRecipes;
		std::map<std::string, RootRecipe> _knownRootRecipes;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="RecipeCache"/> class.
		/// </summary>
		RecipeCache() :
			_knownRecipes(),
			_knownRootRecipes()
		{
		}

		RecipeCache(std::map<std::string, Recipe> knownRecipes) :
			_knownRecipes(std::move(knownRecipes)),
			_knownRootRecipes()
		{
		}

		bool TryGetRootRecipe(
			const Path& recipeFile,
			const RootRecipe*& result)
		{
			// Check if the recipe was already loaded
			auto findRecipe = _knownRootRecipes.find(recipeFile.ToString());
			if (findRecipe != _knownRootRecipes.end())
			{
				result = &findRecipe->second;
				return true;
			}
			else
			{
				RootRecipe loadRecipe;
				if (RootRecipeExtensions::TryLoadRootRecipeFromFile(recipeFile, loadRecipe))
				{
					// Save the recipe for later
					auto [insertRecipeIterator, wasInserted] = _knownRootRecipes.emplace(
						recipeFile.ToString(),
						std::move(loadRecipe));

					result = &insertRecipeIterator->second;
					return true;
				}
				else
				{
					// Failed to load this recipe
					return false;
				}
			}
		}

		const Recipe& GetRecipe(const Path& recipeFile)
		{
			// The Recipe must already be loaded
			auto findRecipe = _knownRecipes.find(recipeFile.ToString());
			if (findRecipe != _knownRecipes.end())
			{
				return findRecipe->second;
			}
			else
			{
				throw std::runtime_error(
					std::format("Recipe [{}] not found in closure", recipeFile.ToString()));
			}
		}

		bool TryGetOrLoadRecipe(
			const Path& recipeFile,
			const Recipe*& result)
		{
			// Check if the recipe was already loaded
			auto findRecipe = _knownRecipes.find(recipeFile.ToString());
			if (findRecipe != _knownRecipes.end())
			{
				result = &findRecipe->second;
				return true;
			}
			else
			{
				Recipe loadRecipe;
				if (RecipeExtensions::TryLoadRecipeFromFile(recipeFile, loadRecipe))
				{
					// Save the recipe for later
					auto [insertRecipeIterator, wasInserted] = _knownRecipes.emplace(
						recipeFile.ToString(),
						std::move(loadRecipe));

					result = &insertRecipeIterator->second;
					return true;
				}
				else
				{
					// Failed to load this recipe
					return false;
				}
			}
		}
	};
}
