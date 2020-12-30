﻿// <copyright file="RecipeBuildRunner.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "RecipeBuildManager.h"
#include "RecipeBuildArguments.h"

namespace Soup::Build
{
	/// <summary>
	/// The recipe build runner that knows how to perform the correct build for a recipe
	/// and all of its development and runtime dependencies
	/// </summary>
	export class RecipeBuildRunner
	{
	public:
		static Path GetConfigurationDirectory(
			std::string_view compiler,
			std::string_view flavor,
			std::string_view system,
			std::string_view architecture)
		{
			// Setup the output directories
			return Path(compiler) +
				Path(flavor) +
				Path(system) +
				Path(architecture);
		}

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="RecipeBuildRunner"/> class.
		/// </summary>
		RecipeBuildRunner(
			std::string hostCompiler,
			std::string runtimeCompiler,
			RecipeBuildArguments arguments) :
			_hostCompiler(std::move(hostCompiler)),
			_runtimeCompiler(std::move(runtimeCompiler)),
			_arguments(std::move(arguments)),
			_buildManager(),
			_buildSet(),
			_hostBuildSet(),
			_hostBuildPaths(),
			_fileSystemState(std::make_shared<Runtime::FileSystemState>())
		{
		}

		/// <summary>
		/// The Core Execute task
		/// </summary>
		void Execute(const Path& workingDirectory)
		{
			// Enable log event ids to track individual builds
			int projectId = 1;
			bool isHostBuild = false;
			Log::EnsureListener().SetShowEventId(true);

			// TODO: A scoped listener cleanup would be nice
			try
			{
				auto recipePath = workingDirectory + Runtime::BuildConstants::RecipeFileName();
				Runtime::Recipe recipe = {};
				if (!_buildManager.TryGetRecipe(recipePath, recipe))
				{
					Log::Error("The target Recipe does not exist: " + recipePath.ToString());
					Log::HighPriority("Make sure the path is correct and try again");

					// Nothing we can do, exit
					throw HandledException(1123124);
				}

				auto rootParentSet = std::set<std::string>();
				projectId = BuildRecipeAndDependencies(
					projectId,
					workingDirectory,
					recipe,
					isHostBuild,
					rootParentSet);

				Log::EnsureListener().SetShowEventId(false);
			}
			catch(...)
			{
				Log::EnsureListener().SetShowEventId(false);
				throw;
			}
		}

	private:
		/// <summary>
		/// Build the dependencies for the provided recipe recursively
		/// </summary>
		int BuildRecipeAndDependencies(
			int projectId,
			const Path& workingDirectory,
			Runtime::Recipe& recipe,
			bool isHostBuild,
			const std::set<std::string>& parentSet)
		{
			// Add current package to the parent set when building child dependencies
			auto activeParentSet = parentSet;
			activeParentSet.insert(std::string(recipe.GetName()));

			auto knownDependecyTypes = std::array<std::string_view, 3>({
				"Runtime",
				"Test",
				"Build",
			});

			for (auto knownDependecyType : knownDependecyTypes)
			{
				if (recipe.HasNamedDependencies(knownDependecyType))
				{
					for (auto dependency : recipe.GetNamedDependencies(knownDependecyType))
					{
						// Load this package recipe
						auto packagePath = GetPackageReferencePath(workingDirectory, dependency);
						auto packageRecipePath = packagePath + Runtime::BuildConstants::RecipeFileName();
						Runtime::Recipe dependencyRecipe = {};
						if (!_buildManager.TryGetRecipe(packageRecipePath, dependencyRecipe))
						{
							if (dependency.IsLocal())
							{
								Log::Error("The dependency Recipe does not exist: " + packageRecipePath.ToString());
								Log::HighPriority("Make sure the path is correct and try again");
							}
							else
							{
								Log::Error("The dependency Recipe version has not been installed: " + dependency.ToString());
								Log::HighPriority("Run `install` and try again");
							}

							// Nothing we can do, exit
							throw HandledException(1234);
						}

						// Ensure we do not have any circular dependencies
						if (activeParentSet.contains(dependencyRecipe.GetName()))
						{
							Log::Error("Found circular dependency: " + recipe.GetName() + " -> " + dependencyRecipe.GetName());
							throw std::runtime_error("BuildRecipeAndDependencies: Circular dependency.");
						}

						// Build all recursive dependencies
						bool isDependencyHostBuild = isHostBuild || knownDependecyType == "Build";
						projectId = BuildRecipeAndDependencies(
							projectId,
							packagePath,
							dependencyRecipe,
							isDependencyHostBuild,
							activeParentSet);
					}
				}
			}

			// Build the root recipe
			projectId = CheckBuildRecipe(
				projectId,
				workingDirectory,
				recipe,
				isHostBuild);

			// Return the updated project id after building all dependencies
			return projectId;
		}

