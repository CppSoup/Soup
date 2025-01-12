// <copyright file="DependencyTargetSet.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <map>
#include <set>
#include <string>

export module Soup.Core:DependencyTargetSet;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The dependency target set used to track all directories and macros in use by a package build runtime
	/// </summary>
	class DependencyTargetSet
	{
	public:
		// The active set of macros that are only used to ensure the subgraph macros do not
		// collide with package versions in the runtime closure
		std::map<std::string, std::string> GenerateSubGraphMacros;

		// The active set of macros that are only used by this package during generate
		std::map<std::string, std::string> GenerateCurrentMacros;

		// The active set of directories that can accessed during generate by this package only
		std::set<Path> GenerateCurrentReadDirectories;
		std::set<Path> GenerateCurrentWriteDirectories;

		// The active set of macros that are only used by this package during evaluate
		std::map<std::string, std::string> EvaluateCurrentMacros;

		// The recursive set of macros that all upstream dependencies can utilize during evaluate
		std::map<std::string, std::string> EvaluateRecursiveMacros;

		// The active set of directories that can accessed during evaluate by this package only
		std::set<Path> EvaluateCurrentReadDirectories;
		std::set<Path> EvaluateCurrentWriteDirectories;

		// The recursive set of directories that can be read during evaluate
		std::set<Path> EvaluateRecursiveReadDirectories;

	public:
		DependencyTargetSet() :
			GenerateSubGraphMacros(),
			GenerateCurrentMacros(),
			GenerateCurrentReadDirectories(),
			GenerateCurrentWriteDirectories(),
			EvaluateCurrentMacros(),
			EvaluateRecursiveMacros(),
			EvaluateCurrentReadDirectories(),
			EvaluateCurrentWriteDirectories(),
			EvaluateRecursiveReadDirectories()
		{
		}
	};
}