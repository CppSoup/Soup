// <copyright file="PackageProvider.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "package-lock/PackageLockExtensions.h"
#include "RecipeBuildArguments.h"
#include "recipe/RecipeCache.h"

export namespace Soup::Core
{
	using PackageId = int;
	using PackageGraphId = int;

	class PackageChildInfo
	{
	public:
		PackageChildInfo() :
			OriginalReference(),
			IsSubGraph(false),
			PackageId(-1),
			PackageGraphId(-1)
		{
		}

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
		::Soup::Core::PackageId PackageId;
		::Soup::Core::PackageGraphId PackageGraphId;

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

	using PackageChildrenMap = std::map<std::string, std::vector<PackageChildInfo>>;

	class PackageInfo
	{
	public:
		PackageInfo(
			PackageId id,
			PackageName name,
			bool isPrebuilt,
			Path packageRoot,
			Path targetDirectory,
			const Recipe* recipe,
			PackageChildrenMap dependencies) :
			Id(id),
			Name(std::move(name)),
			IsPrebuilt(isPrebuilt),
			PackageRoot(std::move(packageRoot)),
			TargetDirectory(std::move(targetDirectory)),
			Recipe(recipe),
			Dependencies(std::move(dependencies))
		{
		}

		PackageId Id;
		PackageName Name;
		bool IsPrebuilt;
		Path PackageRoot;
		Path TargetDirectory;
		const ::Soup::Core::Recipe* Recipe;
		PackageChildrenMap Dependencies;

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const PackageInfo& rhs) const
		{
			return Id == rhs.Id &&
				Name == rhs.Name &&
				IsPrebuilt == rhs.IsPrebuilt &&
				PackageRoot == rhs.PackageRoot &&
				TargetDirectory == rhs.TargetDirectory &&
				Recipe == rhs.Recipe &&
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

	class PackageGraph
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
	using PackageGraphLookupMap = std::map<PackageGraphId, PackageGraph>;
	using PackageLookupMap = std::map<PackageId, PackageInfo>;
	class PackageProvider
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

		PackageGraphId GetRootPackageGraphId()
		{
			return _rootPackageGraphId;
		}

		const PackageGraphLookupMap& GetPackageGraphLookup()
		{
			return _packageGraphLookup;
		}

		const PackageLookupMap& GetPackageLookup()
		{
			return _packageLookup;
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
				throw std::runtime_error(
					std::format("PackageGraphId [{}] not found in lookup", packageGraphId));
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
				throw std::runtime_error(
					std::format("packageId [{}] not found in lookup", packageId));
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
