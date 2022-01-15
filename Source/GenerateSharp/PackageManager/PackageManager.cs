﻿// <copyright file="PackageManager.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright

namespace Soup.Build.PackageManager
{
	using System;
	using System.Collections.Generic;
	using System.IO.Compression;
	using System.Net.Http;
	using System.Threading.Tasks;
	using Opal;
	using Opal.System;
	using Soup.Build.Utilities;

	/// <summary>
	/// The package manager
	/// </summary>
	public class PackageManager
	{
		private static string StagingFolderName => ".staging/";

		//// private static string SoupApiEndpoint => "https://localhost:7071";
		private static string SoupApiEndpoint => "https://api.soupbuild.com";

		/// <summary>
		/// Restore packages
		/// </summary>
		public static async Task RestorePackagesAsync(Path workingDirectory)
		{
			var packageStore = LifetimeManager.Get<IFileSystem>().GetUserProfileDirectory() +
				new Path(".soup/packages/");
			Log.Diag("Using Package Store: " + packageStore.ToString());

			// Create the staging directory
			var stagingPath = EnsureStagingDirectoryExists(packageStore);

			try
			{
				var packageLockPath =
					workingDirectory +
					BuildConstants.PackageLockFileName;
				var loadPackageLock = await PackageLockExtensions.TryLoadFromFileAsync(packageLockPath);
				if (loadPackageLock.IsSuccess)
				{
					Log.Info("Restore from package lock");
					await RestorePackageLockAsync(
						packageStore,
						stagingPath,
						loadPackageLock.Result);
				}
				else
				{
					Log.Info("Discovering full closure");
					var closure = new Dictionary<string, IDictionary<string, PackageReference>>();
					await RestoreRecursiveDependenciesAsync(
						workingDirectory,
						packageStore,
						stagingPath,
						closure);

					// Build up the package lock file
					var packageLock = new PackageLock();
					foreach (var languageClosure in closure)
					{
						foreach (var package in languageClosure.Value)
						{
							Log.Diag($"{languageClosure.Key} {package.Key} -> {package.Value}");
							packageLock.AddProject(languageClosure.Key, package.Key, package.Value.ToString());
						}
					}

					// Save the updated package lock
					await PackageLockExtensions.SaveToFileAsync(packageLockPath, packageLock);
				}

				// Cleanup the working directory
				Log.Diag("Deleting staging directory");
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);
			}
			catch (Exception)
			{
				// Cleanup the staging directory and accept that we failed
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);
				throw;
			}
		}

		/// <summary>
		/// Install a package
		/// </summary>
		public static async Task InstallPackageReferenceAsync(Path workingDirectory, string packageReference)
		{
			var recipePath =
				workingDirectory +
				BuildConstants.RecipeFileName;
			var (isSuccess, recipe) = await RecipeExtensions.TryLoadRecipeFromFileAsync(recipePath);
			if (!isSuccess)
			{
				throw new InvalidOperationException("Could not load the recipe file.");
			}

			var packageStore = LifetimeManager.Get<IFileSystem>().GetUserProfileDirectory() +
				new Path(".soup/packages/");
			Log.Info("Using Package Store: " + packageStore.ToString());

			// Create the staging directory
			var stagingPath = EnsureStagingDirectoryExists(packageStore);

			try
			{
				// Parse the package reference to get the name
				var targetPackageReference = PackageReference.Parse(packageReference);
				string packageName = packageReference;
				if (!targetPackageReference.IsLocal)
				{
					packageName = targetPackageReference.GetName;
				}

				// Check if the package is already installed
				var packageNameNormalized = packageName.ToUpperInvariant();
				if (recipe.HasRuntimeDependencies)
				{
					foreach (var dependency in recipe.RuntimeDependencies)
					{
						if (!dependency.IsLocal)
						{
							var dependencyNameNormalized = dependency.GetName.ToUpperInvariant();
							if (dependencyNameNormalized == packageNameNormalized)
							{
								Log.Warning("Package already installed.");
								return;
							}
						}
					}
				}

				// Get the latest version if no version provided
				if (targetPackageReference.IsLocal)
				{
					var packageModel = await GetPackageModelAsync(recipe.Language, packageName);
					var latestVersion = new SemanticVersion(packageModel.Latest.Major, packageModel.Latest.Minor, packageModel.Latest.Patch);
					Log.HighPriority("Latest Version: " + latestVersion.ToString());
					targetPackageReference = new PackageReference(packageModel.Name, latestVersion);
				}

				var closure = new Dictionary<string, IDictionary<string, PackageReference>>();
				await CheckRecursiveEnsurePackageDownloadedAsync(
					recipe.Language,
					targetPackageReference.GetName,
					targetPackageReference.Version,
					packageStore,
					stagingPath,
					closure);

				// Cleanup the working directory
				Log.Info("Deleting staging directory");
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);

				// Register the package in the recipe
				Log.Info("Adding reference to recipe");
				recipe.AddRuntimeDependency(targetPackageReference.ToString());

				// Save the state of the recipe
				await RecipeExtensions.SaveToFileAsync(recipePath, recipe);
			}
			catch (Exception)
			{
				// Cleanup the staging directory and accept that we failed
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);
				throw;
			}
		}

		/// <summary>
		/// Publish a package
		/// </summary>
		public static async Task PublishPackageAsync(Path workingDirectory)
		{
			Log.Info($"Publish Project: {workingDirectory}");

			var recipePath =
				workingDirectory +
				BuildConstants.RecipeFileName;
			var (isSuccess, recipe) = await RecipeExtensions.TryLoadRecipeFromFileAsync(recipePath);
			if (!isSuccess)
			{
				throw new InvalidOperationException("Could not load the recipe file.");
			}

			var packageStore = LifetimeManager.Get<IFileSystem>().GetUserProfileDirectory() +
				new Path(".soup/packages/");
			Log.Info("Using Package Store: " + packageStore.ToString());

			// Create the staging directory
			var stagingPath = EnsureStagingDirectoryExists(packageStore);

			try
			{
				var archivePath = stagingPath + new Path(recipe.Name + ".zip");

				// Create the archive of the package
				using (var writeArchiveFile = LifetimeManager.Get<IFileSystem>().OpenWrite(archivePath, true))
				using (var zipArchive = new ZipArchive(writeArchiveFile.GetOutStream(), ZipArchiveMode.Create, false))
				{
					AddPackageFiles(workingDirectory, zipArchive);
				}

				// Authenticate the user
				Log.Info("Request Authentication Token");
				var accessToken = await AuthenticationManager.EnsureSignInAsync();

				// Publish the archive
				Log.Info("Publish package");
				using (var httpClient = new HttpClient())
				{
					var packageClient = new Api.Client.PackageClient(httpClient)
					{
						BaseUrl = SoupApiEndpoint,
						BearerToken = accessToken,
					};

					// Check if the package exists
					bool packageExists = false;
					try
					{
						var package = await packageClient.GetPackageAsync(recipe.Language, recipe.Name);
						packageExists = true;
					}
					catch (Api.Client.ApiException ex)
					{
						if (ex.StatusCode == 404)
						{
							packageExists = false;
						}
						else
						{
							throw;
						}
					}

					// Create the package if it does not exist
					if (!packageExists)
					{
						var createPackageModel = new Api.Client.PackageCreateOrUpdateModel()
						{
							Description = string.Empty,
						};
						await packageClient.CreateOrUpdatePackageAsync(recipe.Language, recipe.Name, createPackageModel);
					}

					var packageVersionClient = new Api.Client.PackageVersionClient(httpClient)
					{
						BaseUrl = SoupApiEndpoint,
						BearerToken = accessToken,
					};

					using (var readArchiveFile = LifetimeManager.Get<IFileSystem>().OpenRead(archivePath))
					{
						try
						{
							await packageVersionClient.PublishPackageVersionAsync(
								recipe.Language,
								recipe.Name,
								recipe.Version.ToString(),
								new Api.Client.FileParameter(readArchiveFile.GetInStream(), string.Empty, "application/zip"));

							Log.Info("Package published");
						}
						catch (Api.Client.ApiException ex)
						{
							if (ex.StatusCode == 409)
							{
								Log.Info("Package version already exists");
							}
							else
							{
								throw;
							}
						}
					}
				}

				// Cleanup the staging directory
				Log.Info("Cleanup staging directory");
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);
			}
			catch (Exception)
			{
				// Cleanup the staging directory and accept that we failed
				Log.Info("Publish Failed: Cleanup staging directory");
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingPath, true);
				throw;
			}
		}

		private static async Task<Api.Client.PackageModel> GetPackageModelAsync(string languageName, string packageName)
		{
			using (var httpClient = new HttpClient())
			{
				var client = new Api.Client.PackageClient(httpClient)
				{
					BaseUrl = SoupApiEndpoint,
				};
				return await client.GetPackageAsync(languageName, packageName);
			}
		}

		private static void AddPackageFiles(Path workingDirectory, ZipArchive archive)
		{
			foreach (var child in LifetimeManager.Get<IFileSystem>().GetDirectoryChildren(workingDirectory))
			{
				if (child.IsDirectory)
				{
					// Ignore output folder
					if (child.Path.GetFileName() != "out")
					{
						AddAllFilesRecursive(child.Path, workingDirectory, archive);
					}
				}
				else
				{
					var relativePath = child.Path.GetRelativeTo(workingDirectory);
					var relativeName = relativePath.ToString().Substring(2);
					var fileEentry = archive.CreateEntryFromFile(child.Path.ToString(), relativeName);
				}
			}
		}

		private static void AddAllFilesRecursive(Path directory, Path workingDirectory, ZipArchive archive)
		{
			foreach (var child in LifetimeManager.Get<IFileSystem>().GetDirectoryChildren(directory))
			{
				if (child.IsDirectory)
				{
					AddAllFilesRecursive(child.Path, workingDirectory, archive);
				}
				else
				{
					var relativePath = child.Path.GetRelativeTo(workingDirectory);
					var relativeName = relativePath.ToString().Substring(2);
					var fileEentry = archive.CreateEntryFromFile(child.Path.ToString(), relativeName);
				}
			}
		}

		/// <summary>
		/// Ensure a package version is downloaded
		/// </summary>
		private static async Task CheckRecursiveEnsurePackageDownloadedAsync(
			string languageName,
			string packageName,
			 SemanticVersion packageVersion,
			 Path packagesDirectory,
			 Path stagingDirectory,
			 IDictionary<string, IDictionary<string, PackageReference>> closure)
		{
			if (closure.ContainsKey(languageName) && closure[languageName].ContainsKey(packageName))
			{
				Log.Diag("Recipe already processed.");
			}
			else
			{
				// Add the new package to the closure
				if (!closure.ContainsKey(languageName))
					closure.Add(languageName, new Dictionary<string, PackageReference>());
				closure[languageName].Add(packageName, new PackageReference(packageName, packageVersion));

				Log.HighPriority($"Install Package: {languageName} {packageName}@{packageVersion}");

				var languageRootFolder = packagesDirectory + new Path(languageName);
				var packageRootFolder = languageRootFolder + new Path(packageName);
				var packageVersionFolder = packageRootFolder + new Path(packageVersion.ToString()) + new Path("/");

				// Check if the package version already exists
				if (LifetimeManager.Get<IFileSystem>().Exists(packageVersionFolder))
				{
					Log.HighPriority("Found local version");
				}
				else
				{
					// Download the archive
					Log.HighPriority("Downloading package");
					var archivePath = stagingDirectory + new Path(packageName + ".zip");
					using (var httpClient = new HttpClient())
					{
						var client = new Api.Client.PackageVersionClient(httpClient)
						{
							BaseUrl = SoupApiEndpoint,
						};

						try
						{
							var result = await client.DownloadPackageVersionAsync(languageName, packageName, packageVersion.ToString());

							// Write the contents to disk, scope cleanup
							using (var archiveWriteFile = LifetimeManager.Get<IFileSystem>().OpenWrite(archivePath, true))
							{
								await result.Stream.CopyToAsync(archiveWriteFile.GetOutStream());
							}
						}
						catch (Api.Client.ApiException ex)
						{
							if (ex.StatusCode == 404)
							{
								Log.HighPriority("Package Version Missing");
								throw new HandledException();
							}
							else
							{
								throw;
							}
						}
					}

					// Create the package folder to extract to
					var stagingVersionFolder = stagingDirectory + new Path($"{languageName}_{packageName}_{packageVersion}/");
					LifetimeManager.Get<IFileSystem>().CreateDirectory2(stagingVersionFolder);

					// Unpack the contents of the archive
					ZipFile.ExtractToDirectory(archivePath.ToString(), stagingVersionFolder.ToString());

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

					// Install recursive dependencies
					await RestoreRecursiveDependenciesAsync(
						packageVersionFolder,
						packagesDirectory,
						stagingDirectory,
						closure);
				}
			}
		}

		/// <summary>
		/// Ensure a package version is downloaded
		/// </summary>
		private static async Task EnsurePackageDownloadedAsync(
			string languageName,
			string packageName,
			 SemanticVersion packageVersion,
			 Path packagesDirectory,
			 Path stagingDirectory)
		{
			Log.HighPriority($"Install Package: {languageName} {packageName}@{packageVersion}");

			var languageRootFolder = packagesDirectory + new Path(languageName);
			var packageRootFolder = languageRootFolder + new Path(packageName);
			var packageVersionFolder = packageRootFolder + new Path(packageVersion.ToString()) + new Path("/");

			// Check if the package version already exists
			if (LifetimeManager.Get<IFileSystem>().Exists(packageVersionFolder))
			{
				Log.HighPriority("Found local version");
			}
			else
			{
				// Download the archive
				Log.HighPriority("Downloading package");
				var archivePath = stagingDirectory + new Path(packageName + ".zip");
				using (var httpClient = new HttpClient())
				{
					var client = new Api.Client.PackageVersionClient(httpClient)
					{
						BaseUrl = SoupApiEndpoint,
					};

					try
					{
						var result = await client.DownloadPackageVersionAsync(languageName, packageName, packageVersion.ToString());

						// Write the contents to disk, scope cleanup
						using (var archiveWriteFile = LifetimeManager.Get<IFileSystem>().OpenWrite(archivePath, true))
						{
							await result.Stream.CopyToAsync(archiveWriteFile.GetOutStream());
						}
					}
					catch (Api.Client.ApiException ex)
					{
						if (ex.StatusCode == 404)
						{
							Log.HighPriority("Package Version Missing");
							throw new HandledException();
						}
						else
						{
							throw;
						}
					}
				}

				// Create the package folder to extract to
				var stagingVersionFolder = stagingDirectory + new Path($"{languageName}_{packageName}_{packageVersion}/");
				LifetimeManager.Get<IFileSystem>().CreateDirectory2(stagingVersionFolder);

				// Unpack the contents of the archive
				ZipFile.ExtractToDirectory(archivePath.ToString(), stagingVersionFolder.ToString());

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

		/// <summary>
		/// Restore package lock
		/// </summary>
		static async Task RestorePackageLockAsync(
			Path packagesDirectory,
			Path stagingDirectory,
			PackageLock packageLock)
		{

			foreach (var languageProjects in packageLock.GetProjects())
			{
				foreach (var project in languageProjects.Value.AsList())
				{
					var projectTable = project.AsTable();
					var packageReference = PackageReference.Parse(projectTable["Version"].AsString());
					if (!packageReference.IsLocal)
					{
						await EnsurePackageDownloadedAsync(
							languageProjects.Key,
							packageReference.GetName,
							packageReference.Version,
							packagesDirectory,
							stagingDirectory);
					}
				}
			}
		}

		/// <summary>
		/// Recursively restore all dependencies
		/// </summary>
		static async Task RestoreRecursiveDependenciesAsync(
			Path recipeDirectory,
			Path packagesDirectory,
			Path stagingDirectory,
			IDictionary<string, IDictionary<string, PackageReference>> closure)
		{
			var recipePath =
				recipeDirectory +
				BuildConstants.RecipeFileName;
			var (isSuccess, recipe) = await RecipeExtensions.TryLoadRecipeFromFileAsync(recipePath);
			if (!isSuccess)
			{
				throw new InvalidOperationException("Could not load the recipe file.");
			}

			if (closure.ContainsKey(recipe.Language) && closure[recipe.Language].ContainsKey(recipe.Name))
			{
				Log.Diag("Recipe already processed.");
			}
			else
			{
				// Add the project to the closure
				if (!closure.ContainsKey(recipe.Language))
					closure.Add(recipe.Language, new Dictionary<string, PackageReference>());
				closure[recipe.Language].Add(recipe.Name, new PackageReference(recipeDirectory));

				foreach (var dependecyType in recipe.GetDependencyTypes())
				{
					if (recipe.HasNamedDependencies(dependecyType))
					{
						// Same language as parent is implied
						var implicitLanguage = recipe.Language;

						// Build dependencies do not inherit the parent language
						// Instead, they default to C#
						if (dependecyType == "Build")
						{
							implicitLanguage = "C#";
						}

						foreach (var dependency in recipe.GetNamedDependencies(dependecyType))
						{
							// If local then check children for external package references
							// Otherwise install the external package reference and its dependencies
							if (dependency.IsLocal)
							{
								var dependencyPath = recipeDirectory + dependency.Path;
								await RestoreRecursiveDependenciesAsync(
									dependencyPath,
									packagesDirectory,
									stagingDirectory,
									closure);
							}
							else
							{
								await CheckRecursiveEnsurePackageDownloadedAsync(
									implicitLanguage,
									dependency.GetName,
									dependency.Version,
									packagesDirectory,
									stagingDirectory,
									closure);
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Ensure the staging directory exists
		/// </summary>
		static Path EnsureStagingDirectoryExists(Path packageStore)
		{
			var stagingDirectory = packageStore + new Path(StagingFolderName);
			if (LifetimeManager.Get<IFileSystem>().Exists(stagingDirectory))
			{
				Log.Warning("The staging directory already exists from a previous failed run");
				LifetimeManager.Get<IFileSystem>().DeleteDirectory(stagingDirectory, true);
			}

			// Create the folder
			LifetimeManager.Get<IFileSystem>().CreateDirectory2(stagingDirectory);

			return stagingDirectory;
		}
	}
}
