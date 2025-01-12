// <copyright file="MacroManager.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <map>
#include <string>

export module Soup.Core:MacroManager;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The macro manager handles all things macro... It just replaces stuff.
	/// </summary>
	export class MacroManager
	{
	private:
		const std::map<std::string, std::string>& _macros;

	public:
		MacroManager(
			const std::map<std::string, std::string>& macros) :
			_macros(macros)
		{
		}

		Path ResolveMacros(Path value)
		{
			// TODO: Is there a way to not process the path again?
			auto rawValue = std::move(value.ToString());
			return Path(ResolveMacros(std::move(rawValue)));
		}

		std::string ResolveMacros(std::string value)
		{
			for (auto& [macro, macroValue] : _macros)
			{
				for(size_t i = 0; ; i += macroValue.length())
				{
					// Find the next instance of the macro
					i = value.find(macro, i);

					// No more instances, early exit
					if(i == std::string::npos)
						break;

					// Erase the macro and insert the real value
					// TODO: Less than ideal, but gets the job done
					value.erase(i, macro.length());
					value.insert(i, macroValue);
				}
			}

			return value;
		}
	};
}