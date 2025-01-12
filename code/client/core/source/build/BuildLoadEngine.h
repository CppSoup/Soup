// <copyright file="BuildLoadEngine.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "package-lock/PackageLockExtensions.h"
#include "RecipeBuildArguments.h"
#include "recipe/RecipeCache.h"

namespace Soup::Core
{
	struct PackageLockState
	{
		bool HasPackageLock;
		Path RootDirectory;
		PackageClosures Closures;
	};

	using KnownPackageMap = std::map<PackageIdentifier, std::pair<PackageId, Path>>;

	/// <summary>
	/// The build load engine that loads the package build graph.
	/// It is responsible for loading up the entire closure and validating build state to
	/// create a package graph that can be evaluated by the build runner.
	/// </summary>
	export class BuildLoadEngine
	{
	private:
		const int _packageLockVersion = 5;
		const Path _builtInPackageOutPath = Path("./out/");
		const std::string _builtInWrenLanguage = "Wren";
		const std::string _dependencyTypeBuild = "Build";
		const std::string _dependencyTypeTool = "Tool";
		const std::string _rootClosureName = "Root";

		// Built ins
		const Path& _builtInPackageDirectory;
		const std::map<std::string, KnownLanguage>& _knownLanguageLookup;
		const std::map<std::string, std::map<PackageName, SemanticVersion>>& _builtInPackageLookup;

		// Arguments
		const ValueTable& _targetBuildGlobalParameters;

		// System Parameters
		Path _userDataPath;
		const ValueTable& _hostBuildGlobalParameters;

		// Shared Runtime State
		RecipeCache& _recipeCache;
		std::map<std::string, PackageLockState> _knownPackageLocks;

		int _uniquePackageId;
		int _uniqueGraphId;

		// The package build graph results
		PackageGraphLookupMap _packageGraphLookup;
		PackageLookupMap _packageLookup;

		// Mapping from sub graph dependency to Package path to graph id
		std::map<Path, std::pair<PackageGraphId, std::vector<PackageChildInfo>>> _knownSubGraphSet;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildLoadEngine"/> class.
		/// </summary>
		BuildLoadEngine(
			const Path& builtInPackageDirectory,
			const std::map<std::string, KnownLanguage>& knownLanguageLookup,
			const std::map<std::string, std::map<PackageName, SemanticVersion>>& builtInPackageLookup,
			const ValueTable& targetBuildGlobalParameters,
			const ValueTable& hostBuildGlobalParameters,
			Path userDataPath,
			RecipeCache& recipeCache) :
			_builtInPackageDirectory(builtInPackageDirectory),
			_knownLanguageLookup(knownLanguageLookup),
			_builtInPackageLookup(builtInPackageLookup),
			_targetBuildGlobalParameters(targetBuildGlobalParameters),
			_hostBuildGlobalParameters(hostBuildGlobalParameters),
			_userDataPath(std::move(userDataPath)),
			_recipeCache(recipeCache),
			_knownPackageLocks(),
			_packageGraphLookup(),
			_packageLookup(),
			_knownSubGraphSet()
		{
		}

		/// <summary>
		/// Load the package lock and using it recursively load up all packages that are a part of the build closure
		/// Validates that there are no circular dependencies and all required packages are available
		/// </summary>
		PackageProvider Load(const Path& projectRoot)
		{
			// Load the package lock from project folder
			const auto& packageLockState = LoadPackageLock(projectRoot);

			// There is no parent, create empty state
			auto parentPackageLockState = PackageLockState();

			auto recipePath = projectRoot + BuildConstants::RecipeFileName();
			const Recipe* recipe;
			if (!_recipeCache.TryGetOrLoadRecipe(recipePath, recipe))
			{
				Log::Error("The target Recipe does not exist: {}", recipePath.ToString());
				Log::HighPriority("Make sure the path is correct and try again");

				// Nothing we can do, exit
				throw HandledException(1123124);
			}

			// Create the root build graph
			_uniqueGraphId = 0;
			_uniquePackageId = 0;
			PackageGraphId rootGraphId = ++_uniqueGraphId;
			PackageId rootPackageId = ++_uniquePackageId;

			// Save the package graph
			_packageGraphLookup.emplace(
				rootGraphId,
				PackageGraph(rootGraphId, rootPackageId, _targetBuildGlobalParameters));

			// Build up the unique identifier
			auto packageIdentifier = PackageIdentifier(
				recipe->GetLanguage().GetName(),
				std::nullopt,
				recipe->GetName());

			auto parentSet = std::set<PackageName>();
			auto knownPackageSet = KnownPackageMap();
			auto toolDependencyProjects = std::vector<PackageChildInfo>();
			std::optional<std::string> rootBuildToolClosureName = std::nullopt;
			LoadClosure(
				rootBuildToolClosureName,
				packageIdentifier,
				*recipe,
				projectRoot,
				rootPackageId,
				parentSet,
				knownPackageSet,
				packageLockState,
				parentPackageLockState,
				toolDependencyProjects);

			for (auto& toolDependency : toolDependencyProjects)
				Log::Warning("Top Level Tool Dependency discarded: {}", toolDependency.OriginalReference.ToString());

			return PackageProvider(rootGraphId, std::move(_packageGraphLookup), std::move(_packageLookup));
		}

