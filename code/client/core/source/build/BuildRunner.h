// <copyright file="BuildRunner.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "IEvaluateEngine.h"
#include "PackageProvider.h"
#include "RecipeBuildArguments.h"
#include "RecipeBuildLocationManager.h"
#include "local-user-config/LocalUserConfig.h"
#include "operation-graph/OperationGraphManager.h"
#include "recipe/RecipeBuildStateConverter.h"

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

		// System Parameters
		Path _userDataPath;
		const std::vector<Path>& _systemReadAccess;

		// Shared Runtime
		RecipeCache& _recipeCache;
		PackageProvider& _packageProvider;
		IEvaluateEngine& _evaluateEngine;
		FileSystemState& _fileSystemState;
		RecipeBuildLocationManager& _locationManager;

		// Mapping from package id to the required information to be used with dependencies parameters
		std::map<PackageId, RecipeBuildCacheState> _buildCache;

		const std::string _dependencyTypeBuild = "Build";
		const std::string _dependencyTypeTool = "Tool";

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildRunner"/> class.
		/// </summary>
		BuildRunner(
			const RecipeBuildArguments& arguments,
			Path userDataPath,
			const std::vector<Path>& systemReadAccess,
			RecipeCache& recipeCache,
			PackageProvider& packageProvider,
			IEvaluateEngine& evaluateEngine,
			FileSystemState& fileSystemState,
			RecipeBuildLocationManager& locationManager) :
			_arguments(arguments),
			_userDataPath(std::move(userDataPath)),
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
			if (packageInfo.IsPrebuilt)
			{
				if (_buildCache.contains(packageInfo.Id))
				{
					Log::Diag("Prebuilt Package was already processed");
				}
				else
				{
					// Cache the build state for upstream dependencies
					Log::Diag("Package was prebuilt: {}", packageInfo.Name.ToString());
					_buildCache.emplace(
						packageInfo.Id,
						RecipeBuildCacheState(
							packageInfo.Name.ToString(),
							Path(std::format("/(TARGET_{})/", packageInfo.Name.ToString())),
							packageInfo.TargetDirectory,
							packageInfo.TargetDirectory + Path("./.soup/"),
							{},
							{}));
				}
			}
			else
			{
				for (auto& [dependencyType, dependencyTypeSet] : packageInfo.Dependencies)
				{
					for (auto& dependency : dependencyTypeSet)
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
				Log::Diag("Running Build: [{}]{}", packageInfo.Recipe->GetLanguage().GetName(), packageInfo.Name.ToString());

				// Check if we already built this package down a different dependency path
				if (_buildCache.contains(packageInfo.Id))
				{
					Log::Diag("Recipe already built: [{}]{}", packageInfo.Recipe->GetLanguage().GetName(), packageInfo.Name.ToString());
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
			Log::Info("Build '{}'", packageInfo.Name.ToString());

			// Build up the expected output directory for the build to be used to cache state
			auto macroPackageDirectory = Path(
				std::format("/(PACKAGE_{})/", packageInfo.Name.ToString()));
			auto macroTargetDirectory = Path(
				std::format("/(TARGET_{})/", packageInfo.Name.ToString()));
			auto realTargetDirectory = _locationManager.GetOutputDirectory(
				packageInfo.Name,
				packageInfo.PackageRoot,
				*packageInfo.Recipe,
				packageGraph.GlobalParameters,
				_recipeCache);
			auto soupTargetDirectory = realTargetDirectory + BuildConstants::SoupTargetDirectory();

			// Build up the set of directories and macros that grant access to the generate/evaluate phases
			auto packageAccessSet = GenerateAccessSet(
				packageInfo,
				macroPackageDirectory,
				macroTargetDirectory,
				realTargetDirectory);

			// Preload target
			// TODO: Ideally this should be done in the preload step, but easier here with the graph id
			_fileSystemState.PreloadDirectory(realTargetDirectory, false);

			//////////////////////////////////////////////
			// SETUP
			/////////////////////////////////////////////

			// Load the previous operation graph and results if they exist
			auto evaluateGraphFile = soupTargetDirectory + BuildConstants::EvaluateGraphFileName();
			Log::Info("Checking for existing Evaluate Operation Graph");
			Log::Diag(evaluateGraphFile.ToString());
			auto evaluateGraph = OperationGraph();
			auto evaluateResults = OperationResults();
			auto hasExistingGraph = OperationGraphManager::TryLoadState(
				evaluateGraphFile,
				evaluateGraph,
				_fileSystemState);
			
			if (hasExistingGraph)
			{
				Log::Info("Previous graph found");

				auto evaluateResultsFile = soupTargetDirectory + BuildConstants::EvaluateResultsFileName();
				Log::Info("Checking for existing Evaluate Operation Results");
				Log::Diag(evaluateResultsFile.ToString());
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
				// Ensure the target directories exists
				if (!System::IFileSystem::Current().Exists(soupTargetDirectory))
				{
					Log::Info("Create Directory: {}", soupTargetDirectory.ToString());
					System::IFileSystem::Current().CreateDirectory(soupTargetDirectory);
				}

				auto ranGenerate = RunIncrementalGenerate(
					packageInfo,
					macroPackageDirectory,
					macroTargetDirectory,
					realTargetDirectory,
					soupTargetDirectory,
					packageGraph.GlobalParameters,
					packageAccessSet);

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
					auto updatedEvaluateResults = MergeOperationResults(
						evaluateGraph,
						evaluateResults,
						updatedEvaluateGraph);

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
					realTargetDirectory,
					soupTargetDirectory);
			}

			// Cache the build state for upstream dependencies
			_buildCache.emplace(
				packageInfo.Id,
				RecipeBuildCacheState(
					packageInfo.Name.ToString(),
					std::move(macroTargetDirectory),
					std::move(realTargetDirectory),
					std::move(soupTargetDirectory),
					std::move(packageAccessSet.EvaluateRecursiveReadDirectories),
					std::move(packageAccessSet.EvaluateRecursiveMacros)));
		}

		/// <summary>
		/// Run an incremental generate phase
		/// </summary>
		bool RunIncrementalGenerate(
			const PackageInfo& packageInfo,
			const Path& macroPackageDirectory,
			const Path& macroTargetDirectory,
			const Path& realTargetDirectory,
			const Path& soupTargetDirectory,
			const ValueTable& globalParameters,
			const DependencyTargetSet& packageAccessSet)
		{
			// Clone the global parameters
			auto inputTable = ValueTable();

			// Pass along internal dependency information
			inputTable.emplace("Dependencies", GenerateInputDependenciesValueTable(packageInfo));

			// Setup input that will be included in the global state
			auto globalState = ValueTable();

			// Setup environment context generated by build runner
			auto context = ValueTable();
			context.emplace("PackageDirectory", macroPackageDirectory.ToString());
			context.emplace("TargetDirectory", macroTargetDirectory.ToString());
			context.emplace("HostPlatform", _arguments.HostPlatform);
			globalState.emplace("Context", std::move(context));

			// Pass along the parameters
			globalState.emplace("Parameters", globalParameters);

			// Generate the dependencies input state
			globalState.emplace("Dependencies", GenerateParametersDependenciesValueTable(packageInfo));

			// Pass along the file system state
			auto fileSystemRoot = BuildDirectoryStructure(packageInfo.PackageRoot);
			globalState.emplace("FileSystem", std::move(fileSystemRoot));

			inputTable.emplace("GlobalState", std::move(globalState));

			// Build up the input state for the generate call
			auto generateMacros = ValueTable();
			auto generateSubGraphMacros = ValueTable();
			auto evaluateAllowedReadAccess = ValueList();
			auto evaluateAllowedWriteAccess = ValueList();
			auto evaluateMacros = ValueTable();

			// Allow generate to resolve generate macros
			for (auto& [key, value] : packageAccessSet.GenerateCurrentMacros)
				generateMacros.emplace(key, value);

			// Make subgraph macros unique
			for (auto& [key, value] : packageAccessSet.GenerateSubGraphMacros)
				generateSubGraphMacros.emplace(key, value);

			// Pass along the read access set
			for (auto& value : packageAccessSet.EvaluateCurrentReadDirectories)
				evaluateAllowedReadAccess.push_back(value.ToString());
			for (auto& value : packageAccessSet.EvaluateRecursiveReadDirectories)
				evaluateAllowedReadAccess.push_back(value.ToString());

			// Pass along the write access set
			for (auto& value : packageAccessSet.EvaluateCurrentWriteDirectories)
				evaluateAllowedWriteAccess.push_back(value.ToString());

			// Allow generate to resolve evaluate macros
			for (auto& [key, value] : packageAccessSet.EvaluateCurrentMacros)
				evaluateMacros.emplace(key, value);
			for (auto& [key, value] : packageAccessSet.EvaluateRecursiveMacros)
				evaluateMacros.emplace(key, value);

			inputTable.emplace("PackageRoot", packageInfo.PackageRoot.ToString());
			inputTable.emplace("UserDataPath", _userDataPath.ToString());
			inputTable.emplace("GenerateMacros", std::move(generateMacros));
			inputTable.emplace("GenerateSubGraphMacros", std::move(generateSubGraphMacros));
			inputTable.emplace("EvaluateReadAccess", std::move(evaluateAllowedReadAccess));
			inputTable.emplace("EvaluateWriteAccess", std::move(evaluateAllowedWriteAccess));
			inputTable.emplace("EvaluateMacros", std::move(evaluateMacros));

			auto inputFile = soupTargetDirectory + BuildConstants::GenerateInputFileName();
			Log::Info("Check outdated generate input file: {}", inputFile.ToString());
			if (IsOutdated(inputTable, inputFile))
			{
				Log::Info("Save Generate Input file");
				ValueTableManager::SaveState(inputFile, inputTable);
			}

			// Run the incremental generate
			auto generateGraph = OperationGraph();

			// Add the single root operation to perform the generate
			auto moduleName = System::IProcessManager::Current().GetCurrentProcessFileName();
			auto generateFolder = moduleName.GetParent();

			#if defined(_WIN32)
			auto generateExecutable = generateFolder + Path("./Soup.Generate.exe");
			#elif defined(__linux__)
			auto generateExecutable = generateFolder + Path("./generate");
			#else
			#error "Unknown platform"
			#endif

			OperationId generateOperationId = 1;
			auto generateArguments = std::vector<std::string>();
			generateArguments.push_back(soupTargetDirectory.ToString());
			auto generateOperation = OperationInfo(
				generateOperationId,
				std::format("Generate: [{}]{}", packageInfo.Recipe->GetLanguage().GetName(), packageInfo.Name.ToString()),
				CommandInfo(
					packageInfo.PackageRoot,
					generateExecutable,
					std::move(generateArguments)),
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

			// Allow read access to the generate executable folder
			generateAllowedReadAccess.push_back(generateFolder);

			// Allow read access to the local user config
			auto localUserConfigPath = _userDataPath + BuildConstants::LocalUserConfigFileName();
			generateAllowedReadAccess.push_back(std::move(localUserConfigPath));

			// TODO: Windows specific
			generateAllowedReadAccess.push_back(Path("C:/Windows/"));
			generateAllowedReadAccess.push_back(Path("C:/Program Files/dotnet/"));

			// Pass along the read access set
			for (auto& value : packageAccessSet.GenerateCurrentReadDirectories)
				generateAllowedReadAccess.push_back(value);

			// Pass along the write access set
			for (auto& value : packageAccessSet.GenerateCurrentWriteDirectories)
				generateAllowedWriteAccess.push_back(value);

			// Load the previous build results if it exists
			auto generateResultsFile = soupTargetDirectory + BuildConstants::GenerateResultsFileName();
			Log::Info("Checking for existing Generate Operation Results");
			Log::Diag(generateResultsFile.ToString());
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
			auto temporaryDirectory = realTargetDirectory + BuildConstants::TemporaryFolderName();

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

		OperationResults MergeOperationResults(
			const OperationGraph& previousGraph,
			OperationResults& previousResults,
			const OperationGraph& updatedGraph)
		{
			auto updatedResults = OperationResults();
			for (auto& [operationId, updatedOperation] : updatedGraph.GetOperations())
			{
				// Check if the new operation command existing in the previous set too
				OperationId previousOperationId;
				if (previousGraph.TryFindOperation(updatedOperation.Command, previousOperationId))
				{
					// Check if there is an existing result for the previous operation
					OperationResult* previousOperationResult;
					if (previousResults.TryFindResult(previousOperationId, previousOperationResult))
					{
						// Move this result into the updated results store
						updatedResults.AddOrUpdateOperationResult(updatedOperation.Id, std::move(*previousOperationResult));
					}
				}
			}

			return updatedResults;
		}

		void RunEvaluate(
			const OperationGraph& evaluateGraph,
			OperationResults& evaluateResults,
			const Path& realTargetDirectory,
			const Path& soupTargetDirectory)
		{
			// Set the temporary folder under the target folder
			auto temporaryDirectory = realTargetDirectory + BuildConstants::TemporaryFolderName();

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

			// Ensure the temporary directories exists
			if (!System::IFileSystem::Current().Exists(temporaryDirectory))
			{
				Log::Info("Create Directory: {}", temporaryDirectory.ToString());
				System::IFileSystem::Current().CreateDirectory(temporaryDirectory);
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

		ValueTable GenerateInputDependenciesValueTable(
			const PackageInfo& packageInfo)
		{
			auto result = ValueTable();

			for (const auto& [dependencyType, dependencyTypeSet] : packageInfo.Dependencies)
			{
				auto dependencyTypeTable = ValueTable();
				for (auto& dependency : dependencyTypeSet)
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
						dependencyTypeTable.emplace(
							dependencyState.Name,
							Value(ValueTable({
								{
									"SoupTargetDirectory",
									Value(dependencyState.SoupTargetDirectory.ToString())
								},
							})));
					}
					else
					{
						Log::Error("Dependency does not exist in build cache: {}", dependencyPackageInfo.PackageRoot.ToString());
						throw std::runtime_error(
							std::format(
								"Dependency does not exist in build cache: {}",
								dependencyPackageInfo.PackageRoot.ToString()));
					}
				}

				result.emplace(dependencyType, Value(std::move(dependencyTypeTable)));
			}

			return result;
		}

		ValueTable GenerateParametersDependenciesValueTable(
			const PackageInfo& packageInfo)
		{
			auto result = ValueTable();

			for (const auto& [dependencyType, dependencyTypeSet] : packageInfo.Dependencies)
			{
				auto dependencyTypeTable = ValueTable();
				for (auto& dependency : dependencyTypeSet)
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
						auto contextTable = ValueTable({
							{
								"Reference",
								Value(dependency.OriginalReference.ToString())
							},
							{
								"TargetDirectory",
								Value(dependencyState.MacroTargetDirectory.ToString())
							},
						});

						dependencyTypeTable.emplace(
							dependencyState.Name,
							Value(ValueTable({
								{ "Context", std::move(contextTable) },
							})));
					}
					else
					{
						Log::Error("Dependency does not exist in build cache: {}", dependencyPackageInfo.PackageRoot.ToString());
						throw std::runtime_error(
							std::format(
								"Dependency does not exist in build cache: {}",
								dependencyPackageInfo.PackageRoot.ToString()));
					}
				}

				result.emplace(dependencyType, Value(std::move(dependencyTypeTable)));
			}

			return result;
		}

		DependencyTargetSet GenerateAccessSet(
			const PackageInfo& packageInfo,
			const Path& macroPackageDirectory,
			const Path& macroTargetDirectory,
			const Path& realTargetDirectory)
		{
			auto targetSet = DependencyTargetSet();

			// Allow reading from the package root (Recipe) during generate
			targetSet.GenerateCurrentReadDirectories.insert(packageInfo.PackageRoot);

			// Allow reading and writing to the package target (Input/Output) during generate
			targetSet.GenerateCurrentReadDirectories.insert(realTargetDirectory);
			targetSet.GenerateCurrentWriteDirectories.insert(realTargetDirectory);

			// Only this build has read access to the package source directory during evaluate
			targetSet.EvaluateCurrentReadDirectories.insert(macroPackageDirectory);
			targetSet.EvaluateCurrentMacros.emplace(
				macroPackageDirectory.ToString(),
				packageInfo.PackageRoot.ToString());

			// This and all recursive packages will have read access to the target directory
			// and the current package has write permissions
			targetSet.EvaluateRecursiveReadDirectories.insert(macroTargetDirectory);
			targetSet.EvaluateCurrentWriteDirectories.insert(macroTargetDirectory);
			targetSet.EvaluateRecursiveMacros.emplace(
				macroTargetDirectory.ToString(),
				realTargetDirectory.ToString());

			for (auto& [dependencyType, dependencyTypeSet] : packageInfo.Dependencies)
			{
				for (auto& dependency : dependencyTypeSet)
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

						if (dependency.IsSubGraph)
						{
							// Check known types for subgraphs
							if (dependencyType == _dependencyTypeBuild)
							{
								// Replace with unique macro to prevent collisions
								auto macroBuildTargetDirectory = Path(
									std::format("/(BUILD_TARGET_{})/", dependencyPackageInfo.Name.ToString()));
								targetSet.GenerateSubGraphMacros.emplace(
									dependencyState.MacroTargetDirectory.ToString(),
									macroBuildTargetDirectory.ToString());

								// Allow read access for all direct build dependencies target directories
								// and macros during generate. This is needed to load the shared properties.
								targetSet.GenerateCurrentReadDirectories.insert(dependencyState.RealTargetDirectory);
								targetSet.GenerateCurrentMacros.emplace(
									macroBuildTargetDirectory.ToString(),
									dependencyState.RealTargetDirectory.ToString());
							}
							else if (dependencyType == _dependencyTypeTool)
							{
								// Replace with unique macro to prevent collisions
								auto macroToolTargetDirectory = Path(
									std::format("/(TOOL_TARGET_{})/", dependencyPackageInfo.Name.ToString()));
								targetSet.GenerateSubGraphMacros.emplace(
									dependencyState.MacroTargetDirectory.ToString(),
									macroToolTargetDirectory.ToString());

								// Allow read access for all indirect tool dependencies target directories
								// and macros during generate and evaluate. This is needed to load the shared properties and run them.
								targetSet.GenerateCurrentReadDirectories.insert(dependencyState.RealTargetDirectory);
								targetSet.EvaluateCurrentReadDirectories.insert(dependencyState.RealTargetDirectory);
								targetSet.EvaluateCurrentMacros.emplace(
									macroToolTargetDirectory.ToString(),
									dependencyState.RealTargetDirectory.ToString());
							}
							else
							{
								throw std::runtime_error(
									std::format("Invalid subgraph dependency type: {}", dependencyType));
							}
						}
						else
						{
							// Within the same graph share all read directories and macros to upstream dependencies
							targetSet.EvaluateRecursiveReadDirectories.insert(
								dependencyState.RecursiveChildMacroTargetDirectorySet.begin(),
								dependencyState.RecursiveChildMacroTargetDirectorySet.end());
							targetSet.EvaluateRecursiveMacros.insert(
								dependencyState.RecursiveChildMacros.begin(),
								dependencyState.RecursiveChildMacros.end());

							// Allow read access for all direct runtime dependencies target directories
							// and macros during generate. This is needed to load the shared properties.
							targetSet.GenerateCurrentReadDirectories.insert(dependencyState.RealTargetDirectory);
							targetSet.GenerateCurrentMacros.emplace(
								dependencyState.MacroTargetDirectory.ToString(),
								dependencyState.RealTargetDirectory.ToString());
						}
					}
					else
					{
						Log::Error("Dependency does not exist in build cache: {}", dependencyPackageInfo.PackageRoot.ToString());
						throw std::runtime_error(
							std::format("Dependency does not exist in build cache: {}", dependencyPackageInfo.PackageRoot.ToString()));
					}
				}
			}

			return targetSet;
		}

		ValueList BuildDirectoryStructure(const Path& directory)
		{
			auto directoryState = _fileSystemState.GetDirectoryState(directory);

			auto result = ValueList();
			BuildDirectoryStructure(directoryState, result);

			return result;
		}

		void BuildDirectoryStructure(DirectoryState& activeDirectory, ValueList& result)
		{
			for (auto& file : activeDirectory.Files)
			{
				result.push_back(file);
			}

			if (!activeDirectory.ChildDirectories.empty())
			{
				auto childDirectories = ValueTable();
				for (auto& childDirectory : activeDirectory.ChildDirectories)
				{
					auto childDirectoryStructure = ValueList();
					BuildDirectoryStructure(childDirectory.second, childDirectoryStructure);
					childDirectories.emplace(
						childDirectory.first,
						std::move(childDirectoryStructure));
				}

				result.push_back(std::move(childDirectories));
			}
		}
	};
}
