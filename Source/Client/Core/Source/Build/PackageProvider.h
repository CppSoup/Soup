﻿// <copyright file="PackageProvider.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "PackageLock/PackageLockExtensions.h"
#include "RecipeBuildArguments.h"
#include "Recipe/RecipeCache.h"
#include "Utils/HandledException.h"
#include "BuildConstants.h"

namespace Soup::Core
{
	export using PackageId = int;
	export using PackageGraphId = int;
	export class PackageChildInfo
	{
	public:
		PackageChildInfo(
			PackageReference originalReference,
			bool isSubGraph,
			PackageId packageId,
			PackageGraphId packageGraphId) :
			OriginalReference(originalReference),
			IsSubGraph(isSubGraph),
			PackageId(packageId),
			PackageGraphId(packageGraphId)
		{
		}

		PackageReference OriginalReference;
		bool IsSubGraph;
		PackageId PackageId;
		PackageGraphId PackageGraphId;

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageChildInfo& rhs) const
		{
			return OriginalReference == rhs.OriginalReference &&
				IsSubGraph == rhs.IsSubGraph &&
				PackageId == rhs.PackageId &&
				PackageGraphId == rhs.PackageGraphId;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageChildInfo& rhs) const
		{
			return !(*this == rhs);
		}
	};

	export using PackageChildrenMap = std::map<std::string, std::vector<PackageChildInfo>>;
	export class PackageInfo
	{
	public:
		PackageInfo(
			PackageId id,
			Path packageRoot,
			const Recipe& recipe,
			std::optional<Path> languageExtension,
			PackageChildrenMap dependencies) :
			Id(id),
			PackageRoot(std::move(packageRoot)),
			Recipe(recipe),
			LanguageExtension(std::move(languageExtension)),
			Dependencies(std::move(dependencies))
		{
		}

		PackageId Id;
		Path PackageRoot;
		const Recipe& Recipe;
		std::optional<Path> LanguageExtension;
		PackageChildrenMap Dependencies;

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageInfo& rhs) const
		{
			return Id == rhs.Id &&
				PackageRoot == rhs.PackageRoot &&
				Recipe == rhs.Recipe &&
				LanguageExtension == rhs.LanguageExtension &&
				Dependencies == rhs.Dependencies;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageInfo& rhs) const
		{
			return !(*this == rhs);
		}
	};

	export class PackageGraph
	{
	public:
		PackageGraph(
			PackageGraphId id,
			PackageId rootPackageId,
			ValueTable globalParameters) :
			Id(id),
			RootPackageId(rootPackageId),
			GlobalParameters(std::move(globalParameters))
		{
		}

		PackageGraphId Id;
		PackageId RootPackageId;
		ValueTable GlobalParameters;

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageGraph& rhs) const
		{
			return Id == rhs.Id &&
				RootPackageId == rhs.RootPackageId &&
				GlobalParameters == rhs.GlobalParameters;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageGraph& rhs) const
		{
			return !(*this == rhs);
		}
	};

	/// <summary>
	/// The package provider that maintains the in memory representation of all build state
	/// </summary>
	export using PackageGraphLookupMap = std::map<PackageGraphId, PackageGraph>;
	export using PackageLookupMap = std::map<PackageId, PackageInfo>;
	export class PackageProvider
	{
	private:
		PackageGraphId _rootPackageGraphId;
		PackageGraphLookupMap _packageGraphLookup;
		PackageLookupMap _packageLookup;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="PackageProvider"/> class.
		/// </summary>
		PackageProvider(
			PackageGraphId rootPackageGraphId,
			PackageGraphLookupMap packageGraphLookup,
			PackageLookupMap packageLookup) :
			_rootPackageGraphId(rootPackageGraphId),
			_packageGraphLookup(std::move(packageGraphLookup)),
			_packageLookup(std::move(packageLookup))
		{
		}

		const PackageGraph& GetRootPackageGraph()
		{
			return GetPackageGraph(_rootPackageGraphId);
		}

		const PackageGraph& GetPackageGraph(PackageGraphId packageGraphId)
		{
			// The PackageGraph must already be loaded
			auto findPackageGraph = _packageGraphLookup.find(packageGraphId);
			if (findPackageGraph != _packageGraphLookup.end())
			{
				return findPackageGraph->second;
			}
			else
			{
				throw std::runtime_error("PackageGraphId [" + std::to_string(packageGraphId) + "] not found in lookup");
			}
		}

		const PackageInfo& GetPackageInfo(PackageId packageId)
		{
			// The PackageInfo must already be loaded
			auto findPackageInfo = _packageLookup.find(packageId);
			if (findPackageInfo != _packageLookup.end())
			{
				return findPackageInfo->second;
			}
			else
			{
				throw std::runtime_error("packageId [" + std::to_string(packageId) + "] not found in lookup");
			}
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageProvider& rhs) const
		{
			return _rootPackageGraphId == rhs._rootPackageGraphId &&
				_packageGraphLookup == rhs._packageGraphLookup &&
				_packageLookup == rhs._packageLookup;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const PackageProvider& rhs) const
		{
			return !(*this == rhs);
		}
	};
}