	private:
		const PackageLockState& LoadPackageLock(const Path& projectRoot)
		{
			auto packageLockPath = projectRoot + BuildConstants::PackageLockFileName();

			// Check if the package lock has already been processed from another graph
			auto findKnownPackageLock = _knownPackageLocks.find(packageLockPath.ToString());
			if (findKnownPackageLock != _knownPackageLocks.end())
			{
				Log::Diag("Package Lock already loaded: {}", packageLockPath.ToString());
				return findKnownPackageLock->second;
			}
			else
			{
				// Load the package lock if present
				auto packageLockState = PackageLockState();
				packageLockState.HasPackageLock = false;
				PackageLock packageLock = {};
				if (PackageLockExtensions::TryLoadFromFile(packageLockPath, packageLock))
				{
					Log::Info("Package lock loaded");
					if (packageLock.GetVersion() == _packageLockVersion)
					{
						packageLockState.RootDirectory = projectRoot;
						packageLockState.Closures = packageLock.GetClosures();
						packageLockState.HasPackageLock = true;
					}
					else
					{
						Log::Error("Package lock version is out of date");
						Log::HighPriority("Run `restore` and try again");

						// Nothing we can do, exit
						throw HandledException(91781861);
					}
				}
				else
				{
					Log::Error("Missing or invalid package lock");
					Log::HighPriority("Run `restore` and try again");

					// Nothing we can do, exit
					throw HandledException(9183917);
				}

				// Save the package lock
				auto result = _knownPackageLocks.emplace(
					packageLockPath.ToString(),
					std::move(packageLockState));

				return result.first->second;
			}
		}

		std::pair<std::string, std::string> GetPackageSubGraphsClosure(
			const PackageIdentifier& packageIdentifier,
			const PackageLockState& packageLockState) const
		{
			if (!packageLockState.HasPackageLock)
			{
				return std::make_pair("", "");
			}

			// Find the required closure
			auto findClosure = packageLockState.Closures.find(_rootClosureName);
			if (findClosure == packageLockState.Closures.end())
			{
				throw std::runtime_error(
					std::format(
						"Closure [{}] not found in lock [{}]",
						_rootClosureName,
						packageLockState.RootDirectory.ToString()));
			}

			// Find the package version in the lock
			auto findPackageLock = findClosure->second.find(packageIdentifier.GetLanguage());
			if (findPackageLock == findClosure->second.end())
			{
				throw std::runtime_error(
					std::format(
						"Language [{}] [{}] not found in lock [{}]",
						_rootClosureName,
						packageIdentifier.GetLanguage(),
						packageLockState.RootDirectory.ToString()));
			}

			auto packageVersion = findPackageLock->second.find(packageIdentifier.GetPackageName());
			if (packageVersion == findPackageLock->second.end())
			{
				throw std::runtime_error(
					std::format(
						"Package [{}] [{}] not found in lock [{}]",
						_rootClosureName,
						packageIdentifier.ToString(),
						packageLockState.RootDirectory.ToString()));
			}

			auto& packageBuild = packageVersion->second.BuildValue;
			auto& packageTool = packageVersion->second.ToolValue;
			if (!packageBuild.has_value())
			{
				throw std::runtime_error(
					std::format(
						"Package [{}] [{}] does not have build closure [{}]",
						_rootClosureName,
						packageIdentifier.ToString(),
						packageLockState.RootDirectory.ToString()));
			}

			if (!packageTool.has_value())
			{
				throw std::runtime_error(
					std::format(
						"Package [{}] [{}] does not have tool closure [{}]",
						_rootClosureName,
						packageIdentifier.ToString(),
						packageLockState.RootDirectory.ToString()));
			}

			return std::make_pair(packageBuild.value(), packageTool.value());
		}

		Path GetPackageReferencePath(
			const PackageReference& activeReference,
			const PackageLockState& packageLockState) const
		{
			Path packagePath;
			if (activeReference.IsLocal())
			{
				// Use local reference relative to lock directory
				packagePath = activeReference.GetPath();
				if (!packagePath.HasRoot())
				{
					packagePath = packageLockState.RootDirectory + packagePath;
				}
			}
			else
			{
				// Build the global store location path
				auto packageStore = _userDataPath + Path("./packages/");
				auto& languageName = activeReference.GetLanguage();
				auto activeVersionString = activeReference.GetVersion().ToString();
				packagePath = packageStore +
					Path(
						std::format(
							"./{}/{}/{}/{}/",
							languageName,
							activeReference.GetOwner(),
							activeReference.GetName(),
							activeVersionString));
			}

			return packagePath;
		}

