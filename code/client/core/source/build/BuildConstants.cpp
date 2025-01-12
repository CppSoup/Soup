// <copyright file="BuildConstants.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

export module Soup.Core:BuildConstants;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The constants used throughout the build
	/// </summary>
	export class BuildConstants
	{
	public:
		static const Path& EvaluateGraphFileName()
		{
			static const auto value = Path("./Evaluate.bog");
			return value;
		}

		static const Path& EvaluateResultsFileName()
		{
			static const auto value = Path("./Evaluate.bor");
			return value;
		}

		static const Path& GenerateInputFileName()
		{
			static const auto value = Path("./GenerateInput.bvt");
			return value;
		}

		static const Path& GenerateSharedStateFileName()
		{
			static const auto value = Path("./GenerateSharedState.bvt");
			return value;
		}

		static const Path& GenerateResultsFileName()
		{
			static const auto value = Path("./Generate.bor");
			return value;
		}

		static const Path& LocalUserConfigFileName()
		{
			static const auto value = Path("./LocalUserConfig.sml");
			return value;
		}

		static const Path& PackageLockFileName()
		{
			static const auto value = Path("./PackageLock.sml");
			return value;
		}

		static const Path& SoupLocalStoreDirectory()
		{
			static const auto value = Path("./.soup/");
			return value;
		}

		static const Path& RecipeFileName()
		{
			static const auto value = Path("./Recipe.sml");
			return value;
		}

		static const Path& GenerateInfoFileName()
		{
			static const auto value = Path("./GenerateInfo.bvt");
			return value;
		}

		static const Path& SoupTargetDirectory()
		{
			static const auto value = Path("./.soup/");
			return value;
		}

		static const Path& TemporaryFolderName()
		{
			static const auto value = Path("./temp/");
			return value;
		}
	};
}
