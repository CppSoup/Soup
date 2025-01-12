// <copyright file="BuildFailedException.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <exception>

export module Soup.Core:BuildFailedException;

import Opal;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The exception thrown to indicate that a build operation has failed
	/// </summary>
	export class BuildFailedException : public std::exception
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildFailedException"/> class.
		/// </summary>
		BuildFailedException()
		{
		}
	};
}
