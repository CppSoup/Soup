// <copyright file="PackageManager.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup::Core
{
	/// <summary>
	/// The package manager wrapper that sends requests along to the managed implementation
	/// </summary>
	export class PackageManager
	{
	public:
		/// <summary>
		/// Restore packages
		/// </summary>
		static void RestorePackages(const Path& workingDirectory)
		{
			Log::Info("RestorePackages");

			auto arguments = std::vector<std::string>(
			{
				"restore-packages",
				workingDirectory.ToString(),
			});

			RunCommand(std::move(arguments));
		}

		/// <summary>
		/// Initialize a package
		/// </summary>
		static void InitializePackage(const Path& workingDirectory)
		{
			Log::Info("InitializePackage");

			auto arguments = std::vector<std::string>(
			{
				"initialize-package",
				workingDirectory.ToString(),
			});

			RunCommand(std::move(arguments));
		}

		/// <summary>
		/// Install a package
		/// </summary>
		static void InstallPackageReference(const Path& workingDirectory, const std::string& packageReference)
		{
			Log::Info("InstallPackageReference");

			auto arguments = std::vector<std::string>(
			{
				"install-package",
				workingDirectory.ToString(),
				packageReference,
			});

			RunCommand(std::move(arguments));
		}

		/// <summary>
		/// Publish a package
		/// </summary>
		static void PublishPackage(const Path& workingDirectory)
		{
			Log::Info("PublishPackage");

			auto arguments = std::vector<std::string>(
			{
				"publish-package",
				workingDirectory.ToString(),
			});

			RunCommand(std::move(arguments));
		}

	private:
		static void RunCommand(std::vector<std::string> arguments)
		{
			auto moduleName = System::IProcessManager::Current().GetCurrentProcessFileName();
			auto moduleFolder = moduleName.GetParent();
			auto packageManagerFolder = moduleFolder + Path("./PackageManager/");
			#if defined(_WIN32)
			auto executable = packageManagerFolder + Path("./Soup.Build.PackageManager.exe");
			#elif defined(__linux__)
			auto executable = packageManagerFolder + Path("./Soup.Build.PackageManager");
			#else
			#error "Unknown platform"
			#endif

			// Log diagnostic information
			std::stringstream message;
			message << "  " << executable.ToString();
			for (auto& argument : arguments)
				message << " " << argument;

			Log::Info("Running PackageManager");
			Log::Diag(message.str());

			// Execute the requested target
			auto process = System::IProcessManager::Current().CreateProcess(
				executable,
				std::move(arguments),
				packageManagerFolder,
				false);
			process->Start();
			process->WaitForExit();

			auto exitCode = process->GetExitCode();

			if (exitCode != 0)
			{
				Log::Error("Package Manager Failed");
				throw HandledException(exitCode);
			}
		}
	};
}
