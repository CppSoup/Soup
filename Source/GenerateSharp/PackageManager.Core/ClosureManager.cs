﻿// <copyright file="ClosureManager.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright

using Opal;
using Opal.System;
using Soup.Build.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;

namespace Soup.Build.PackageManager;

/// <summary>
/// The closure builder
/// </summary>
public class ClosureManager : IClosureManager
{
	private const int PackageLockVersion = 5;
	private const string RootClosureName = "Root";
	private const string BuiltInLanguageCSharp = "C#";
	private const string BuiltInLanguageCpp = "C++";
	private const string BuiltInLanguageWren = "Wren";
	private const string BuiltInLanguagePackageCSharp = "Soup.CSharp";
	private const string BuiltInLanguagePackageCpp = "Soup.Cpp";
	private const string BuiltInLanguagePackageWren = "Soup.Wren";
	private const string BuiltInLanguageSafeNameCSharp = "CSharp";
	private const string BuiltInLanguageSafeNameCpp = "Cpp";
	private const string BuiltInLanguageSafeNameWren = "Wren";
	private const string BuiltInOwner = "mwasplund";
	private const string DependencyTypeBuild = "Build";
	private const string DependencyTypeTool = "Tool";

	private readonly Uri _apiEndpoint;

	private readonly HttpClient _httpClient;

	private readonly SemanticVersion _builtInLanguageVersionCSharp;
	private readonly SemanticVersion _builtInLanguageVersionCpp;
	private readonly SemanticVersion _builtInLanguageVersionWren;

	public ClosureManager(
		Uri apiEndpoint,
		HttpClient httpClient,
		SemanticVersion builtInLanguageVersionCSharp,
		SemanticVersion builtInLanguageVersionCpp,
		SemanticVersion builtInLanguageVersionWren)
	{
		_apiEndpoint = apiEndpoint;
		_httpClient = httpClient;
		_builtInLanguageVersionCSharp = builtInLanguageVersionCSharp;
		_builtInLanguageVersionCpp = builtInLanguageVersionCpp;
		_builtInLanguageVersionWren = builtInLanguageVersionWren;
	}

	/// <summary>
	/// Restore packages
	/// </summary>
	public async Task GenerateAndRestoreRecursiveLocksAsync(
		Path workingDirectory,
		Path packageStoreDirectory,
		Path packageLockStoreDirectory,
		Path stagingDirectory)
	{
		// Place the lock in the local directory
		var packageLockPath =
			workingDirectory +
			BuildConstants.PackageLockFileName;

		var processedLocks = new HashSet<Path>();
		await CheckGenerateAndRestoreRecursiveLocksAsync(
			workingDirectory,
			packageLockPath,
			packageStoreDirectory,
			packageLockStoreDirectory,
			stagingDirectory,
			processedLocks);
	}

	private async Task CheckGenerateAndRestoreRecursiveLocksAsync(
		Path workingDirectory,
		Path packageLockPath,
		Path packageStoreDirectory,
		Path packageLockStoreDirectory,
		Path stagingDirectory,
		HashSet<Path> processedLocks)
	{
		if (!processedLocks.Add(packageLockPath))
		{
			Log.Info("Root already processed");
		}
		else
		{
			var packageLock = await EnsurePackageLockAsync(
				workingDirectory,
				packageLockPath);
			await RestorePackageLockAsync(
				packageStoreDirectory,
				stagingDirectory,
				packageLock);

			await CheckGenerateAndRestoreSubGraphDependencyLocksAsync(
				workingDirectory,
				packageStoreDirectory,
				packageLockStoreDirectory,
				stagingDirectory,
				packageLock,
				processedLocks);
		}
	}

