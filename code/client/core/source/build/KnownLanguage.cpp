// <copyright file="KnownLanguage.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <string>

export module Soup.Core:KnownLanguage;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	export struct KnownLanguage
	{
		KnownLanguage(
			std::string extensionOwner,
			std::string extensionName) :
			ExtensionOwner(std::move(extensionOwner)),
			ExtensionName(std::move(extensionName))
		{
		}

		std::string ExtensionOwner;
		std::string ExtensionName;
	};
}
