// <copyright file="IValue.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

using System.Collections.Generic;

namespace Soup.Build
{
	public interface IValueList : IReadOnlyList<IValue>
	{
		void Add(IValue item);

		void Clear();
	}
}
