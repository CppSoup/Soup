// <copyright file="PackageLockExtensions.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "PackageLock.h"
#include "recipe/RecipeSML.h"

namespace Soup::Core
{
	/// <summary>
	/// The package lock extensions
	/// </summary>
	export class PackageLockExtensions
	{
	public:
		/// <summary>
		/// Attempt to load from file
		/// </summary>
		static bool TryLoadFromFile(
			const Path& packageLockFile,
			PackageLock& result)
		{
			// Open the file to read from
			Log::Diag("Load PackageLock: {}", packageLockFile.ToString());
			std::shared_ptr<System::IInputFile> file;
			if (!System::IFileSystem::Current().TryOpenRead(packageLockFile, true, file))
			{
				Log::Info("PackageLock file does not exist");
				return false;
			}

			// Read the contents of the recipe file
			try
			{
				result = PackageLock(
					RecipeSML::Deserialize(
						packageLockFile,
						file->GetInStream()));
				return true;
			}
			catch (std::exception& ex)
			{
				Log::Error("Deserialize Threw: {}", ex.what());
				Log::Info("Failed to parse PackageLock.");
				return false;
			}
		}
	};
}