		Path GetPackageLockPath(
			const PackageReference& originalReference,
			const PackageReference& activeReference,
			const Path& workingDirectory,
			const PackageLockState& packageLockState) const
		{
			Path packagePath;
			if (originalReference.IsLocal())
			{
				// Use local reference relative to package directory
				packagePath = originalReference.GetPath();
				if (!packagePath.HasRoot())
				{
					packagePath = workingDirectory + packagePath;
				}
			}
			else
			{
				if (activeReference.IsLocal())
				{
					// Use local reference relative to lock directory
					packagePath = activeReference.GetPath();
					if (!packagePath.HasRoot())
					{
						packagePath = packageLockState.RootDirectory + packagePath;
					}
				}
				else
				{
					// Build the global store location path
					auto packageStore = _userDataPath + Path("./locks/");
					auto& languageName = activeReference.GetLanguage();
					packagePath = packageStore +
						Path(std::format(
							"./{}/{}/{}/{}/",
							languageName,
							activeReference.GetOwner(),
							activeReference.GetName(),
							activeReference.GetVersion().ToString()));
				}
			}

			return packagePath;
		}

		PackageReference GetActivePackageReference(
			const PackageIdentifier& identifier,
			const std::string& closureName,
			const PackageLockState& packageLockState) const
		{
			if (!packageLockState.HasPackageLock)
			{
				throw std::runtime_error("Package locks are currently required.");
			}

			// Find the required closure
			auto findClosure = packageLockState.Closures.find(closureName);
			if (findClosure == packageLockState.Closures.end())
			{
				throw std::runtime_error(
					std::format(
						"Closure [{}] not found in lock [{}]",
						closureName,
						packageLockState.RootDirectory.ToString()));
			}

			// Find the package version in the lock
			auto findPackageLock = findClosure->second.find(identifier.GetLanguage());
			if (findPackageLock == findClosure->second.end())
			{
				throw std::runtime_error(
					std::format(
						"Language [{}] [{}] not found in lock [{}]",
						closureName,
						identifier.ToString(),
						packageLockState.RootDirectory.ToString()));
			}

			auto findPackageVersion = findPackageLock->second.find(identifier.GetPackageName());
			if (findPackageVersion == findPackageLock->second.end())
			{
				throw std::runtime_error(
					std::format(
						"Package [{}] [{}] not found in lock [{}]",
						closureName,
						identifier.ToString(),
						packageLockState.RootDirectory.ToString()));
			}

			auto& lockReference = findPackageVersion->second.Reference;
			if (lockReference.IsLocal())
				return lockReference;
			else
				return PackageReference(identifier.GetLanguage(), lockReference.GetOwner(), lockReference.GetName(), lockReference.GetVersion());
		}

		void LoadClosure(
			const std::optional<std::string>& buildToolClosureName,
			const PackageIdentifier& packageIdentifier,
			const Recipe& recipe,
			const Path& projectRoot,
			PackageId packageId,
			const std::set<PackageName>& parentSet,
			KnownPackageMap& knownPackageSet,
			const PackageLockState& packageLockState,
			const PackageLockState& parentPackageLockState,
			std::vector<PackageChildInfo>& toolDependencies)
		{
			// Add current package to the parent set when building child dependencies
			auto activeParentSet = parentSet;
			activeParentSet.insert(packageIdentifier.GetPackageName());

			// Keep track of the packages we have already seen
			auto insertKnown = knownPackageSet.emplace(
				packageIdentifier,
				std::make_pair(packageId, projectRoot));

			// Same language as parent is implied
			if (!recipe.HasLanguage())
				throw std::runtime_error("Recipe does not have a language reference.");

			// Get the current package build closure
			auto [buildClosureName, toolClosureName] = GetPackageSubGraphsClosure(
				packageIdentifier,
				packageLockState);

			auto dependencyProjects = std::map<std::string, std::vector<PackageChildInfo>>();
			auto buildDependencyToolDependencies = std::vector<PackageChildInfo>();
			for (auto dependencyType : recipe.GetDependencyTypes())
			{
				if (dependencyType == _dependencyTypeBuild)
				{
					auto [buildDependencies, buildToolDependencies] = LoadBuildDependencies(
						packageIdentifier,
						recipe,
						projectRoot,
						buildClosureName,
						toolClosureName,
						packageLockState);
					dependencyProjects.emplace(_dependencyTypeBuild, std::move(buildDependencies));
					buildDependencyToolDependencies = std::move(buildToolDependencies);
				}
				else if (dependencyType == _dependencyTypeTool)
				{
					if (buildToolClosureName.has_value())
					{
						toolDependencies = LoadToolDependencies(
							packageIdentifier,
							recipe,
							projectRoot,
							buildToolClosureName.value(),
							packageLockState,
							parentPackageLockState);
					}
					else
					{
						Log::Diag("Skipping tool dependencies outside direct dependencies of a Build dependency");
					}
				}
				else
				{
					auto dependencyTypeProjects = LoadRuntimeDependencies(
						packageIdentifier,
						recipe,
						projectRoot,
						dependencyType,
						activeParentSet,
						knownPackageSet,
						packageLockState,
						parentPackageLockState);
					dependencyProjects.emplace(dependencyType, std::move(dependencyTypeProjects));
				}
			}

			// Add the language as a build dependency
			auto [languageExtensionPackageChildInfo, languageExtensionToolDependencies] =
				LoadLanguageBuildDependency(
					packageIdentifier,
					recipe,
					projectRoot,
					buildClosureName,
					toolClosureName,
					packageLockState);
			buildDependencyToolDependencies.insert(
				buildDependencyToolDependencies.end(),
				std::make_move_iterator(languageExtensionToolDependencies.begin()),
				std::make_move_iterator(languageExtensionToolDependencies.end()));

			dependencyProjects[_dependencyTypeBuild].push_back(
				std::move(languageExtensionPackageChildInfo));

			// Tool dependencies for build dependencies are implicit dependencies for the project itself
			if (!buildDependencyToolDependencies.empty())
			{
				dependencyProjects.emplace(_dependencyTypeTool, std::move(buildDependencyToolDependencies));
			}

			// Save the package info
			_packageLookup.emplace(
				packageId,
				PackageInfo(
					packageId,
					packageIdentifier.GetPackageName(),
					false,
					projectRoot,
					Path(),
					&recipe,
					std::move(dependencyProjects)));
		}