	private async Task CheckGenerateAndRestoreSubGraphDependencyLocksAsync(
		Path workingDirectory,
		Path packageStoreDirectory,
		Path packageLockStoreDirectory,
		Path stagingDirectory,
		PackageLock packageLock,
		HashSet<Path> processedLocks)
	{
		foreach (var closure in packageLock.GetClosures().Values)
		{
			// Skip the root closure and only generate locks for the build extensions
			if (closure.Key != RootClosureName)
			{
				foreach (var (languageName, languageProjects) in closure.Value.Value.AsTable().Values)
				{
					foreach (var (projectUniqueName, project) in languageProjects.Value.AsTable().Values)
					{
						var projectTable = project.Value.AsTable();
						var projectName = PackageName.Parse(projectUniqueName);
						var projectVersion = projectTable.Values[PackageLock.Property_Version].Value.AsString().Value;
						if (SemanticVersion.TryParse(projectVersion, out var version))
						{
							// Check if the package version already exists
							if (projectName.HasOwner && projectName.Owner == BuiltInOwner &&
								((projectName.Name == BuiltInLanguagePackageCpp && version == _builtInLanguageVersionCpp) ||
								(projectName.Name == BuiltInLanguagePackageCSharp && version == _builtInLanguageVersionCSharp) ||
								(projectName.Name == BuiltInLanguagePackageWren && version == _builtInLanguageVersionWren)))
							{
								Log.HighPriority("Skip built in language version in build closure");
							}
							else
							{
								var languageSafeName = GetLanguageSafeName(languageName);
								var userFolder = projectName.Owner is not null ? new Path(projectName.Owner) : new Path("Local");
								var packageLanguageNameVersionPath =
									new Path(languageSafeName) +
									userFolder +
									new Path(projectName.Name) +
									new Path(version.ToString()) +
									new Path();
								var packageContentDirectory = packageStoreDirectory + packageLanguageNameVersionPath;

								// Place the lock in the lock store
								var packageLockDirectory =
									packageLockStoreDirectory +
									packageLanguageNameVersionPath;
								var packageLockPath =
									packageLockDirectory +
									BuildConstants.PackageLockFileName;

								EnsureDirectoryExists(packageLockDirectory);

								await CheckGenerateAndRestoreRecursiveLocksAsync(
									packageContentDirectory,
									packageLockPath,
									packageStoreDirectory,
									packageLockStoreDirectory,
									stagingDirectory,
									processedLocks);
							}
						}
						else
						{
							// Process the local dependency and place the lock in the root
							var referencePath = new Path(projectVersion);
							var dependencyPath = workingDirectory + referencePath;
							var dependencyLockPath =
								dependencyPath +
								BuildConstants.PackageLockFileName;

							await CheckGenerateAndRestoreRecursiveLocksAsync(
								dependencyPath,
								dependencyLockPath,
								packageStoreDirectory,
								packageLockStoreDirectory,
								stagingDirectory,
								processedLocks);
						}
					}
				}
			}
		}
	}

	/// <summary>
	/// Restore packages
	/// </summary>
	private async Task<PackageLock> EnsurePackageLockAsync(
		Path workingDirectory,
		Path packageLockPath)
	{
		Log.Info($"Ensure Package Lock Exists: {packageLockPath}");
		var loadPackageLock = await PackageLockExtensions.TryLoadFromFileAsync(packageLockPath);
		if (loadPackageLock.IsSuccess)
		{
			Log.Info("Restore from package lock");
			return loadPackageLock.Result;
		}
		else
		{
			Log.Info("Discovering full closure");
			var localPackageReverseLookup = new Dictionary<int, (string Language, string Name, Path Path)>();
			var localPackageLookup = new Dictionary<Path, Api.Client.PackageLocalReferenceModel>();
			var publicPackages = new List<Api.Client.PackagePublicReferenceModel>();
			var rootPackageId = await EnsureDiscoverDependenciesAsync(
				workingDirectory,
				localPackageReverseLookup,
				localPackageLookup,
				publicPackages);

			// Attempt to resolve all dependencies to compatible and up-to-date versions
			Log.Info("Generate final service closure");
			var (runtimeClosure, buildClosures, toolClosures) = await GenerateServiceClosureAsync(
				rootPackageId,
				localPackageReverseLookup,
				localPackageLookup,
				publicPackages);

			// Build up the package lock file
			var packageLock = BuildPackageLock(
				workingDirectory,
				runtimeClosure,
				buildClosures,
				toolClosures);

			// Save the updated package lock
			await PackageLockExtensions.SaveToFileAsync(packageLockPath, packageLock);

			return packageLock;
		}
	}

