// <copyright file="BuildEvaluateEngine.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "IEvaluateEngine.h"
#include "BuildHistoryChecker.h"
#include "operation-graph/OperationGraph.h"
#include "SystemAccessTracker.h"

namespace Soup::Core
{
	class BuildEvaluateState
	{
	public:
		BuildEvaluateState(
			const OperationGraph& operationGraph,
			OperationResults& operationResults,
			const Path& temporaryDirectory,
			const std::vector<Path>& globalAllowedReadAccess,
			const std::vector<Path>& globalAllowedWriteAccess) :
			OperationGraph(operationGraph),
			OperationResults(operationResults),
			TemporaryDirectory(temporaryDirectory),
			GlobalAllowedReadAccess(globalAllowedReadAccess),
			GlobalAllowedWriteAccess(globalAllowedWriteAccess),
			RemainingDependencyCounts(),
			LookupLoaded(false),
			InputFileLookup(),
			OutputFileLookup()
		{
		}

		const ::Soup::Core::OperationGraph& OperationGraph;
		::Soup::Core::OperationResults& OperationResults;

		const Path& TemporaryDirectory;

		const std::vector<Path>& GlobalAllowedReadAccess;
		const std::vector<Path>& GlobalAllowedWriteAccess;

		// Running State
		std::unordered_map<OperationId, int32_t> RemainingDependencyCounts;

		bool LookupLoaded;
		std::unordered_map<FileId, std::set<OperationId>> InputFileLookup;
		std::unordered_map<FileId, OperationId> OutputFileLookup;

		void EnsureOperationLookupLoaded()
		{
			if (LookupLoaded)
				return;

			for (auto& operation : OperationGraph.GetOperations())
			{
				auto& operationInfo = operation.second;

				for (auto fileId : operationInfo.DeclaredInput)
				{
					auto findResult = InputFileLookup.find(fileId);
					if (findResult != InputFileLookup.end())
					{
						findResult->second.insert(operationInfo.Id);
					}
					else
					{
						auto [insertIterator, wasInserted] = InputFileLookup.emplace(
							fileId,
							std::set<OperationId>());
						insertIterator->second.insert(operationInfo.Id);
					}
				}

				for (auto fileId : operationInfo.DeclaredOutput)
				{
					OutputFileLookup.emplace(fileId, operationInfo.Id);
				}
			}

			LookupLoaded = true;
		}

		bool TryGetInputFileOperations(
			FileId fileId,
			const std::set<OperationId>*& result)
		{
			auto findResult = InputFileLookup.find(fileId);
			if (findResult != InputFileLookup.end())
			{
				result = &findResult->second;
				return true;
			}
			else
			{
				return false;
			}
		}

		bool TryGetOutputFileOperation(
			FileId fileId,
			OperationId& result)
		{
			auto findResult = OutputFileLookup.find(fileId);
			if (findResult != OutputFileLookup.end())
			{
				result = findResult->second;
				return true;
			}
			else
			{
				return false;
			}
		}
	};

	/// <summary>
	/// The core build evaluation engine that knows how to perform a build from a provided Operation Graph.
	/// </summary>
	export class BuildEvaluateEngine : public IEvaluateEngine
	{
	private:
		bool _forceRebuild;
		bool _disableMonitor;
		bool _partialMonitor;

		// Shared Runtime State
		FileSystemState& _fileSystemState;
		BuildHistoryChecker _stateChecker;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildEvaluateEngine"/> class.
		/// </summary>
		BuildEvaluateEngine(
			bool forceRebuild,
			bool disableMonitor,
			bool partialMonitor,
			FileSystemState& fileSystemState) :
			_forceRebuild(forceRebuild),
			_disableMonitor(disableMonitor),
			_partialMonitor(partialMonitor),
			_fileSystemState(fileSystemState),
			_stateChecker(fileSystemState)
		{
		}

		/// <summary>
		/// Execute the entire operation graph that is referenced by this build evaluate engine
		/// </summary>
		bool Evaluate(
			const OperationGraph& operationGraph,
			OperationResults& operationResults,
			const Path& temporaryDirectory,
			const std::vector<Path>& globalAllowedReadAccess,
			const std::vector<Path>& globalAllowedWriteAccess) override
		{
			// Run all build operations in the correct order with incremental build checks
			Log::Diag("Build evaluation start");
			auto evaluateState = BuildEvaluateState(
				operationGraph,
				operationResults,
				temporaryDirectory,
				globalAllowedReadAccess,
				globalAllowedWriteAccess);

			auto result = CheckExecuteOperations(
				evaluateState,
				operationGraph.GetRootOperationIds());
			Log::Diag("Build evaluation end");

			return result;
		}