		std::vector<PackageChildInfo> LoadRuntimeDependencies(
			const PackageIdentifier& packageIdentifier,
			const Recipe& recipe,
			const Path& projectRoot,
			const std::string& dependencyType,
			const std::set<PackageName>& activeParentSet,
			KnownPackageMap& knownPackageSet,
			const PackageLockState& packageLockState,
			const PackageLockState& parentPackageLockState)
		{
			auto dependencyTypeProjects = std::vector<PackageChildInfo>();
			for (auto dependency : recipe.GetNamedDependencies(dependencyType))
			{
				auto dependencyInfo = LoadRuntimeDependency(
					packageIdentifier,
					dependency,
					projectRoot,
					activeParentSet,
					knownPackageSet,
					packageLockState,
					parentPackageLockState);

				dependencyTypeProjects.push_back(
					std::move(dependencyInfo));
			}

			return dependencyTypeProjects;
		}

		PackageChildInfo LoadRuntimeDependency(
			const PackageIdentifier& parentIdentifier,
			const PackageReference& originalReference,
			const Path& projectRoot,
			const std::set<PackageName>& activeParentSet,
			KnownPackageMap& knownPackageSet,
			const PackageLockState& packageLockState,
			const PackageLockState& parentPackageLockState)
		{
			const Recipe* dependencyRecipe;
			Path dependencyProjectRoot;
			PackageIdentifier dependencyIdentifier;
			PackageReference activeReference;
			if (originalReference.IsLocal())
			{
				// Use local reference relative to package directory
				dependencyProjectRoot = originalReference.GetPath();
				if (!dependencyProjectRoot.HasRoot())
				{
					dependencyProjectRoot = projectRoot + dependencyProjectRoot;
				}

				auto packageRecipePath = dependencyProjectRoot + BuildConstants::RecipeFileName();
				if (!_recipeCache.TryGetOrLoadRecipe(packageRecipePath, dependencyRecipe))
				{
					Log::Error("The dependency Recipe does not exist: {}", packageRecipePath.ToString());
					Log::HighPriority("Make sure the path is correct and try again");

					// Nothing we can do, exit
					throw HandledException(1234672);
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					dependencyRecipe->GetLanguage().GetName(),
					std::nullopt,
					dependencyRecipe->GetName());

				// Use the original reference unchanged
				activeReference = originalReference;
			}
			else
			{
				// Resolve the actual package language
				std::string language;
				if (originalReference.HasLanguage())
				{
					language = originalReference.GetLanguage();
				}
				else
				{
					// Use the parent recipe language as the implicit language
					language = parentIdentifier.GetLanguage();
				}

				// Resolve the owner
				std::optional<std::string> owner = std::nullopt;
				if (originalReference.HasOwner())
				{
					owner = originalReference.GetOwner();
				}
				else
				{
					if (parentIdentifier.HasOwner())
					{
						// Use the parent recipe owner as the implicit owner
						owner = parentIdentifier.GetOwner();
					}
					else
					{
						Log::Error("Implicit owner not allowed on local packages: {}", parentIdentifier.ToString());

						// Nothing we can do, exit
						throw HandledException(9571836);
					}
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					language,
					owner,
					originalReference.GetName());

				// Resolve the actual package that will be used
				activeReference = GetActivePackageReference(
					dependencyIdentifier,
					_rootClosureName,
					packageLockState);

				// Load this package recipe
				dependencyProjectRoot = GetPackageReferencePath(
					activeReference,
					packageLockState);
				auto packageRecipePath = dependencyProjectRoot + BuildConstants::RecipeFileName();
				if (!_recipeCache.TryGetOrLoadRecipe(packageRecipePath, dependencyRecipe))
				{
					Log::Error("The dependency Recipe version has not been installed: {} -> {} [{}]", activeReference.ToString(), dependencyProjectRoot.ToString(), projectRoot.ToString());
					Log::HighPriority("Run `restore` and try again");

					// Nothing we can do, exit
					throw HandledException(123432);
				}
			}

			// Ensure we do not have any circular dependencies
			if (activeParentSet.contains(dependencyIdentifier.GetPackageName()))
			{
				Log::Error("Found circular dependency: {} -> {}", parentIdentifier.ToString(), dependencyIdentifier.ToString());
				throw std::runtime_error("BuildRecipeAndDependencies: Circular dependency.");
			}

			// Check if the package has already been processed from another reference
			auto findKnownPackage = knownPackageSet.find(dependencyIdentifier);
			if (findKnownPackage != knownPackageSet.end())
			{
				// Verify the project name is unique
				if (findKnownPackage->second.second != dependencyProjectRoot)
				{
					Log::Error("Recipe with this name already exists: {} [{}] [{}]", dependencyIdentifier.ToString(), projectRoot.ToString(), findKnownPackage->second.second.ToString());
					throw std::runtime_error("Recipe name not unique");
				}
				else
				{
					Log::Diag("Recipe already loaded: {}", dependencyIdentifier.ToString());
					return PackageChildInfo(activeReference, false, findKnownPackage->second.first, -1);
				}
			}
			else
			{
				// Discover all recursive dependencies
				auto childPackageId = ++_uniquePackageId;
				auto toolDependencyProjects = std::vector<PackageChildInfo>();
				std::optional<std::string> runtimeBuildToolClosureName = std::nullopt;
				LoadClosure(
					runtimeBuildToolClosureName,
					dependencyIdentifier,
					*dependencyRecipe,
					dependencyProjectRoot,
					childPackageId,
					activeParentSet,
					knownPackageSet,
					packageLockState,
					parentPackageLockState,
					toolDependencyProjects);

				for (auto& toolDependency : toolDependencyProjects)
					Log::Warning("Runtime Dependency Tool Dependency discarded: {}", toolDependency.OriginalReference.ToString());

				// Update the child project id
				return PackageChildInfo(activeReference, false, childPackageId, -1);
			}
		}