	private async Task<(
		IDictionary<string, IDictionary<PackageName, (PackageReference Package, string BuildClosure, string ToolClosure)>> RuntimeClosure,
		IDictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>> BuildClosures,
		IDictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>> ToolClosures)> GenerateServiceClosureAsync(
		int rootPackageId,
		IDictionary<int, (string Language, string Name, Path Path)> localPackageReverseLookup,
		IDictionary<Path, Api.Client.PackageLocalReferenceModel> localPackageLookup,
		IList<Api.Client.PackagePublicReferenceModel> publicPackages)
	{
		// Publish the archive
		var packageClient = new Api.Client.ClosureClient(_httpClient, null)
		{
			BaseUrl = _apiEndpoint,
		};

		// Pull out the root package
		var rootPackage = localPackageLookup.Values.First(value => value.Id == rootPackageId);
		var localPackages = localPackageLookup.Values.Where(value => value.Id != rootPackageId).ToList();

		// Request the built in versions for the language extensions
		var preferredVersions = new List<Api.Client.PackagePublicExactReferenceModel>
		{
			new Api.Client.PackagePublicExactReferenceModel()
			{
				Language = BuiltInLanguageWren,
				Owner = BuiltInOwner,
				Name = BuiltInLanguagePackageCSharp,
				Version = new Api.Client.SemanticVersionExactModel()
				{
					Major = _builtInLanguageVersionCSharp.Major,
					Minor = _builtInLanguageVersionCSharp.Minor ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
					Patch = _builtInLanguageVersionCSharp.Patch ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
				},
			},
			new Api.Client.PackagePublicExactReferenceModel()
			{
				Language = BuiltInLanguageWren,
				Owner = BuiltInOwner,
				Name = BuiltInLanguagePackageCpp,
				Version = new Api.Client.SemanticVersionExactModel()
				{
					Major = _builtInLanguageVersionCpp.Major,
					Minor = _builtInLanguageVersionCpp.Minor ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
					Patch = _builtInLanguageVersionCpp.Patch ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
				},
			},
			new Api.Client.PackagePublicExactReferenceModel()
			{
				Language = BuiltInLanguageWren,
				Owner = BuiltInOwner,
				Name = BuiltInLanguagePackageWren,
				Version = new Api.Client.SemanticVersionExactModel()
				{
					Major = _builtInLanguageVersionWren.Major,
					Minor = _builtInLanguageVersionWren.Minor ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
					Patch = _builtInLanguageVersionWren.Patch ?? throw new InvalidOperationException("Built In Language must be fully resolved"),
				},
			}
		};

		var generateClosureRequest = new Api.Client.GenerateClosureRequestModel()
		{
			RootPackage = rootPackage,
			LocalPackages = localPackages,
			PublicPackages = publicPackages,
			PreferredVersions = preferredVersions,
		};

		Api.Client.GenerateClosureResultModel result;
		try
		{
			result = await packageClient.GenerateClosureAsync(generateClosureRequest);
		}
		catch (HttpRequestException)
		{
			Log.Error("Failed to connect, check internet connection and try again");
			throw new HandledException();
		}
		catch (Api.Client.ApiException)
		{
			Log.Error("Service request failed");
			throw;
		}

		if (result.Result != Api.Client.GenerateClosureResult.Success)
		{
			Log.Error($"Unable to create closure: {result.Message}");
			throw new HandledException();
		}

		// Convert back to resolved closures
		var runtimeClosure = new Dictionary<string, IDictionary<PackageName, (PackageReference Package, string BuildClosure, string ToolClosure)>>();
		foreach (var package in result.RuntimeClosure)
		{
			string language;
			PackageName uniqueName;
			PackageReference packageReference;
			if (package.Public is not null)
			{
				language = package.Public.Language;
				var owner = package.Public.Owner;
				var name = package.Public.Name;

				// Create unique name from owner/name
				uniqueName = new PackageName(owner, name);

				var version = new SemanticVersion(
					package.Public.Version.Major,
					package.Public.Version.Minor,
					package.Public.Version.Patch);
				packageReference = new PackageReference(owner, language, name, version);
			}
			else if (package.LocalId is not null)
			{
				var localReference = localPackageReverseLookup[package.LocalId.Value];
				language = localReference.Language;
				uniqueName = new PackageName(null, localReference.Name);
				packageReference = new PackageReference(localReference.Path);
			}
			else
			{
				throw new InvalidOperationException("Package had neither public or local reference");
			}

			if (!runtimeClosure.ContainsKey(language))
			{
				runtimeClosure.Add(
					language,
					new Dictionary<PackageName, (PackageReference Package, string BuildClosure, string ToolClosure)>());
			}

			if (runtimeClosure[language].ContainsKey(uniqueName))
				Log.Warning($"Duplicate reference seen in generate closure response {uniqueName}");
			else
				runtimeClosure[language].Add(uniqueName, (packageReference, package.Build, package.Tool));
		}

		var buildClosures = new Dictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>>();
		foreach (var (closureName, closure) in result.BuildClosures)
		{
			var buildClosure = new Dictionary<string, IDictionary<PackageName, PackageReference>>();
			foreach (var package in closure)
			{
				string language;
				PackageName uniqueName;
				PackageReference packageReference;
				if (package.Public is not null)
				{
					language = package.Public.Language;
					var owner = package.Public.Owner;
					var name = package.Public.Name;

					// Create unique name from owner/name
					uniqueName = new PackageName(owner, name);

					var version = new SemanticVersion(
						package.Public.Version.Major,
						package.Public.Version.Minor,
						package.Public.Version.Patch);
					packageReference = new PackageReference(language, owner, name, version);
				}
				else if (package.LocalId is not null)
				{
					var localReference = localPackageReverseLookup[package.LocalId.Value];
					language = localReference.Language;
					uniqueName = new PackageName(null, localReference.Name);
					packageReference = new PackageReference(localReference.Path);
				}
				else
				{
					throw new InvalidOperationException("Package had neither public or local reference");
				}

				if (!buildClosure.ContainsKey(language))
					buildClosure.Add(language, new Dictionary<PackageName, PackageReference>());

				buildClosure[language].Add(uniqueName, packageReference);
			}

			buildClosures.Add(closureName, buildClosure);
		}

		var toolClosures = new Dictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>>();
		foreach (var (closureName, closure) in result.ToolClosures)
		{
			var toolClosure = new Dictionary<string, IDictionary<PackageName, PackageReference>>();
			foreach (var package in closure)
			{
				string language;
				PackageName uniqueName;
				PackageReference packageReference;
				if (package.Public is not null)
				{
					language = package.Public.Language;
					var owner = package.Public.Owner;
					var name = package.Public.Name;

					// Create unique name from owner/name
					uniqueName = new PackageName(owner, name);

					var version = new SemanticVersion(
						package.Public.Version.Major,
						package.Public.Version.Minor,
						package.Public.Version.Patch);
					packageReference = new PackageReference(language, owner, name, version);
				}
				else if (package.LocalId is not null)
				{
					var localReference = localPackageReverseLookup[package.LocalId.Value];
					language = localReference.Language;
					uniqueName = new PackageName(null, localReference.Name);
					packageReference = new PackageReference(localReference.Path);
				}
				else
				{
					throw new InvalidOperationException("Package had neither public or local reference");
				}

				if (!toolClosure.ContainsKey(language))
					toolClosure.Add(language, new Dictionary<PackageName, PackageReference>());

				toolClosure[language].Add(uniqueName, packageReference);
			}

			toolClosures.Add(closureName, toolClosure);
		}

		return (runtimeClosure, buildClosures, toolClosures);
	}