	private:
		/// <summary>
		/// Execute the collection of build operations
		/// </summary>
		bool CheckExecuteOperations(
			BuildEvaluateState& evaluateState,
			const std::vector<OperationId>& operations)
		{
			bool didAnyEvaluate = false;
			for (auto operationId : operations)
			{
				// Check if the operation was already a child from a different path
				// Only run the operation when all of its dependencies have completed
				auto& operationInfo = evaluateState.OperationGraph.GetOperationInfo(operationId);
				auto currentOperationSearch = evaluateState.RemainingDependencyCounts.find(operationId);
				int32_t remainingCount = -1;
				if (currentOperationSearch != evaluateState.RemainingDependencyCounts.end())
				{
					remainingCount = --currentOperationSearch->second;
				}
				else
				{
					// Get the cached total count and store the active count in the lookup
					remainingCount = operationInfo.DependencyCount - 1;
					auto insertResult = evaluateState.RemainingDependencyCounts.emplace(operationId, remainingCount);
					if (!insertResult.second)
						throw std::runtime_error("The operation id already existed in the remaining count lookup");
				}

				if (remainingCount == 0)
				{
					// Run the single operation
					didAnyEvaluate |= CheckExecuteOperation(
						evaluateState,
						operationInfo);
					
					// Recursively build all of the operation children
					didAnyEvaluate |= CheckExecuteOperations(
						evaluateState,
						operationInfo.Children);
				}
				else if (remainingCount < 0)
				{
					throw std::runtime_error("Remaining dependency count less than zero");
				}
				else
				{
					// This operation will be executed from a different path
				}
			}

			return didAnyEvaluate;
		}

		/// <summary>
		/// Check if an individual operation has been run and execute if required
		/// </summary>
		bool CheckExecuteOperation(
			BuildEvaluateState& evaluateState,
			const OperationInfo& operationInfo)
		{
			// Check if each source file is out of date and requires a rebuild
			Log::Diag("Check for previous operation invocation");

			// Check if this operation was run before
			auto buildRequired = false;
			OperationResult* previousResult;
			if (evaluateState.OperationResults.TryFindResult(operationInfo.Id, previousResult) &&
				previousResult->WasSuccessfulRun)
			{
				// Check if the executable has changed since the last run
				bool executableOutOfDate = false;
				if (operationInfo.Command.Executable != Path("./writefile.exe"))
				{
					// Only check for "real" executables
					auto executableFileId = _fileSystemState.ToFileId(
						operationInfo.Command.Executable,
						operationInfo.Command.WorkingDirectory);
					if (_stateChecker.IsOutdated(previousResult->EvaluateTime, executableFileId))
					{
						Log::Diag("Executable out of date");
						executableOutOfDate = true;
					}
				}

				// Perform the incremental build checks
				if (executableOutOfDate ||
					_stateChecker.IsOutdated(previousResult->ObservedOutput, previousResult->ObservedInput))
				{
					buildRequired = true;
				}
				else
				{
					if (_forceRebuild)
					{
						Log::HighPriority("Up to date: Force Build");
						buildRequired = true;
					}
					else
					{
						Log::Info("Up to date");
					}
				}
			}
			else
			{
				// The build command has not been run before
				Log::Info("Operation has no successful previous invocation");
				buildRequired = true;
			}

			if (buildRequired)
			{
				Log::HighPriority(operationInfo.Title);
				auto messageBuilder = std::stringstream();
				messageBuilder << "Execute: [" << operationInfo.Command.WorkingDirectory.ToString() << "] ";
				messageBuilder << operationInfo.Command.Executable.ToString();
				for (auto& argument : operationInfo.Command.Arguments)
					messageBuilder << " " << argument;

				Log::Diag(messageBuilder.str());

				auto operationResult = OperationResult();

				// Check for special in-process write operations
				if (operationInfo.Command.Executable == Path("./writefile.exe"))
				{
					ExecuteWriteFileOperation(
						operationInfo,
						operationResult);
				}
				else
				{
					ExecuteOperation(
						evaluateState.TemporaryDirectory,
						evaluateState.GlobalAllowedReadAccess,
						evaluateState.GlobalAllowedWriteAccess,
						operationInfo,
						operationResult);
				}

				// Ensure there are no new dependencies
				VerifyObservedState(evaluateState, operationInfo, operationResult);

				evaluateState.OperationResults.AddOrUpdateOperationResult(
					operationInfo.Id,
					std::move(operationResult));
			}
			else
			{
				Log::Info(operationInfo.Title);
			}

			return buildRequired;
		}