		std::pair<std::vector<PackageChildInfo>, std::vector<PackageChildInfo>> LoadBuildDependencies(
			const PackageIdentifier& packageIdentifier,
			const Recipe& recipe,
			const Path& projectRoot,
			const std::string& buildClosureName,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState)
		{
			auto buildProjects = std::vector<PackageChildInfo>();
			auto buildToolProjects = std::vector<PackageChildInfo>();
			for (auto dependency : recipe.GetNamedDependencies(_dependencyTypeBuild))
			{
				auto [buildDependency, buildToolDependencies] = LoadBuildDependency(
					packageIdentifier,
					dependency,
					projectRoot,
					buildClosureName,
					toolClosureName,
					packageLockState);
				buildProjects.push_back(std::move(buildDependency));

				// Propagate the build tool dependencies
				buildToolProjects.insert(
					buildToolProjects.end(),
					std::make_move_iterator(buildToolDependencies.begin()),
					std::make_move_iterator(buildToolDependencies.end()));
			}

			return std::make_pair(std::move(buildProjects), std::move(buildToolProjects));
		}

		std::vector<PackageChildInfo> LoadToolDependencies(
			const PackageIdentifier& packageIdentifier,
			const Recipe& recipe,
			const Path& projectRoot,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState,
			const PackageLockState& parentPackageLockState)
		{
			auto dependencyTypeProjects = std::vector<PackageChildInfo>();
			for (auto dependency : recipe.GetNamedDependencies(_dependencyTypeTool))
			{
				dependencyTypeProjects.push_back(
					LoadToolDependency(
						packageIdentifier,
						dependency,
						projectRoot,
						toolClosureName,
						packageLockState,
						parentPackageLockState));
			}

			return dependencyTypeProjects;
		}