	private static PackageLock BuildPackageLock(
		Path workingDirectory,
		IDictionary<string, IDictionary<PackageName, (PackageReference Package, string BuildClosure, string ToolClosure)>> runtimeClosure,
		IDictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>> buildClosures,
		IDictionary<string, IDictionary<string, IDictionary<PackageName, PackageReference>>> toolClosures)
	{
		var packageLock = new PackageLock();
		packageLock.SetVersion(PackageLockVersion);
		foreach (var (languageName, languageClosure) in runtimeClosure.OrderBy(value => value.Key))
		{
			foreach (var (packageName, (package, buildClosure, toolClosure)) in languageClosure.OrderBy(value => value.Key))
			{
				var value = string.Empty;
				var ownerName = string.Empty;
				if (package.IsLocal)
				{
					value = package.Path.GetRelativeTo(workingDirectory).ToString();
				}
				else
				{
					if (package.Version == null)
						throw new InvalidOperationException("Package lock closure must have version");
					value = package.Version.ToString();
					ownerName = package.Owner;
				}

				Log.Diag($"{RootClosureName}:{languageName} {packageName} -> {value}");
				packageLock.AddProject(
					RootClosureName,
					languageName,
					packageName,
					value,
					buildClosure,
					toolClosure);
			}
		}

		foreach (var buildClosure in buildClosures.OrderBy(value => value.Key))
		{
			packageLock.EnsureClosure(buildClosure.Key);
			foreach (var (languageName, languageClosure) in buildClosure.Value.OrderBy(value => value.Key))
			{
				foreach (var (packageName, package) in languageClosure)
				{
					var value = string.Empty;
					var ownerName = string.Empty;
					if (package.IsLocal)
					{
						value = package.Path.GetRelativeTo(workingDirectory).ToString();
					}
					else
					{
						if (package.Version == null)
							throw new InvalidOperationException("Package lock closure must have version");
						value = package.Version.ToString();
						ownerName = package.Owner;
					}

					Log.Diag($"{buildClosure.Key}:{languageName} {packageName} -> {value}");
					packageLock.AddProject(
						buildClosure.Key,
						languageName,
						packageName,
						value,
						null,
						null);
				}
			}
		}

		foreach (var toolClosure in toolClosures.OrderBy(value => value.Key))
		{
			packageLock.EnsureClosure(toolClosure.Key);
			foreach (var (languageName, languageClosure) in toolClosure.Value.OrderBy(value => value.Key))
			{
				foreach (var (packageName, package) in languageClosure)
				{
					var value = string.Empty;
					if (package.IsLocal)
					{
						value = package.Path.GetRelativeTo(workingDirectory).ToString();
					}
					else
					{
						if (package.Version == null)
							throw new InvalidOperationException("Package lock closure must have version");
						value = package.Version.ToString();
					}

					Log.Diag($"{toolClosure.Key}:{languageName} {packageName} -> {value}");
					packageLock.AddProject(
						toolClosure.Key,
						languageName,
						packageName,
						value,
						null,
						null);
				}
			}
		}

		return packageLock;
	}

