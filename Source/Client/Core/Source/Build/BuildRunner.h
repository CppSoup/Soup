﻿// <copyright file="BuildRunner.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "IEvaluateEngine.h"
#include "BuildConstants.h"
#include "BuildFailedException.h"
#include "PackageProvider.h"
#include "RecipeBuildArguments.h"
#include "FileSystemState.h"
#include "LocalUserConfig/LocalUserConfig.h"
#include "OperationGraph/OperationGraphManager.h"
#include "OperationGraph/OperationResultsManager.h"
#include "Utils/HandledException.h"
#include "ValueTable/ValueTableManager.h"
#include "Recipe/RecipeBuildStateConverter.h"
#include "PathList/PathListManager.h"

namespace Soup::Core
{
	/// <summary>
	/// The build runner that knows how to perform the correct build for a recipe
	/// and all of its development and runtime dependencies
	/// </summary>
	export class BuildRunner
	{
	private:
		// Root arguments
		const RecipeBuildArguments& _arguments;

		// SDK Parameters
		const ValueList& _sdkParameters;
		const std::vector<Path>& _sdkReadAccess;

		// System Parameters
		const std::vector<Path>& _systemReadAccess;

		// Shared Runtime
		RecipeCache& _recipeCache;
		PackageProvider& _packageProvider;
		IEvaluateEngine& _evaluateEngine;
		FileSystemState& _fileSystemState;
		RecipeBuildLocationManager& _locationManager;

		// Mapping from package id to the required information to be used with dependencies parameters
		std::map<PackageId, RecipeBuildCacheState> _buildCache;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildRunner"/> class.
		/// </summary>
		BuildRunner(
			const RecipeBuildArguments& arguments,
			const ValueList& sdkParameters,
			const std::vector<Path>& sdkReadAccess,
			const std::vector<Path>& systemReadAccess,
			RecipeCache& recipeCache,
			PackageProvider& packageProvider,
			IEvaluateEngine& evaluateEngine,
			FileSystemState& fileSystemState,
			RecipeBuildLocationManager& locationManager) :
			_arguments(arguments),
			_sdkParameters(sdkParameters),
			_sdkReadAccess(sdkReadAccess),
			_systemReadAccess(systemReadAccess),
			_recipeCache(recipeCache),
			_packageProvider(packageProvider),
			_evaluateEngine(evaluateEngine),
			_fileSystemState(fileSystemState),
			_locationManager(locationManager),
			_buildCache()
		{
		}