		std::pair<PackageChildInfo, std::vector<PackageChildInfo>> LoadBuildDependency(
			const PackageIdentifier& parentIdentifier,
			const PackageReference& originalReference,
			const Path& projectRoot,
			const std::string& buildClosureName,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState)
		{
			PackageIdentifier dependencyIdentifier;
			PackageReference activeReference;
			if (originalReference.IsLocal())
			{
				// Use local reference relative to package directory
				auto dependencyProjectRoot = originalReference.GetPath();
				if (!dependencyProjectRoot.HasRoot())
				{
					dependencyProjectRoot = projectRoot + dependencyProjectRoot;
				}

				auto packageRecipePath = dependencyProjectRoot + BuildConstants::RecipeFileName();
				const Recipe* dependencyRecipe;
				if (!_recipeCache.TryGetOrLoadRecipe(packageRecipePath, dependencyRecipe))
				{
					Log::Error("The dependency Recipe does not exist: {}", packageRecipePath.ToString());
					Log::HighPriority("Make sure the path is correct and try again");

					// Nothing we can do, exit
					throw HandledException(1234672);
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					dependencyRecipe->GetLanguage().GetName(),
					std::nullopt,
					dependencyRecipe->GetName());

				// Use the original reference unchanged
				activeReference = originalReference;
			}
			else
			{
				// Resolve the actual package language
				std::string language;
				if (originalReference.HasLanguage())
				{
					language = originalReference.GetLanguage();
				}
				else
				{
					// Build dependencies do not inherit the parent language
					// Instead, they default to Wren
					language = _builtInWrenLanguage;
				}

				// Resolve the owner
				std::optional<std::string> owner = std::nullopt;
				if (originalReference.HasOwner())
				{
					owner = originalReference.GetOwner();
				}
				else
				{
					if (parentIdentifier.HasOwner())
					{
						// Use the parent recipe owner as the implicit owner
						owner = parentIdentifier.GetOwner();
					}
					else
					{
						Log::Error("Implicit owner not allowed on local packages: {}", parentIdentifier.ToString());

						// Nothing we can do, exit
						throw HandledException(9571836);
					}
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					language,
					owner,
					originalReference.GetName());

				// Resolve the actual package that will be used
				activeReference = GetActivePackageReference(
					dependencyIdentifier,
					buildClosureName,
					packageLockState);
			}

			// Check for a built in version of the package
			if (HasBuiltInVersion(activeReference))
			{
				return LoadSubGraphBuiltInPackage(
					dependencyIdentifier,
					activeReference,
					toolClosureName,
					packageLockState);
			}
			else
			{
				return LoadSubGraphDependency(
					dependencyIdentifier,
					originalReference,
					activeReference,
					projectRoot,
					toolClosureName,
					packageLockState);
			}
		}

		bool HasBuiltInVersion(
			const PackageReference& reference)
		{
			if (reference.IsLocal())
				return false;

			auto builtInLanguagePackageResult = _builtInPackageLookup.find(reference.GetLanguage());
			if (builtInLanguagePackageResult == _builtInPackageLookup.end())
				return false;

			auto packageName = PackageName(reference.GetOwner(), reference.GetName());
			auto builtInPackageResult = builtInLanguagePackageResult->second.find(packageName);
			if (builtInPackageResult == builtInLanguagePackageResult->second.end())
				return false;

			return builtInPackageResult->second == reference.GetVersion();
		}

		PackageChildInfo LoadToolDependency(
			const PackageIdentifier& parentIdentifier,
			const PackageReference& originalReference,
			const Path& projectRoot,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState,
			const PackageLockState& parentPackageLockState)
		{
			PackageIdentifier dependencyIdentifier;
			PackageReference activeReference;
			if (originalReference.IsLocal())
			{
				// Use local reference relative to package directory
				auto dependencyProjectRoot = originalReference.GetPath();
				if (!dependencyProjectRoot.HasRoot())
				{
					dependencyProjectRoot = projectRoot + dependencyProjectRoot;
				}

				auto packageRecipePath = dependencyProjectRoot + BuildConstants::RecipeFileName();
				const Recipe* dependencyRecipe;
				if (!_recipeCache.TryGetOrLoadRecipe(packageRecipePath, dependencyRecipe))
				{
					Log::Error("The dependency Recipe does not exist: {}", packageRecipePath.ToString());
					Log::HighPriority("Make sure the path is correct and try again");

					// Nothing we can do, exit
					throw HandledException(1234672);
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					dependencyRecipe->GetLanguage().GetName(),
					std::nullopt,
					dependencyRecipe->GetName());

				// Use the original reference unchanged
				activeReference = originalReference;
			}
			else
			{
				// Resolve the actual package language
				std::string language;
				if (originalReference.HasLanguage())
				{
					language = originalReference.GetLanguage();
				}
				else
				{
					// Tool dependencies do not inherit the parent language
					// They must be explicitly defined
					throw std::runtime_error(
						std::format(
							"Tool dependency must have explicit language defined: {}",
							originalReference.ToString()));
				}

				// Resolve the owner
				std::optional<std::string> owner = std::nullopt;
				if (originalReference.HasOwner())
				{
					owner = originalReference.GetOwner();
				}
				else
				{
					if (parentIdentifier.HasOwner())
					{
						// Use the parent recipe owner as the implicit owner
						owner = parentIdentifier.GetOwner();
					}
					else
					{
						Log::Error("Implicit owner not allowed on local packages: {}", parentIdentifier.ToString());

						// Nothing we can do, exit
						throw HandledException(9571836);
					}
				}

				// Build up the unique identifier
				dependencyIdentifier = PackageIdentifier(
					language,
					owner,
					originalReference.GetName());

				// Retrieve the tool version from the build dependency parent lock
				activeReference = GetActivePackageReference(
					dependencyIdentifier,
					toolClosureName,
					parentPackageLockState);
			}

			PackageChildInfo toolDependency;
			std::vector<PackageChildInfo> toolToolDependencies;

			// Check for a built in version of the package
			auto toolToolClosureName = std::string();
			if (HasBuiltInVersion(activeReference))
			{
				std::tie(toolDependency, toolToolDependencies) = LoadSubGraphBuiltInPackage(
					dependencyIdentifier,
					activeReference,
					toolToolClosureName,
					packageLockState);
			}
			else
			{
				std::tie(toolDependency, toolToolDependencies) = LoadSubGraphDependency(
					dependencyIdentifier,
					originalReference,
					activeReference,
					projectRoot,
					toolToolClosureName,
					packageLockState);
			}

			for (auto& toolToolDependency : toolToolDependencies)
				Log::Warning("Tool Tool Dependency discarded: {}", toolToolDependency.OriginalReference.ToString());

			return toolDependency;
		}