	/// <summary>
	/// Recursively discover all dependencies
	/// </summary>
	private async Task<int> EnsureDiscoverDependenciesAsync(
		Path recipeDirectory,
		IDictionary<int, (string Language, string Name, Path Path)> localPackageReverseLookup,
		IDictionary<Path, Api.Client.PackageLocalReferenceModel> localPackageLookup,
		List<Api.Client.PackagePublicReferenceModel> publicPackages)
	{
		var recipePath =
			recipeDirectory +
			BuildConstants.RecipeFileName;
		var (isSuccess, recipe) = await RecipeExtensions.TryLoadRecipeFromFileAsync(recipePath);
		if (!isSuccess)
		{
			throw new InvalidOperationException($"Could not load the recipe file: {recipePath}");
		}

		if (localPackageLookup.TryGetValue(recipeDirectory, out var existingPackage))
		{
			Log.Diag("Recipe already processed.");
			return existingPackage.Id;
		}
		else
		{
			var dependencyIds = await DiscoverDependenciesAsync(
				recipeDirectory,
				recipe,
				localPackageReverseLookup,
				localPackageLookup,
				publicPackages);
			var packageReference = new Api.Client.PackageLocalReferenceModel()
			{
				Id = localPackageLookup.Count + publicPackages.Count + 1,
				Language = new Api.Client.LanguageReferenceModel()
				{
					Name = recipe.Language.Name,
					Version = new Api.Client.SemanticVersionModel()
					{
						Major = recipe.Language.Version.Major,
						Minor = recipe.Language.Version.Minor,
						Patch = recipe.Language.Version.Patch,
					},
				},
				Dependencies = dependencyIds,
			};

			localPackageReverseLookup.Add(packageReference.Id, (recipe.Language.Name, recipe.Name, recipeDirectory));
			localPackageLookup.Add(recipeDirectory, packageReference);
			return packageReference.Id;
		}
	}

