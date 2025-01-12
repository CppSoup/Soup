// <copyright file="RecipeExtensions.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "Recipe.h"
#include "RecipeSML.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe extensions
	/// </summary>
	export class RecipeExtensions
	{
	public:
		/// <summary>
		/// Attempt to load from file
		/// </summary>
		static bool TryLoadRecipeFromFile(
			const Path& recipeFile,
			Recipe& result)
		{
			// Open the file to read from
			Log::Diag("Load Recipe: {}", recipeFile.ToString());
			std::shared_ptr<System::IInputFile> file;
			if (!System::IFileSystem::Current().TryOpenRead(recipeFile, true, file))
			{
				Log::Info("Recipe file does not exist.");
				return false;
			}

			// Read the contents of the recipe file
			try
			{
				result = Recipe(
					RecipeSML::Deserialize(
						recipeFile,
						file->GetInStream()));
				return true;
			}
			catch (std::exception& ex)
			{
				Log::Error("Deserialize Threw: {}", ex.what());
				Log::Info("Failed to parse Recipe.");
				return false;
			}
		}

		/// <summary>
		/// Build up the recipe file location from the package reference
		/// </summary>
		static Path GetPackageRecipeFile(
			const Path& workingDirectory,
			const PackageReference& package)
		{
			if (package.IsLocal())
			{
				auto recipeFile = workingDirectory + package.GetPath();
				return recipeFile;
			}
			else
			{
				throw std::runtime_error("Non-local packages not supported.");
			}
		}

		/// <summary>
		/// Save the recipe to file
		/// </summary>
		static void SaveToFile(
			const Path& recipeFile,
			Recipe& recipe)
		{
			// Open the file to write to
			auto file = System::IFileSystem::Current().OpenWrite(recipeFile, false);

			// Write the recipe to the file stream
			RecipeSML::Serialize(recipe.GetTable(), file->GetOutStream());
		}
	};
}