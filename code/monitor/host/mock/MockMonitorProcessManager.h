// <copyright file="MockMonitorProcessManager.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "../IMonitorProcessManager.h"

namespace Monitor
{
	/// <summary>
	/// The mock monitor process manager
	/// TODO: Move into test project
	/// </summary>
	export class MockMonitorProcessManager : public IMonitorProcessManager
	{
	private:
		std::atomic<int> m_uniqueId;
		std::vector<std::string> _requests;
		std::map<std::string, std::string> _executeResults;
		std::map<std::string, std::function<void(ISystemAccessMonitor&)>> _executeMonitors;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref='MockMonitorProcessManager'/> class.
		/// </summary>
		MockMonitorProcessManager() :
			m_uniqueId(1),
			_requests(),
			_executeResults()
		{
		}

		/// <summary>
		/// Create a result 
		/// </summary>
		void RegisterExecuteResult(
			std::string command,
			std::string output)
		{
			_executeResults.emplace(
				std::move(command),
				std::move(output));
		}
		
		/// <summary>
		/// Create a result 
		/// </summary>
		void RegisterExecuteCallback(
			std::string command,
			std::function<void(ISystemAccessMonitor&)> monitor)
		{
			_executeMonitors.emplace(
				std::move(command),
				std::move(monitor));
		}

		/// <summary>
		/// Get the load requests
		/// </summary>
		const std::vector<std::string>& GetRequests() const
		{
			return _requests;
		}

		/// <summary>
		/// Creates a process for the provided executable path
		/// </summary>
		std::shared_ptr<Opal::System::IProcess> CreateMonitorProcess(
			const Path& executable,
			std::vector<std::string> arguments,
			const Path& workingDirectory,
			const std::map<std::string, std::string>& environmentVariables,
			std::shared_ptr<ISystemAccessMonitor> monitor,
			bool enableAccessChecks,
			bool partialMonitor,
			std::vector<Path> allowedReadAccess,
			std::vector<Path> allowedWriteAccess) override final
		{
			std::stringstream message;
			auto id = m_uniqueId++;
			message << "CreateMonitorProcess: " << id << " [" << workingDirectory.ToString() << "] " << executable.ToString();
			for (auto& argument : arguments)
				message << " " << argument;
			message << " Environment [" << environmentVariables.size() << "]";
			message << " " << enableAccessChecks;
			message << " " << partialMonitor;
			message << " AllowedRead [" << allowedReadAccess.size() << "]";
			message << " AllowedWrite [" << allowedWriteAccess.size() << "]";

			_requests.push_back(message.str());

			// Check if there is a registered monitor
			auto findMonitor = _executeMonitors.find(message.str());
			if (findMonitor != _executeMonitors.end())
			{
				findMonitor->second(*monitor);
			}

			// Check if there is a registered output
			auto findOutput = _executeResults.find(message.str());
			if (findOutput != _executeResults.end())
			{
				return std::make_shared<Opal::System::MockProcess>(
					id,
					_requests,
					0,
					findOutput->second,
					std::string());
			}
			else
			{
				return std::make_shared<Opal::System::MockProcess>(
					id,
					_requests,
					0,
					std::string(),
					std::string());
			}
		}
	};
}
