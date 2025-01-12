// <copyright file="IMonitorProcessManager.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "ISystemAccessMonitor.h"

namespace Monitor
{
	/// <summary>
	/// The process manager interface that supports monitoring
	/// Interface mainly used to allow for unit testing client code
	/// </summary>
	export class IMonitorProcessManager
	{
	public:
		/// <summary>
		/// Gets the current active process manager
		/// </summary>
		static IMonitorProcessManager& Current()
		{
			if (_current == nullptr)
				throw std::runtime_error("No monitor process manager implementation registered.");
			return *_current;
		}

		/// <summary>
		/// Register a new active process manager
		/// </summary>
		static void Register(std::shared_ptr<IMonitorProcessManager> value)
		{
			_current = std::move(value);
		}

	public:
		/// <summary>
		/// Creates a process for the provided executable path
		/// </summary>
		virtual std::shared_ptr<Opal::System::IProcess> CreateMonitorProcess(
			const Path& executable,
			std::vector<std::string> arguments,
			const Path& workingDirectory,
			const std::map<std::string, std::string>& environmentVariables,
			std::shared_ptr<ISystemAccessMonitor> monitor,
			bool enableAccessChecks,
			bool partialMonitor,
			std::vector<Path> allowedReadAccess,
			std::vector<Path> allowedWriteAccess) = 0;

	private:
		static std::shared_ptr<IMonitorProcessManager> _current;
	};

#ifdef MONITOR_IMPLEMENTATION
	std::shared_ptr<IMonitorProcessManager> IMonitorProcessManager::_current = nullptr;
#endif
}