		/// <summary>
		/// Execute a single build operation
		/// </summary>
		void ExecuteWriteFileOperation(
			const OperationInfo& operationInfo,
			OperationResult& operationResult)
		{
			Log::Info("Execute InProcess WriteFile");

			// Pull out the file path argument
			if (operationInfo.Command.Arguments.size() != 2)
			{
				Log::Error("WriteFile path argument malformed");
				throw BuildFailedException();
			}

			auto fileName = Path(operationInfo.Command.Arguments[0]);
			Log::Info("WritFile: {}", fileName.ToString());

			auto filePath = fileName.HasRoot() ? fileName : operationInfo.Command.WorkingDirectory + fileName;
			auto& content = operationInfo.Command.Arguments[1];

			// Open the file to write to
			auto file = System::IFileSystem::Current().OpenWrite(filePath, false);
			file->GetOutStream() << content;

			operationResult.ObservedInput = {};
			operationResult.ObservedOutput = {
				_fileSystemState.ToFileId(filePath, operationInfo.Command.WorkingDirectory),
			};

			// Mark this operation as successful to enable future incremental builds
			operationResult.WasSuccessfulRun = true;
			operationResult.EvaluateTime = System::ISystem::Current().GetCurrentTime();

			// Ensure the File System State is notified of any output files that have changed
			_fileSystemState.InvalidateFileWriteTimes(operationResult.ObservedOutput);
		}

		/// <summary>
		/// Execute a single build operation
		/// </summary>
		void ExecuteOperation(
			const Path& temporaryDirectory,
			const std::vector<Path>& globalAllowedReadAccess,
			const std::vector<Path>& globalAllowedWriteAccess,
			const OperationInfo& operationInfo,
			OperationResult& operationResult)
		{
			auto monitor = std::make_shared<SystemAccessTracker>();

			// Add the temp folder to the environment
			auto environment = std::map<std::string, std::string>();
			environment.emplace("TEMP", temporaryDirectory.ToString());
			environment.emplace("TMP", temporaryDirectory.ToString());

			// Allow access to the declared inputs/outputs
			bool enableAccessChecks = true;
			auto allowedReadAccess = std::vector<Path>();
			auto allowedWriteAccess = std::vector<Path>();
			if (enableAccessChecks)
			{
				allowedReadAccess = _fileSystemState.GetFilePaths(operationInfo.ReadAccess);
				allowedWriteAccess = _fileSystemState.GetFilePaths(operationInfo.WriteAccess);
			}

			// Allow access to the global overrides
			std::copy(globalAllowedReadAccess.begin(), globalAllowedReadAccess.end(), std::back_inserter(allowedReadAccess));
			std::copy(globalAllowedWriteAccess.begin(), globalAllowedWriteAccess.end(), std::back_inserter(allowedWriteAccess));

			Log::Diag("Allowed Read Access:");
			for (auto& file : allowedReadAccess)
				Log::Diag(file.ToString());
			Log::Diag("Allowed Write Access:");
			for (auto& file : allowedWriteAccess)
				Log::Diag(file.ToString());

			std::shared_ptr<System::IProcess> process = nullptr;
			if (_disableMonitor)
			{
				process = System::IProcessManager::Current().CreateProcess(
					operationInfo.Command.Executable,
					operationInfo.Command.Arguments,
					operationInfo.Command.WorkingDirectory,
					true);
			}
			else
			{
				process = Monitor::IMonitorProcessManager::Current().CreateMonitorProcess(
					operationInfo.Command.Executable,
					operationInfo.Command.Arguments,
					operationInfo.Command.WorkingDirectory,
					environment,
					monitor,
					enableAccessChecks,
					_partialMonitor,
					std::move(allowedReadAccess),
					std::move(allowedWriteAccess));
			}

			process->Start();
			process->WaitForExit();

			auto stdOut = process->GetStandardOutput();
			auto stdErr = process->GetStandardError();
			auto exitCode = process->GetExitCode();

			// Check the result of the monitor
			monitor->VerifyResult();

			if (!stdOut.empty())
			{
				// Upgrade output to a warning if the command fails
				if (exitCode != 0)
					Log::Warning(stdOut);
				else
					Log::Info(stdOut);
			}

			// If there was any error output then the build failed
			// TODO: Find warnings + errors
			if (!stdErr.empty())
			{
				Log::Error(stdErr);
			}

			if (exitCode == 0)
			{
				// Save off the build graph for future builds
				auto input = std::vector<Path>();
				for (auto& value : monitor->GetInput())
				{
					auto path = Path::Parse(value);
					#ifdef TRACE_FILE_SYSTEM_STATE
						Log::Diag("ObservedInput: {}", path.ToString());
					#endif
					input.push_back(std::move(path));
				}

				auto output = std::vector<Path>();
				for (auto& value : monitor->GetOutput())
				{
					auto path = Path::Parse(value);
					#ifdef TRACE_FILE_SYSTEM_STATE
						Log::Diag("ObservedOutput: {}", path.ToString());
					#endif
					output.push_back(std::move(path));
				}

				operationResult.ObservedInput = _fileSystemState.ToFileIds(
					input,
					operationInfo.Command.WorkingDirectory);
				operationResult.ObservedOutput = _fileSystemState.ToFileIds(
					output,
					operationInfo.Command.WorkingDirectory);

				// Mark this operation as successful to enable future incremental builds
				operationResult.WasSuccessfulRun = true;
				operationResult.EvaluateTime = System::ISystem::Current().GetCurrentTime();

				// Ensure the File System State is notified of any output files that have changed
				_fileSystemState.InvalidateFileWriteTimes(operationResult.ObservedOutput);
			}
			else
			{
				// Leave the previous state untouched and abandon the remaining operations
				Log::Error("Operation exited with non-success code: {}", exitCode);
				throw BuildFailedException();
			}
		}

