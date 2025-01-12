// <copyright file="IBuildEvaluateEngine.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "operation-graph/OperationGraph.h"

namespace Soup::Core
{
	/// <summary>
	/// The core build evaluation interface that knows how to perform a build from a provided Operation Graph.
	/// </summary>
	export class IEvaluateEngine
	{
	public:
		/// <summary>
		/// Execute the entire operation graph that is referenced by this build evaluate engine
		/// Returns true if any of the operations were evaluated
		/// </summary>
		virtual bool Evaluate(
			const OperationGraph& operationGraph,
			OperationResults& operationResults,
			const Path& temporaryDirectory,
			const std::vector<Path>& globalAllowedReadAccess,
			const std::vector<Path>& globalAllowedWriteAccess) = 0;
	};
}
