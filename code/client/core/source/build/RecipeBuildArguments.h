// <copyright file="RecipeBuildArguments.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup::Core
{
	/// <summary>
	/// The set of build arguments for building a recipe
	/// </summary>
	export struct RecipeBuildArguments
	{
		/// <summary>
		/// Gets or sets set of host platform name
		/// </summary>
		std::string HostPlatform;

		/// <summary>
		/// Gets or sets set of global arguments
		/// </summary>
		ValueTable GlobalParameters;

		/// <summary>
		/// Gets or sets the working directory
		/// </summary>
		Path WorkingDirectory;

		/// <summary>
		/// Gets or sets a value indicating whether to skip running the build generate phase
		/// </summary>
		bool SkipGenerate;

		/// <summary>
		/// Gets or sets a value indicating whether to skip running the build evaluate phase
		/// </summary>
		bool SkipEvaluate;

		/// <summary>
		/// Gets or sets a value indicating whether to disable monitoring
		/// </summary>
		bool DisableMonitor;

		/// <summary>
		/// Gets or sets a value indicating whether to enable partial monitoring
		/// </summary>
		bool PartialMonitor;

		/// <summary>
		/// Gets or sets a value indicating whether to force a rebuild
		/// </summary>
		bool ForceRebuild;

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const RecipeBuildArguments& rhs) const
		{
			return GlobalParameters == rhs.GlobalParameters &&
				WorkingDirectory == rhs.WorkingDirectory &&
				SkipGenerate == rhs.SkipGenerate &&
				SkipEvaluate == rhs.SkipEvaluate &&
				ForceRebuild == rhs.ForceRebuild;
		}

		bool operator !=(const RecipeBuildArguments& rhs) const
		{
			return !(*this == rhs);
		}
	};
}