		void VerifyObservedState(
			BuildEvaluateState& evaluateState,
			const OperationInfo& operationInfo,
			OperationResult& operationResult)
		{
			// TODO: Should generate NEW input/output lookup to check for entirely observed dependencies

			evaluateState.EnsureOperationLookupLoaded();

			// Verify new inputs
			for (auto fileId : operationResult.ObservedInput)
			{
				// Check if this input was generated from another operation
				OperationId matchedOutputOperationId;
				if (evaluateState.TryGetOutputFileOperation(fileId, matchedOutputOperationId) &&
					operationInfo.Id != matchedOutputOperationId)
				{
					// If it is a known output file then it must be a declared input for this operation
					const std::set<OperationId>* matchedInputOperationIds;
					if (!evaluateState.TryGetInputFileOperations(fileId, matchedInputOperationIds) ||
						!matchedInputOperationIds->contains(operationInfo.Id))
					{
						auto filePath = _fileSystemState.GetFilePath(fileId);
						auto& existingOperation = evaluateState.OperationGraph.GetOperationInfo(matchedOutputOperationId);
						auto message = std::format(
							"File \"{}\" observed as input for operation \"{}\" was written to by operation \"{}\" and must be declared as input",
							filePath.ToString(),
							operationInfo.Title,
							existingOperation.Title);
						throw std::runtime_error(message);
					}
				}
			}

			// Verify new outputs
			for (auto fileId : operationResult.ObservedOutput)
			{
				// Ensure the file is not also an output
				auto findObservedInput = std::find(operationResult.ObservedInput.begin(), operationResult.ObservedInput.end(), fileId);
				if (findObservedInput != operationResult.ObservedInput.end())
				{
					auto filePath = _fileSystemState.GetFilePath(fileId);
					Log::Warning("File \"{}\" observed as both input and output for operation \"{}\"", filePath.ToString(), operationInfo.Title);
					Log::Warning("Removing from input list for now. Will be treated as error in the future.");
					operationResult.ObservedInput.erase(findObservedInput);
				}

				// Ensure declared output is compatible
				OperationId matchedOutputOperationId;
				if (evaluateState.TryGetOutputFileOperation(fileId, matchedOutputOperationId))
				{
					if (matchedOutputOperationId != operationInfo.Id)
					{
						auto filePath = _fileSystemState.GetFilePath(fileId);
						auto& existingOperation = evaluateState.OperationGraph.GetOperationInfo(matchedOutputOperationId);
						auto message = std::format(
							"File \"{}\" observed as output for operation \"{}\" was already written by operation \"{}\"",
							filePath.ToString(),
							operationInfo.Title,
							existingOperation.Title);
						throw std::runtime_error(message);
					}
				}
				else
				{
					// Ensure new ouput does not create a dependency connection
					const std::set<OperationId>* matchedInputOperationIds;
					if (evaluateState.TryGetInputFileOperations(fileId, matchedInputOperationIds))
					{
						if (!matchedInputOperationIds->contains(operationInfo.Id))
						{
							auto filePath = _fileSystemState.GetFilePath(fileId);
							auto message = std::format(
								"File \"{}\" observed as output from operation \"{}\" creates new dependency to existing declared inputs",
								filePath.ToString(),
								operationInfo.Title);
							throw std::runtime_error(message);
						}
					}
				}
			}
		}
	};
}
