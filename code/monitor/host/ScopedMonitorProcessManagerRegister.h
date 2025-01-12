// <copyright file="ScopedMonitorProcessManagerRegister.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "IMonitorProcessManager.h"

namespace Monitor
{
	/// <summary>
	/// A scoped process manager registration helper
	/// </summary>
	export class ScopedMonitorProcessManagerRegister
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref='ScopedMonitorProcessManagerRegister'/> class.
		/// </summary>
		ScopedMonitorProcessManagerRegister(std::shared_ptr<IMonitorProcessManager> listener)
		{
			IMonitorProcessManager::Register(std::move(listener));
		}

		/// <summary>
		/// Finalizes an instance of the <see cref='ScopedMonitorProcessManagerRegister'/> class.
		/// </summary>
		~ScopedMonitorProcessManagerRegister()
		{
			IMonitorProcessManager::Register(nullptr);
		}
	};
}
