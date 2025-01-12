// <copyright file="BuildEngine.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "BuildRunner.h"
#include "BuildEvaluateEngine.h"
#include "BuildLoadEngine.h"
#include "local-user-config/LocalUserConfigExtensions.h"

namespace Soup::Core
{
	/// <summary>
	/// The core build system that brings together the individual build components to build a single root package
	/// </summary>
	export class BuildEngine
	{
	public:
		static std::map<std::string, KnownLanguage> GetKnownLanguages()
		{
			auto result = std::map<std::string, KnownLanguage>(
			{
				{
					"C",
					KnownLanguage("Soup", "C")
				},
				{
					"C++",
					KnownLanguage("Soup", "Cpp")
				},
				{
					"C#",
					KnownLanguage("Soup", "CSharp")
				},
				{
					"Wren",
					KnownLanguage("Soup", "Wren")
				},
			});

			return result;
		}

		static std::map<std::string, std::map<PackageName, SemanticVersion>> GetBuiltInPackages()
		{
			auto result = std::map<std::string, std::map<PackageName, SemanticVersion>>(
			{
				{
					"C++",
					{
						{
							PackageName("mwasplund", "copy"),
							SemanticVersion(1, 1, 0)
						},
						{
							PackageName("mwasplund", "mkdir"),
							SemanticVersion(1, 1, 0)
						},
					}
				},
				{
					"Wren",
					{
						{
							PackageName("Soup", "Wren"),
							SemanticVersion(0, 4, 3)
						},
					}
				},
			});

			return result;
		}

		/// <summary>
		/// Load the build graph
		/// </summary>
		static PackageProvider LoadBuildGraph(
			const Path& builtInDirectory,
			const Path& workingDirectory,
			const ValueTable& targetGlobalParameters,
			const Path& userDataPath,
			RecipeCache& recipeCache)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			// Load the system specific state
			auto hostGlobalParameters = LoadHostSystemState();

			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

			// Log::Info("LoadSystemState: {} seconds", duration.count());

			startTime = std::chrono::high_resolution_clock::now();

			// Generate the package build graph
			auto knownLanguages = GetKnownLanguages();
			auto builtInPackages = GetBuiltInPackages();
			auto loadEngine = BuildLoadEngine(
				builtInDirectory,
				knownLanguages,
				builtInPackages,
				targetGlobalParameters,
				hostGlobalParameters,
				userDataPath,
				recipeCache);
			auto packageProvider = loadEngine.Load(workingDirectory);

			endTime = std::chrono::high_resolution_clock::now();
			duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

			// Log::Info("BuildLoadEngine: {} seconds", duration.count());

			return packageProvider;
		}

		/// <summary>
		/// Preload the file system
		/// </summary>
		static FileSystemState PreloadFileSystemState(
			PackageProvider& packageProvider)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			// Initialize a shared File System State to cache file system access
			auto fileSystemState = FileSystemState();

			for (auto package : packageProvider.GetPackageLookup())
			{
				fileSystemState.PreloadDirectory(package.second.PackageRoot, true);
				// TODO: fileSystemState.PreloadDirectory(package.second.TargetDirectory, false);
			}

			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

			// Log::Info("PreloadFileSystemState: {} seconds", duration.count());

			startTime = std::chrono::high_resolution_clock::now();

			return fileSystemState;
		}

		static void Execute(
			PackageProvider& packageProvider,
			const RecipeBuildArguments& arguments,
			const Path& userDataPath,
			RecipeCache& recipeCache)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			// Initialize shared location manager
			auto knownLanguages = GetKnownLanguages();
			auto locationManager = RecipeBuildLocationManager(knownLanguages);

			// Load the system specific state
			auto systemReadAccess = LoadHostSystemAccess();

			// Load the file system state
			auto fileSystemState = PreloadFileSystemState(packageProvider);

			// Initialize a shared Evaluate Engine
			auto evaluateEngine = BuildEvaluateEngine(
				arguments.ForceRebuild,
				arguments.DisableMonitor,
				arguments.PartialMonitor,
				fileSystemState);

			// Initialize the build runner that will perform the generate and evaluate phase
			// for each individual package
			auto buildRunner = BuildRunner(
				arguments,
				userDataPath,
				systemReadAccess,
				recipeCache,
				packageProvider,
				evaluateEngine,
				fileSystemState,
				locationManager);
			buildRunner.Execute();

			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);

			// Log::Info("BuildRunner: {} seconds", duration.count());
		}

		static Path GetSoupUserDataPath()
		{
			auto result = System::IFileSystem::Current().GetUserProfileDirectory() +
				BuildConstants::SoupLocalStoreDirectory();
			return result;
		}

	private:
		static ValueTable LoadHostSystemState()
		{
			auto hostGlobalParameters = ValueTable();

			auto system = std::string("Windows");
			hostGlobalParameters.emplace("System", Value(system));

			return hostGlobalParameters;
		}

		static std::vector<Path> LoadHostSystemAccess()
		{
			auto systemReadAccess = std::vector<Path>();

			// Allow read access from system directories
			systemReadAccess.push_back(
				Path("C:/Windows/"));

			return systemReadAccess;
		}
	};
}
