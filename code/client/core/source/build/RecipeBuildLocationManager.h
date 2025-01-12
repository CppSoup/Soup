// <copyright file="RecipeBuildLocationManager.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "recipe/Recipe.h"
#include "PackageProvider.h"
#include "RecipeBuildCacheState.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe build location manager that knows how to generate the unique folder for building a 
	/// Recipe with a given set of parameters
	/// </summary>
	export class RecipeBuildLocationManager
	{
	private:
		// Known languages
		const std::map<std::string, KnownLanguage>& _knownLanguageLookup;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="RecipeBuildLocationManager"/> class.
		/// </summary>
		RecipeBuildLocationManager(
			const std::map<std::string, KnownLanguage>& knownLanguageLookup) :
			_knownLanguageLookup(knownLanguageLookup)
		{
		}

		Path GetOutputDirectory(
			const PackageName& name,
			const Path& packageRoot,
			const Recipe& recipe,
			const ValueTable& globalParameters,
			RecipeCache& recipeCache)
		{
			// Set the default output directory to be relative to the package
			auto rootOutput = packageRoot + Path("./out/");

			// Check for root recipe file with overrides
			Path rootRecipeFile;
			if (RootRecipeExtensions::TryFindRootRecipeFile(packageRoot, rootRecipeFile))
			{
				Log::Info("Found Root Recipe: '{}'", rootRecipeFile.ToString());
				const RootRecipe* rootRecipe;
				if (!recipeCache.TryGetRootRecipe(rootRecipeFile, rootRecipe))
				{
					// Nothing we can do, exit
					Log::Error("Failed to load the root recipe file: {}", rootRecipeFile.ToString());
					throw HandledException(222);
				}

				// Check if there was a root output set
				if (rootRecipe->HasOutputRoot())
				{
					// Relative to the root recipe file itself
					rootOutput = rootRecipe->GetOutputRoot();

					// Add the language sub folder
					auto language = recipe.GetLanguage().GetName();
					rootOutput = rootOutput + Path(std::format("./{}/", language));

					if (name.HasOwner())
					{
						// Add the unique owner
						rootOutput = rootOutput + Path(std::format("./{}/", name.GetOwner()));
					}
					else
					{
						// Label as local
						rootOutput = rootOutput + Path("./Local/");
					}

					// Add the unique recipe name/version
					rootOutput = rootOutput +
						Path(std::format("./{}/{}/", name.GetName(), recipe.GetVersion().ToString()));

					// Ensure there is a root relative to the file itself
					if (!rootOutput.HasRoot())
					{
						rootOutput = rootRecipeFile.GetParent() + rootOutput;
					}

					Log::Info("Override root output: {}", rootOutput.ToString());
				}
			}

			// Add unique folder name for parameters
			auto parametersStream = std::stringstream();
			ValueTableWriter::Serialize(globalParameters, parametersStream);
			auto hashParameters = CryptoPP::Sha1::HashBase64(parametersStream.str());
			auto uniqueParametersFolder = Path(std::format("./{}/", hashParameters));
			rootOutput = rootOutput + uniqueParametersFolder;

			return rootOutput;
		}
	};
}