		/// <summary>
		/// The core build that will either invoke the recipe builder directly
		/// or load a previous state
		/// </summary>
		int CheckBuildRecipe(
			int projectId,
			const Path& workingDirectory,
			Runtime::Recipe& recipe,
			bool isHostBuild)
		{
			// TODO: RAII for active id
			try
			{
				Log::SetActiveId(projectId);
				Log::Diag("Running Build");

				// Select the correct build set to ensure that the different build properties 
				// required the same project to be build twice
				auto& buildSet = isHostBuild ? _hostBuildSet : _buildSet;
				auto findBuildState = buildSet.find(recipe.GetName());
				if (findBuildState != buildSet.end())
				{
					// Verify the project name is unique
					if (findBuildState->second != workingDirectory)
					{
						Log::Error("Recipe with this name already exists: " + recipe.GetName() + " [" + workingDirectory.ToString() + "] [" + findBuildState->second.ToString() + "]");
						throw std::runtime_error("Recipe name not unique");
					}
					else
					{
						Log::Diag("Recipe already built: " + recipe.GetName());
					}
				}
				else
				{
					// Run the required builds in process
					// This will break the circular requirements for the core build libraries
					RunBuild(
						workingDirectory,
						recipe,
						isHostBuild);

					// Keep track of the packages we have already built
					auto insertBuildState = buildSet.emplace(
						recipe.GetName(),
						workingDirectory);

					// Replace the find iterator so it can be used to update the shared table state
					findBuildState = insertBuildState.first;

					// Move to the next build project id
					projectId++;
				}

				Log::SetActiveId(0);
			}
			catch(...)
			{
				Log::SetActiveId(0);
				throw;
			}

			return projectId;
		}

		/// <summary>
		/// Setup and run the individual components of the Generate and Evaluate phases for a given package
		/// </summary>
		void RunBuild(
			const Path& packageRoot,
			Runtime::Recipe& recipe,
			bool isHostBuild)
		{
			// Select the correct compiler to use
			std::string activeCompiler = "";
			std::string activeFlavor = "";
			std::string activeArchitecture = _arguments.Architecture;
			std::string activeSystem = _arguments.System;
			if (isHostBuild)
			{
				Log::HighPriority("Host Build '" + recipe.GetName() + "'");
				activeCompiler = _hostCompiler;
				activeFlavor = "release";
			}
			else
			{
				Log::HighPriority("Build '" + recipe.GetName() + "'");
				activeCompiler = _runtimeCompiler;
				activeFlavor = _arguments.Flavor;
			}

			// Set the default output directory to be relative to the package
			auto rootOutput = packageRoot + Path("out/");

			// Add unique location for host builds
			if (isHostBuild)
			{
				rootOutput = rootOutput + Path("HostBuild/");
			}

			// Check for root recipe file with overrides
			Path rootRecipeFile;
			if (RootRecipeExtensions::TryFindRootRecipeFile(packageRoot, rootRecipeFile))
			{
				Log::Info("Found Root Recipe: '" + rootRecipeFile.ToString() + "'");
				RootRecipe rootRecipe;
				if (!_buildManager.TryGetRootRecipe(rootRecipeFile, rootRecipe))
				{
					// Nothing we can do, exit
					Log::Error("Failed to load the root recipe file: " + rootRecipeFile.ToString());
					throw HandledException(222);
				}

				// Today the only unique thing it can do is set the shared output directory
				if (rootRecipe.HasOutputRoot())
				{
					// Relative to the root recipe file itself
					rootOutput = rootRecipe.GetOutputRoot();

					// Add unique location for host builds
					if (isHostBuild)
					{
						rootOutput = rootOutput + Path("HostBuild/");
					}

					// Add the unique recipe name
					rootOutput = rootOutput + Path(recipe.GetName() + "/");

					// Ensure there is a root relative to the file itself
					if (!rootOutput.HasRoot())
					{
						rootOutput = rootRecipeFile.GetParent() + rootOutput;
					}

					Log::Info("Override root output: " + rootOutput.ToString());
				}
			}

			// Build up the expected output directory for the build to be used to cache state
			auto targetDirectory = rootOutput + GetConfigurationDirectory(
				activeCompiler,
				activeFlavor,
				activeSystem,
				activeArchitecture);

			if (!_arguments.SkipGenerate)
			{
				RunIncrementalGenerate(
					packageRoot,
					targetDirectory,
					activeArchitecture,
					activeCompiler,
					activeFlavor,
					activeSystem);
			}

			if (!_arguments.SkipEvaluate)
			{
				RunEvaluate(targetDirectory);
			}
		}