	/// <summary>
	/// Recursively discover all local dependencies, assume that the closure has been updated correctly for current recipe
	/// </summary>
	private async Task<IDictionary<string, ICollection<int>>> DiscoverDependenciesAsync(
		Path recipeDirectory,
		Recipe recipe,
		IDictionary<int, (string Language, string Name, Path Path)> localPackageReverseLookup,
		IDictionary<Path, Api.Client.PackageLocalReferenceModel> localPackageLookup,
		List<Api.Client.PackagePublicReferenceModel> publicPackages)
	{
		// Restore the explicit dependencies
		var dependencyIds = new Dictionary<string, ICollection<int>>();
		foreach (var dependencyType in recipe.GetDependencyTypes())
		{
			string? implicitLanguage;
			if (dependencyType == DependencyTypeBuild)
			{
				implicitLanguage = BuiltInLanguageWren;
			}
			else if (dependencyType == DependencyTypeTool)
			{
				// No implicit language
				implicitLanguage = null;
			}
			else
			{
				// Same language as parent is implied
				implicitLanguage = recipe.Language.Name;
			}

			var dependencyTypeIds = await DiscoverTypeDependenciesAsync(
				recipeDirectory,
				recipe,
				dependencyType,
				implicitLanguage,
				localPackageReverseLookup,
				localPackageLookup,
				publicPackages);
			dependencyIds.Add(dependencyType, dependencyTypeIds);
		}

		return dependencyIds;
	}

	/// <summary>
	/// Recursively restore all dependencies
	/// </summary>
	private async Task<IList<int>> DiscoverTypeDependenciesAsync(
		Path recipeDirectory,
		Recipe recipe,
		string dependencyType,
		string? implicitLanguage,
		IDictionary<int, (string Language, string Name, Path Path)> localPackageReverseLookup,
		IDictionary<Path, Api.Client.PackageLocalReferenceModel> localPackageLookup,
		List<Api.Client.PackagePublicReferenceModel> publicPackages)
	{
		var dependencyIds = new List<int>();
		foreach (var dependency in recipe.GetNamedDependencies(dependencyType))
		{
			// If local then check children for external package references
			// Otherwise add the external package reference which will be expanded on the service call
			if (dependency.IsLocal)
			{
				Path dependencyPath = dependency.Path;
				if (!dependencyPath.HasRoot)
					dependencyPath = recipeDirectory + dependencyPath;

				var id = await EnsureDiscoverDependenciesAsync(
					dependencyPath,
					localPackageReverseLookup,
					localPackageLookup,
					publicPackages);
				dependencyIds.Add(id);
			}
			else
			{
				if (dependency.Version == null)
					throw new ArgumentException("Local package version was null");

				var language = (dependency.Language ?? implicitLanguage) ?? throw new ArgumentException("Language required for Tool dependency");
				var owner = dependency.Owner ?? throw new ArgumentException("Owner required for Tool dependency");
				var existingMatch = publicPackages
					.FirstOrDefault(value =>
						value.Name == dependency.Name &&
						value.Owner == owner &&
						value.Version.Major == dependency.Version.Major &&
						value.Version.Minor == dependency.Version.Minor &&
						value.Version.Patch == dependency.Version.Patch &&
						value.Language == language);
				if (existingMatch is not null)
				{
					dependencyIds.Add(existingMatch.Id);
				}
				else
				{
					var publicReference = new Api.Client.PackagePublicReferenceModel()
					{
						Id = localPackageLookup.Count + publicPackages.Count + 1,
						Name = dependency.Name,
						Owner = owner,
						Version = new Api.Client.SemanticVersionModel()
						{
							Major = dependency.Version.Major,
							Minor = dependency.Version.Minor,
							Patch = dependency.Version.Patch,
						},
						Language = language,
					};

					publicPackages.Add(publicReference);
					dependencyIds.Add(publicReference.Id);
				}
			}
		}

		return dependencyIds;
	}

	/// <summary>
	/// Restore package lock
	/// </summary>
	private async Task RestorePackageLockAsync(
		Path packageStore,
		Path stagingDirectory,
		PackageLock packageLock)
	{
		foreach (var closure in packageLock.GetClosures().Values)
		{
			Log.Info($"Restore Packages for Closure {closure.Key}");
			var isRuntime = closure.Key == RootClosureName;
			foreach (var (languageName, languageProjects) in closure.Value.Value.AsTable().Values)
			{
				Log.Info($"Restore Packages for Language {languageName}");
				foreach (var (projectUniqueName, project) in languageProjects.Value.AsTable().Values)
				{
					var projectTable = project.Value.AsTable();
					var projectName = PackageName.Parse(projectUniqueName);
					var projectVersion = projectTable.Values[PackageLock.Property_Version].Value.AsString().Value;
					if (SemanticVersion.TryParse(projectVersion, out var version))
					{
						await EnsurePackageDownloadedAsync(
							isRuntime,
							projectName.Owner ?? throw new InvalidOperationException("Missing owner"),
							languageName,
							projectName.Name,
							version,
							packageStore,
							stagingDirectory);
					}
					else
					{
						Log.Info($"Skip Package: {projectName} -> {projectVersion}");
					}
				}
			}
		}
	}