		std::pair<PackageChildInfo, std::vector<PackageChildInfo>> LoadSubGraphDependency(
			const PackageIdentifier& parentIdentifier,
			const PackageReference& originalReference,
			const PackageReference& activeReference,
			const Path& projectRoot,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState)
		{
			// Load this package recipe
			Path dependencyProjectRoot;
			if (originalReference.IsLocal())
			{
				// Use local reference relative to package directory
				dependencyProjectRoot = originalReference.GetPath();
				if (!dependencyProjectRoot.HasRoot())
				{
					dependencyProjectRoot = projectRoot + dependencyProjectRoot;
				}
			}
			else
			{
				dependencyProjectRoot = GetPackageReferencePath(
					activeReference,
					packageLockState);
			}

			// Check if the package has already been processed from another graph
			auto findKnownGraph = _knownSubGraphSet.find(dependencyProjectRoot);
			if (findKnownGraph != _knownSubGraphSet.end())
			{
				// Verify the project name is unique
				Log::Diag("Graph already loaded: {}", dependencyProjectRoot.ToString());
				return std::make_pair(
					PackageChildInfo(activeReference, true, -1, findKnownGraph->second.first),
					findKnownGraph->second.second);
			}
			else
			{
				auto packageRecipePath = dependencyProjectRoot + BuildConstants::RecipeFileName();
				const Recipe* dependencyRecipe;
				if (!_recipeCache.TryGetOrLoadRecipe(packageRecipePath, dependencyRecipe))
				{
					if (activeReference.IsLocal())
					{
						Log::Error("The dependency Recipe does not exist: {}", packageRecipePath.ToString());
						Log::HighPriority("Make sure the path is correct and try again");
					}
					else
					{
						Log::Error("The dependency Recipe version has not been installed: {} -> {} [{}]", activeReference.ToString(), dependencyProjectRoot.ToString(), projectRoot.ToString());
						Log::HighPriority("Run `restore` and try again");
					}

					// Nothing we can do, exit
					throw HandledException(1234);
				}

				// Reset parent set to allow uniqueness within sub graph
				auto parentSet = std::set<PackageName>();
				auto knownPackageSet = KnownPackageMap();

				// Load the package lock if present for the build dependency
				auto packageLockRoot = GetPackageLockPath(
					originalReference,
					activeReference,
					projectRoot,
					packageLockState);
				const auto& dependencyPackageLockState = LoadPackageLock(packageLockRoot);

				// Resolve the owner
				std::optional<std::string> owner = std::nullopt;
				if (!originalReference.IsLocal())
				{
					if (originalReference.HasOwner())
					{
						owner = originalReference.GetOwner();
					}
					else
					{
						if (parentIdentifier.HasOwner())
						{
							// Use the parent recipe owner as the implicit owner
							owner = parentIdentifier.GetOwner();
						}
						else
						{
							Log::Error("Implicit owner not allowed on local packages: {}", parentIdentifier.ToString());

							// Nothing we can do, exit
							throw HandledException(9571836);
						}
					}
				}

				// Build up the unique identifier
				auto dependencyIdentifier = PackageIdentifier(
					dependencyRecipe->GetLanguage().GetName(),
					owner,
					dependencyRecipe->GetName());

				// Discover all recursive dependencies
				auto childPackageId = ++_uniquePackageId;
				auto toolDependencyProjects = std::vector<PackageChildInfo>();
				LoadClosure(
					toolClosureName,
					dependencyIdentifier,
					*dependencyRecipe,
					dependencyProjectRoot,
					childPackageId,
					parentSet,
					knownPackageSet,
					dependencyPackageLockState,
					packageLockState,
					toolDependencyProjects);

				// Create the build graph
				auto graphId = ++_uniqueGraphId;

				// Save the package graph
				_packageGraphLookup.emplace(
					graphId,
					PackageGraph(graphId, childPackageId, _hostBuildGlobalParameters));

				// Keep track of the build graphs we have already seen
				auto insertKnown = _knownSubGraphSet.emplace(
					dependencyProjectRoot,
					std::make_pair(graphId, toolDependencyProjects));

				// Update the child project id
				return std::make_pair(
					PackageChildInfo(activeReference, true, -1, graphId),
					std::move(toolDependencyProjects));
			}
		}