		/// <summary>
		/// Run an incremental generate phase
		/// </summary>
		void RunIncrementalGenerate(
			const Path& packageDirectory,
			const Path& targetDirectory,
			std::string_view architecture,
			std::string_view compiler,
			std::string_view flavor,
			std::string_view system)
		{
			auto soupTargetDirectory = targetDirectory + GetSoupTargetDirectory();

			// Set the input parameters
			auto parametersTable = Runtime::ValueTable();
			parametersTable.SetValue("PackageDirectory", Runtime::Value(packageDirectory.ToString()));
			parametersTable.SetValue("TargetDirectory", Runtime::Value(targetDirectory.ToString()));
			parametersTable.SetValue("SoupTargetDirectory", Runtime::Value(soupTargetDirectory.ToString()));

			parametersTable.SetValue("Architecture", Runtime::Value(std::string(architecture)));
			parametersTable.SetValue("Compiler", Runtime::Value(std::string(compiler)));
			parametersTable.SetValue("Flavor", Runtime::Value(std::string(flavor)));
			parametersTable.SetValue("System", Runtime::Value(std::string(system)));

			auto parametersFile = soupTargetDirectory + Runtime::BuildConstants::GenerateParametersFileName();
			Log::Info("Check outdated parameters file: " + parametersFile.ToString());
			if (IsOutdated(parametersTable, parametersFile))
			{
				Log::Info("Save Parameters file");
				Runtime::ValueTableManager::SaveState(parametersFile, parametersTable);
			}

			// Run the incremental generate
			auto generateGraph = Runtime::OperationGraph();

			// Add the single root operation to perform the generate
			auto moduleName = System::IProcessManager::Current().GetCurrentProcessFileName();
			auto moduleFolder = moduleName.GetParent();
			auto generateExecutable = moduleFolder + Path("Soup.Generate.exe");
			Runtime::OperationId generateOperatioId = 1;
			auto generateArguments = std::stringstream();
			generateArguments << soupTargetDirectory.ToString();
			auto generateOperation = Runtime::OperationInfo(
				generateOperatioId,
				"Generate Phase",
				Runtime::CommandInfo(
					packageDirectory,
					generateExecutable,
					generateArguments.str()),
				{},
				{});
			generateOperation.DependencyCount = 1;
			generateGraph.AddOperation(std::move(generateOperation));

			// Set the Generate operation as the root
			generateGraph.SetRootOperationIds({
				generateOperatioId,
			});

			// Load the previous build graph if it exists and merge it with the new one
			auto generateGraphFile = soupTargetDirectory + GetGenerateGraphFileName();
			Runtime::OperationGraphManager::TryMergeExisting(generateGraphFile, generateGraph, *_fileSystemState);

			// Evaluate the Generate phase
			auto evaluateGenerateEngine = Runtime::BuildEvaluateEngine(
				_fileSystemState,
				generateGraph);
			evaluateGenerateEngine.Evaluate();

			// Save the operation graph for future incremental builds
			Runtime::OperationGraphManager::SaveState(generateGraphFile, generateGraph, *_fileSystemState);
		}