	/// <summary>
	/// Ensure a package version is downloaded
	/// </summary>
	private async Task EnsurePackageDownloadedAsync(
		bool isRuntime,
		string ownerName,
		string languageName,
		string packageName,
		SemanticVersion packageVersion,
		Path packageStore,
		Path stagingDirectory)
	{
		Log.HighPriority($"Install Package: {languageName} {ownerName} {packageName}@{packageVersion}");

		var languageSafeName = GetLanguageSafeName(languageName);
		var languageRootFolder = packageStore + new Path(languageSafeName);
		var packageRootFolder = languageRootFolder + new Path(ownerName) + new Path(packageName);
		var packageVersionFolder = packageRootFolder + new Path(packageVersion.ToString()) + new Path();

		// Check if the package version already exists
		if (!isRuntime &&
			((packageName == BuiltInLanguagePackageCpp && packageVersion == _builtInLanguageVersionCpp) ||
			(packageName == BuiltInLanguagePackageCSharp && packageVersion == _builtInLanguageVersionCSharp) ||
			(packageName == BuiltInLanguagePackageWren && packageVersion == _builtInLanguageVersionWren)))
		{
			Log.HighPriority("Skip built in language version in build closure");
		}
		else if (LifetimeManager.Get<IFileSystem>().Exists(packageVersionFolder))
		{
			Log.HighPriority("Found local version");
		}
		else
		{
			// Download the archive
			Log.HighPriority("Downloading package");
			var archivePath = stagingDirectory + new Path(packageName + ".zip");

			var client = new Api.Client.PackageVersionsClient(_httpClient, null)
			{
				BaseUrl = _apiEndpoint,
			};

			try
			{
				var result = await client.DownloadPackageVersionAsync(
					languageName, ownerName, packageName, packageVersion.ToString());

				// Write the contents to disk, scope cleanup
				using var archiveWriteFile = LifetimeManager.Get<IFileSystem>().OpenWrite(archivePath, true);
				await result.Stream.CopyToAsync(archiveWriteFile.GetOutStream());
			}
			catch (Api.Client.ApiException ex)
			{
				if (ex.StatusCode == HttpStatusCode.NotFound)
				{
					Log.HighPriority("Package Version Missing");
					throw new HandledException();
				}
				else
				{
					throw;
				}
			}

			// Create the package folder to extract to
			var stagingVersionFolder = stagingDirectory + new Path($"{languageName}_{packageName}_{packageVersion}/");
			LifetimeManager.Get<IFileSystem>().CreateDirectory2(stagingVersionFolder);

			// Unpack the contents of the archive
			LifetimeManager.Get<IZipManager>().ExtractToDirectory(archivePath, stagingVersionFolder);

			// Delete the archive file
			LifetimeManager.Get<IFileSystem>().DeleteFile(archivePath);

			// Ensure the package root folder exists
			if (!LifetimeManager.Get<IFileSystem>().Exists(packageRootFolder))
			{
				// Create the folder
				LifetimeManager.Get<IFileSystem>().CreateDirectory2(packageRootFolder);
			}

			// Move the extracted contents into the version folder
			LifetimeManager.Get<IFileSystem>().Rename(stagingVersionFolder, packageVersionFolder);
		}
	}

	private static string GetLanguageSafeName(string language)
	{
		return language switch
		{
			BuiltInLanguageCSharp => BuiltInLanguageSafeNameCSharp,
			BuiltInLanguageCpp => BuiltInLanguageSafeNameCpp,
			BuiltInLanguageWren => BuiltInLanguageSafeNameWren,
			_ => throw new InvalidOperationException($"Unknown language name: {language}"),
		};
	}

	/// <summary>
	/// Ensure the staging directory exists
	/// </summary>
	private static void EnsureDirectoryExists(Path directory)
	{
		if (!LifetimeManager.Get<IFileSystem>().Exists(directory))
		{
			// Create the folder
			Log.Diag($"Create Directory: {directory}");
			LifetimeManager.Get<IFileSystem>().CreateDirectory2(directory);
		}
	}
}