		std::pair<PackageChildInfo, std::vector<PackageChildInfo>> LoadLanguageBuildDependency(
			const PackageIdentifier& packageIdentifier,
			const Recipe& recipe,
			const Path& projectRoot,
			const std::string& buildClosureName,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState)
		{
			auto language = recipe.GetLanguage();

			// Get the active version
			auto knownLanguageResult = _knownLanguageLookup.find(language.GetName());
			if (knownLanguageResult == _knownLanguageLookup.end())
			{
				throw std::runtime_error(
					std::format("Unknown language: {}", language.GetName()));
			}

			auto& knownLanguage = knownLanguageResult->second;

			auto builtInExtensionReference = PackageReference(
				std::nullopt,
				knownLanguage.ExtensionOwner,
				knownLanguage.ExtensionName,
				language.GetVersion());

			return LoadBuildDependency(
				packageIdentifier,
				builtInExtensionReference,
				projectRoot,
				buildClosureName,
				toolClosureName,
				packageLockState);
		}

		std::pair<PackageChildInfo, std::vector<PackageChildInfo>> LoadSubGraphBuiltInPackage(
			const PackageIdentifier& packageIdentifier,
			const PackageReference& activeReference,
			const std::string& toolClosureName,
			const PackageLockState& packageLockState)
		{
			// Use the prebuilt version in the install folder
			auto packageRoot = _builtInPackageDirectory +
				Path(std::format(
					"./{}/{}/{}/",
					activeReference.GetOwner(),
					activeReference.GetName(),
					activeReference.GetVersion().ToString()));

			// Check if the package has already been processed from another graph
			auto findKnownGraph = _knownSubGraphSet.find(packageRoot);
			if (findKnownGraph != _knownSubGraphSet.end())
			{
				// Verify the project name is unique
				Log::Diag("Graph already loaded: {}", packageRoot.ToString());
				return std::make_pair(
					PackageChildInfo(activeReference, true, -1, findKnownGraph->second.first),
					findKnownGraph->second.second);
			}
			else
			{
				auto recipePath = packageRoot + BuildConstants::RecipeFileName();
				const Recipe* recipe;
				if (!_recipeCache.TryGetOrLoadRecipe(recipePath, recipe))
				{
					Log::Error("The built in package Recipe does not exist: {}", recipePath.ToString());
					Log::HighPriority("The installation may be corrupted");

					// Nothing we can do, exit
					throw HandledException(1123124);
				}

				// Built in packages do not load the lock
				auto dependencyPackageLockState = PackageLockState();

				auto packageToolDependencies = std::vector<PackageChildInfo>();
				if (recipe->HasNamedDependencies(_dependencyTypeTool))
				{
					packageToolDependencies = LoadToolDependencies(
						packageIdentifier,
						*recipe,
						packageRoot,
						toolClosureName,
						dependencyPackageLockState,
						packageLockState);
				}

				// Create a fake child package id
				auto packageId = ++_uniquePackageId;

				// Create the build graph
				auto graphId = ++_uniqueGraphId;

				// Save the package graph
				_packageGraphLookup.emplace(
					graphId,
					PackageGraph(graphId, packageId, {}));

				// Keep track of the build graphs we have already seen
				auto insertKnown = _knownSubGraphSet.emplace(
					packageRoot,
					std::make_pair(graphId, packageToolDependencies));

				// The target directory is under the root
				auto targetDirectory = packageRoot + _builtInPackageOutPath;

				// Save the package info
				auto packageName = PackageName(activeReference.GetOwner(), activeReference.GetName());
				_packageLookup.emplace(
					packageId,
					PackageInfo(
						packageId,
						std::move(packageName),
						true,
						std::move(packageRoot),
						std::move(targetDirectory),
						nullptr,
						{}));

				// Update the child project id
				return std::make_pair(
					PackageChildInfo(activeReference, true, -1, graphId),
					std::move(packageToolDependencies));
			}
		}
	};
}