		/// <summary>
		/// The Core Execute task
		/// </summary>
		void Execute()
		{
			// TODO: A scoped listener cleanup would be nice
			try
			{
				Log::EnsureListener().SetShowEventId(true);

				// Enable log event ids to track individual builds
				auto& packageGraph = _packageProvider.GetRootPackageGraph();
				auto& packageInfo = _packageProvider.GetPackageInfo(packageGraph.RootPackageId);
				BuildPackageAndDependencies(packageGraph, packageInfo);

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
		void BuildPackageAndDependencies(const PackageGraph& packageGraph, const PackageInfo& packageInfo)
		{
			for (auto& dependencyType : packageInfo.Dependencies)
			{
				for (auto& dependency : dependencyType.second)
				{
					if (dependency.IsSubGraph)
					{
						// Load this package recipe
						auto& dependencyPackageGraph = _packageProvider.GetPackageGraph(dependency.PackageGraphId);
						auto& dependencyPackageInfo = _packageProvider.GetPackageInfo(dependencyPackageGraph.RootPackageId);

						// Build all recursive dependencies
						BuildPackageAndDependencies(dependencyPackageGraph, dependencyPackageInfo);
					}
					else
					{
						// Load this package recipe
						auto& dependencyPackageInfo = _packageProvider.GetPackageInfo(dependency.PackageId);

						// Build all recursive dependencies
						BuildPackageAndDependencies(packageGraph, dependencyPackageInfo);
					}
				}
			}

			// Build the target recipe
			CheckBuildPackage(packageGraph, packageInfo);
		}

		/// <summary>
		/// The core build that will either invoke the recipe builder directly
		/// or load a previous state
		/// </summary>
		void CheckBuildPackage(const PackageGraph& packageGraph, const PackageInfo& packageInfo)
		{
			// TODO: RAII for active id
			try
			{
				Log::SetActiveId(packageInfo.Id);
				auto languagePackageName = packageInfo.Recipe.GetLanguage().GetName() + "|" + packageInfo.Recipe.GetName();
				Log::Diag("Running Build: " + languagePackageName);

				// Check if we already built this package down a different dependency path
				if (_buildCache.contains(packageInfo.Id))
				{
					Log::Diag("Recipe already built: " + languagePackageName);
				}
				else
				{
					// Run the required builds in process
					RunBuild(packageGraph, packageInfo);
				}

				Log::SetActiveId(0);
			}
			catch(...)
			{
				Log::SetActiveId(0);
				throw;
			}
		}

		/// <summary>
		/// Setup and run the individual components of the Generate and Evaluate phases for a given package
		/// </summary>
		void RunBuild(const PackageGraph& packageGraph, const PackageInfo& packageInfo)
		{
			Log::Info("Build '" + packageInfo.Recipe.GetName() + "'");

			// Build up the expected output directory for the build to be used to cache state
			auto targetDirectory = _locationManager.GetOutputDirectory(
				packageInfo.PackageRoot,
				packageInfo.Recipe,
				packageGraph.GlobalParameters,
				_recipeCache);
			auto soupTargetDirectory = targetDirectory + BuildConstants::SoupTargetDirectory();

			// Build up the child target directory set
			auto childTargetDirectorySets = GenerateChildDependenciesTargetDirectorySet(packageInfo);
			auto& directChildTargetDirectories = childTargetDirectorySets.first;
			auto& recursiveChildTargetDirectories = childTargetDirectorySets.second;

			//////////////////////////////////////////////
			// SETUP
			/////////////////////////////////////////////

			// Load the previous operation graph and results if they exist
			Log::Info("Checking for existing Evaluate Operation Graph");
			auto evaluateGraphFile = soupTargetDirectory + BuildConstants::EvaluateGraphFileName();
			auto evaluateGraph = OperationGraph();
			auto evaluateResults = OperationResults();
			auto hasExistingGraph = OperationGraphManager::TryLoadState(
				evaluateGraphFile,
				evaluateGraph,
				_fileSystemState);
			
			if (hasExistingGraph)
			{
				Log::Info("Previous graph found");

				Log::Info("Checking for existing Evaluate Operation Results");
				auto evaluateResultsFile = soupTargetDirectory + BuildConstants::EvaluateResultsFileName();
				if (OperationResultsManager::TryLoadState(
					evaluateResultsFile,
					evaluateResults,
					_fileSystemState))
				{
					Log::Info("Previous results found");
				}
				else
				{
					Log::Info("No previous results found");
				}
			}
			else
			{
				Log::Info("No previous graph found");
			}

			//////////////////////////////////////////////
			// GENERATE
			/////////////////////////////////////////////
			if (!_arguments.SkipGenerate)
			{
				auto ranGenerate = RunIncrementalGenerate(
					packageInfo,
					targetDirectory,
					soupTargetDirectory,
					packageGraph.GlobalParameters,
					directChildTargetDirectories,
					recursiveChildTargetDirectories);

				//////////////////////////////////////////////
				// SETUP
				/////////////////////////////////////////////

				// Load the new Evaluate Operation Graph and merge any results that are still relevant
				if (ranGenerate)
				{
					Log::Info("Loading new Evaluate Operation Graph");
					auto updatedEvaluateGraph = OperationGraph();
					if (!OperationGraphManager::TryLoadState(
						evaluateGraphFile,
						updatedEvaluateGraph,
						_fileSystemState))
					{
						throw std::runtime_error("Missing required evaluate operation graph after generate evaluated.");
					}

					Log::Diag("Map previous operation graph observed results");
					auto updatedEvaluateResults = OperationResults();
					for (auto& updatedOperationEntry : updatedEvaluateGraph.GetOperations())
					{
						auto& updatedOperation = updatedOperationEntry.second;

						// Check if the new operation command existing in the previous set too
						OperationId previousOperationId;
						if (evaluateGraph.TryFindOperation(updatedOperation.Command, previousOperationId))
						{
							// Check if there is an existing result for the previous operation
							OperationResult* previousOperationResult;
							if (evaluateResults.TryFindResult(previousOperationId, previousOperationResult))
							{
								// Move this result into the updated results store
								updatedEvaluateResults.AddOrUpdateOperationResult(updatedOperation.Id, std::move(*previousOperationResult));
							}
						}
					}

					// Replace the previous operation graph and results
					evaluateGraph = std::move(updatedEvaluateGraph);
					evaluateResults = std::move(updatedEvaluateResults);
				}
			}

			//////////////////////////////////////////////
			// EVALUATE
			/////////////////////////////////////////////
			if (!_arguments.SkipEvaluate)
			{
				RunEvaluate(
					evaluateGraph,
					evaluateResults,
					targetDirectory,
					soupTargetDirectory);
			}

			// Cache the build state for upstream dependencies
			_buildCache.emplace(
				packageInfo.Id,
				RecipeBuildCacheState(
					packageInfo.Recipe.GetName(),
					std::move(targetDirectory),
					std::move(soupTargetDirectory),
					std::move(recursiveChildTargetDirectories)));
		}

		/// <summary>
		/// Run an incremental generate phase
		/// </summary>
		bool RunIncrementalGenerate(
			const PackageInfo& packageInfo,
			const Path& targetDirectory,
			const Path& soupTargetDirectory,
			const ValueTable& globalParameters,
			const std::set<Path>& directChildTargetDirectories,
			const std::set<Path>& recursiveChildTargetDirectories)
		{
			// Clone the global parameters
			auto parametersTable = ValueTable(globalParameters.GetValues());

			std::string languageExtensionPath = "";
			if (packageInfo.LanguageExtension.has_value())
			{
				languageExtensionPath = packageInfo.LanguageExtension.value().ToString();
			}

			// Set the input parameters
			parametersTable.SetValue("LanguageExtensionPath", Value(std::move(languageExtensionPath)));
			parametersTable.SetValue("PackageDirectory", Value(packageInfo.PackageRoot.ToString()));
			parametersTable.SetValue("TargetDirectory", Value(targetDirectory.ToString()));
			parametersTable.SetValue("SoupTargetDirectory", Value(soupTargetDirectory.ToString()));
			parametersTable.SetValue("Dependencies", GenerateParametersDependenciesValueTable(packageInfo));
			parametersTable.SetValue("SDKs", _sdkParameters);

			auto parametersFile = soupTargetDirectory + BuildConstants::GenerateParametersFileName();
			Log::Info("Check outdated parameters file: " + parametersFile.ToString());
			if (IsOutdated(parametersTable, parametersFile))
			{
				Log::Info("Save Parameters file");
				ValueTableManager::SaveState(parametersFile, parametersTable);
			}

			// Initialize the read access with the shared global set
			auto evaluateAllowedReadAccess = std::vector<Path>();
			auto evaluateAllowedWriteAccess = std::vector<Path>();

			// Allow read access for all sdk directories
			std::copy(
				_sdkReadAccess.begin(),
				_sdkReadAccess.end(),
				std::back_inserter(evaluateAllowedReadAccess));

			// Allow read access for all recursive dependencies target directories
			std::copy(
				recursiveChildTargetDirectories.begin(),
				recursiveChildTargetDirectories.end(),
				std::back_inserter(evaluateAllowedReadAccess));

			// Allow reading from the package root (source input) and the target directory (intermediate output)
			evaluateAllowedReadAccess.push_back(packageInfo.PackageRoot);
			evaluateAllowedReadAccess.push_back(targetDirectory);

			// Only allow writing to the target directory
			evaluateAllowedWriteAccess.push_back(targetDirectory);

			auto readAccessFile = soupTargetDirectory + BuildConstants::GenerateReadAccessFileName();
			Log::Info("Check outdated read access file: " + readAccessFile.ToString());
			if (IsOutdated(evaluateAllowedReadAccess, readAccessFile))
			{
				Log::Info("Save Read Access file");
				PathListManager::Save(readAccessFile, evaluateAllowedReadAccess);
			}

			auto writeAccessFile = soupTargetDirectory + BuildConstants::GenerateWriteAccessFileName();
			Log::Info("Check outdated write access file: " + writeAccessFile.ToString());
			if (IsOutdated(evaluateAllowedWriteAccess, writeAccessFile))
			{
				Log::Info("Save Write Access file");
				PathListManager::Save(writeAccessFile, evaluateAllowedWriteAccess);
			}

			// Run the incremental generate
			auto generateGraph = OperationGraph();

			// Add the single root operation to perform the generate
			auto moduleName = System::IProcessManager::Current().GetCurrentProcessFileName();
			auto moduleFolder = moduleName.GetParent();
			auto generateFolder = moduleFolder + Path("Generate/");
			auto generateExecutable = generateFolder + Path("Soup.Build.Generate.exe");
			OperationId generateOperationId = 1;
			auto generateArguments = std::stringstream();
			generateArguments << soupTargetDirectory.ToString();
			auto generateOperation = OperationInfo(
				generateOperationId,
				"Generate: " + packageInfo.Recipe.GetLanguage().GetName() + "|" + packageInfo.Recipe.GetName(),
				CommandInfo(
					packageInfo.PackageRoot,
					generateExecutable,
					generateArguments.str()),
				{},
				{},
				{},
				{});
			generateOperation.DependencyCount = 1;
			generateGraph.AddOperation(std::move(generateOperation));

			// Set the Generate operation as the root
			generateGraph.SetRootOperationIds({
				generateOperationId,
			});

			// Set Read and Write access fore the generate phase
			auto generateAllowedReadAccess = std::vector<Path>();
			auto generateAllowedWriteAccess = std::vector<Path>();

			// Allow read access to the generate executable folder, Windows and the DotNet install
			generateAllowedReadAccess.push_back(generateFolder);

			// Allow read from the language extension directory
			if (packageInfo.LanguageExtension.has_value())
			{
				generateAllowedReadAccess.push_back(packageInfo.LanguageExtension.value().GetParent());
			}

			// TODO: Windows specific
			generateAllowedReadAccess.push_back(Path("C:/Windows/"));
			generateAllowedReadAccess.push_back(Path("C:/Program Files/dotnet/"));

			// Allow reading from the package root (source input) and the target directory (intermediate output)
			generateAllowedReadAccess.push_back(packageInfo.PackageRoot);
			generateAllowedReadAccess.push_back(targetDirectory);

			// Allow read access for all direct dependencies target directories and write to own targets
			// TODO: This is needed to get the shared properties, this may be better to do in process
			// and only allow read access to build dependencies.
			std::copy(
				directChildTargetDirectories.begin(),
				directChildTargetDirectories.end(),
				std::back_inserter(generateAllowedReadAccess));
			generateAllowedWriteAccess.push_back(targetDirectory);

			// Load the previous build results if it exists
			Log::Info("Checking for existing Generate Operation Results");
			auto generateResultsFile = soupTargetDirectory + BuildConstants::GenerateResultsFileName();
			auto generateResults = OperationResults();
			if (OperationResultsManager::TryLoadState(
				generateResultsFile,
				generateResults,
				_fileSystemState))
			{
				Log::Info("Previous results found");
			}
			else
			{
				Log::Info("No previous results found");
			}

			// Set the temporary folder under the target folder
			auto temporaryDirectory = targetDirectory + BuildConstants::TemporaryFolderName();

			// Evaluate the Generate phase
			bool ranEvaluate = _evaluateEngine.Evaluate(
				generateGraph,
				generateResults,
				temporaryDirectory,
				generateAllowedReadAccess,
				generateAllowedWriteAccess);

			if (ranEvaluate)
			{
				// Save the generate operation results for future incremental builds
				OperationResultsManager::SaveState(generateResultsFile, generateResults, _fileSystemState);
			}

			return ranEvaluate;
		}

		void RunEvaluate(
			const OperationGraph& evaluateGraph,
			OperationResults& evaluateResults,
			const Path& targetDirectory,
			const Path& soupTargetDirectory)
		{
			// Set the temporary folder under the target folder
			auto temporaryDirectory = targetDirectory + BuildConstants::TemporaryFolderName();

			// Initialize the read access with the shared global set
			auto allowedReadAccess = std::vector<Path>();
			auto allowedWriteAccess = std::vector<Path>();

			// Allow read access from system runtime directories
			std::copy(
				_systemReadAccess.begin(),
				_systemReadAccess.end(),
				std::back_inserter(allowedReadAccess));

			// Allow read and write access to the temporary directory that is not explicitly declared
			allowedReadAccess.push_back(temporaryDirectory);
			allowedWriteAccess.push_back(temporaryDirectory);

			// TODO: REMOVE - Allow read access from SDKs
			std::copy(
				_sdkReadAccess.begin(),
				_sdkReadAccess.end(),
				std::back_inserter(allowedReadAccess));

			// Ensure the temporary directories exists
			if (!System::IFileSystem::Current().Exists(temporaryDirectory))
			{
				Log::Info("Create Directory: " + temporaryDirectory.ToString());
				System::IFileSystem::Current().CreateDirectory2(temporaryDirectory);
			}

			try
			{
				// Evaluate the build
				auto ranEvaluate = _evaluateEngine.Evaluate(
					evaluateGraph,
					evaluateResults,
					temporaryDirectory,
					allowedReadAccess,
					allowedWriteAccess);

				if (ranEvaluate)
				{
					Log::Info("Saving updated build state");
					auto evaluateResultsFile = soupTargetDirectory + BuildConstants::EvaluateResultsFileName();
					OperationResultsManager::SaveState(evaluateResultsFile, evaluateResults, _fileSystemState);
				}
			}
			catch(const BuildFailedException&)
			{
				Log::Info("Saving partial build state");
				auto evaluateResultsFile = soupTargetDirectory + BuildConstants::EvaluateResultsFileName();
				OperationResultsManager::SaveState(evaluateResultsFile, evaluateResults, _fileSystemState);
				throw;
			}

			Log::Info("Done");
		}

		bool IsOutdated(const ValueTable& parametersTable, const Path& parametersFile)
		{
			// Load up the existing parameters file and check if our state matches the previous
			// to ensure incremental builds function correctly
			auto previousParametersState = ValueTable();
			if (ValueTableManager::TryLoadState(parametersFile, previousParametersState))
			{
				return previousParametersState != parametersTable;
			}
			else
			{
				return true;
			}
		}

		bool IsOutdated(const std::vector<Path>& fileList, const Path& pathListFile)
		{
			// Load up the existing path list file and check if our state matches the previous
			// to ensure incremental builds function correctly
			auto previousFileList = std::vector<Path>();
			if (PathListManager::TryLoad(pathListFile, previousFileList))
			{
				return previousFileList != fileList;
			}
			else
			{
				return true;
			}
		}

		ValueTable GenerateParametersDependenciesValueTable(
			const PackageInfo& packageInfo)
		{
			auto result = ValueTable();

			for (auto& dependencyType : packageInfo.Dependencies)
			{
				auto& dependencyTypeTable = result.SetValue(dependencyType.first, Value(ValueTable())).AsTable();
				for (auto& dependency : dependencyType.second)
				{
					// Load this package recipe
					auto dependencyPackageId = dependency.PackageId;
					if (dependency.IsSubGraph)
					{
						auto& dependencyPackageGraph = _packageProvider.GetPackageGraph(dependency.PackageGraphId);
						dependencyPackageId = dependencyPackageGraph.RootPackageId;
					}

					auto& dependencyPackageInfo = _packageProvider.GetPackageInfo(dependencyPackageId);

					// Grab the build state for upstream dependencies
					auto findBuildCache = _buildCache.find(dependencyPackageInfo.Id);
					if (findBuildCache != _buildCache.end())
					{
						auto& dependencyState = findBuildCache->second;
						dependencyTypeTable.SetValue(
							dependencyState.Name,
							Value(ValueTable({
								{
									"Reference",
									Value(dependency.OriginalReference.ToString())
								},
								{
									"TargetDirectory",
									Value(dependencyState.TargetDirectory.ToString())
								},
								{
									"SoupTargetDirectory",
									Value(dependencyState.SoupTargetDirectory.ToString())
								},
							})));
					}
					else
					{
						Log::Error("Dependency does not exist in build cache: " + dependencyPackageInfo.PackageRoot.ToString());
						throw std::runtime_error("Dependency does not exist in build cache: " + dependencyPackageInfo.PackageRoot.ToString());
					}
				}
			}

			return result;
		}

		std::pair<std::set<Path>, std::set<Path>> GenerateChildDependenciesTargetDirectorySet(
			const PackageInfo& packageInfo)
		{
			auto directDirectories = std::set<Path>();
			auto recursiveDirectories = std::set<Path>();
			for (auto& dependencyType : packageInfo.Dependencies)
			{
				for (auto& dependency : dependencyType.second)
				{
					// Load this package recipe
					auto dependencyPackageId = dependency.PackageId;
					if (dependency.IsSubGraph)
					{
						auto& dependencyPackageGraph = _packageProvider.GetPackageGraph(dependency.PackageGraphId);
						dependencyPackageId = dependencyPackageGraph.RootPackageId;
					}

					auto& dependencyPackageInfo = _packageProvider.GetPackageInfo(dependencyPackageId);

					// Grab the build state for upstream dependencies
					auto findBuildCache = _buildCache.find(dependencyPackageInfo.Id);
					if (findBuildCache != _buildCache.end())
					{
						// Combine the child dependency target and the recursive children
						auto& dependencyState = findBuildCache->second;
						directDirectories.insert(dependencyState.TargetDirectory);
						recursiveDirectories.insert(dependencyState.TargetDirectory);
						recursiveDirectories.insert(dependencyState.RecursiveChildTargetDirectorySet.begin(), dependencyState.RecursiveChildTargetDirectorySet.end());
					}
					else
					{
						Log::Error("Dependency does not exist in build cache: " + dependencyPackageInfo.PackageRoot.ToString());
						throw std::runtime_error("Dependency does not exist in build cache: " + dependencyPackageInfo.PackageRoot.ToString());
					}
				}
			}

			return std::make_pair(std::move(directDirectories), std::move(recursiveDirectories));
		}
	};
}