		bool IsOutdated(const Runtime::ValueTable& parametersTable, const Path& parametersFile)
		{
			// Load up the existing parameters file and check if our state matches the previous
			// to ensure incremental builds function correctly
			auto previousParametersState = Runtime::ValueTable();
			if (Runtime::ValueTableManager::TryLoadState(parametersFile, previousParametersState))
			{
				return previousParametersState != parametersTable;
			}
			else
			{
				return true;
			}
		}

		void RunEvaluate(const Path& targetDirectory)
		{
			// Load and run the previous stored state directly
			Log::Info("Loading evaluate operation graph");
			auto soupTargetDirectory = targetDirectory + GetSoupTargetDirectory();
			auto evaluateGraphFile = soupTargetDirectory + Runtime::BuildConstants::EvaluateOperationGraphFileName();
			auto evaluateGraph = Runtime::OperationGraph();
			if (!Runtime::OperationGraphManager::TryLoadState(
				evaluateGraphFile,
				evaluateGraph,
				*_fileSystemState))
			{
				throw std::runtime_error("Missing cached operation graph for evaluate phase.");
			}

			if (_arguments.ForceRebuild)
			{
				Log::Diag("Purge operation graph to force build");
				for (auto& operation : evaluateGraph.GetOperations())
				{
					auto& operationInfo = operation.second;
					operationInfo.WasSuccessfulRun = false;
					operationInfo.ObservedInput = {};
					operationInfo.ObservedOutput = {};
				}
			}

			try
			{
				// Evaluate the build
				auto evaluateEngine = Runtime::BuildEvaluateEngine(
					_fileSystemState,
					evaluateGraph);
				evaluateEngine.Evaluate();
			}
			catch(const Runtime::BuildFailedException&)
			{
				Log::Info("Saving partial build state");
				Runtime::OperationGraphManager::SaveState(
					evaluateGraphFile,
					evaluateGraph,
					*_fileSystemState);
				throw;
			}

			Log::Info("Saving updated build state");
			Runtime::OperationGraphManager::SaveState(
				evaluateGraphFile,
				evaluateGraph,
				*_fileSystemState);

			Log::HighPriority("Done");
		}

		Path GetSoupUserDataPath() const
		{
			auto result = System::IFileSystem::Current().GetUserProfileDirectory() +
				Path(".soup/");
			return result;
		}

		Path GetPackageReferencePath(const Path& workingDirectory, const Runtime::PackageReference& reference) const
		{
			// If the path is relative then combine with the working directory
			Path packagePath;
			if (reference.IsLocal())
			{
				packagePath = reference.GetPath();
				if (!packagePath.HasRoot())
				{
					packagePath = workingDirectory + packagePath;
				}
			}
			else
			{
				auto packageStore = GetSoupUserDataPath() +
					Path("packages/");
				packagePath = packageStore + Path(reference.GetName()) + Path(reference.GetVersion().ToString());
			}

			return packagePath;
		}

	private:
		static Path GetSoupTargetDirectory()
		{
			static const auto value = Path(".soup/");
			return value;
		}

		static Path GetGenerateGraphFileName()
		{
			static const auto value = Path("GenerateGraph.bog");
			return value;
		}

	private:
		std::string _hostCompiler;
		std::string _runtimeCompiler;
		RecipeBuildArguments _arguments;

		RecipeBuildManager _buildManager;

		std::map<std::string, Path> _buildSet;
		std::map<std::string, Path> _hostBuildSet;

		std::map<std::string, Path> _hostBuildPaths;

		std::shared_ptr<Runtime::FileSystemState> _fileSystemState;
	};
}
