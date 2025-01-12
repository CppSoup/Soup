// <copyright file="OperationResultsManager.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <chrono>
#include <memory>
#include <stdexcept>

export module Soup.Core:OperationResultsManager;

import Opal;
import :FileSystemState;
import :OperationResultsReader;
import :OperationResultsWriter;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The operation results manager
	/// </summary>
	export class OperationResultsManager
	{
	public:
		/// <summary>
		/// Load the operation state from the provided directory
		/// </summary>
		static bool TryLoadState(
			const Path& operationResultsFile,
			OperationResults& result,
			FileSystemState& fileSystemState)
		{
			// Open the file to read from
			std::shared_ptr<System::IInputFile> file;
			if (!System::IFileSystem::Current().TryOpenRead(operationResultsFile, true, file))
			{
				Log::Info("Operation results file does not exist");
				return false;
			}

			// Read the contents of the build state file
			try
			{
				result = OperationResultsReader::Deserialize(file->GetInStream(), fileSystemState);
				return true;
			}
			catch(std::runtime_error& ex)
			{
				Log::Error(ex.what());
				return false;
			}
			catch(...)
			{
				Log::Error("Failed to parse operation results");
				return false;
			}
		}

		/// <summary>
		/// Save the operation state for the provided directory
		/// </summary>
		static void SaveState(
			const Path& operationResultsFile,
			const OperationResults& state,
			const FileSystemState& fileSystemState)
		{
			// Open the file to write to
			auto file = System::IFileSystem::Current().OpenWrite(operationResultsFile, true);

			// Update the operation graph referenced files
			auto files = std::set<FileId>();
			for (auto& resultReference : state.GetResults())
			{
				auto& result = resultReference.second;
				files.insert(result.ObservedInput.begin(), result.ObservedInput.end());
				files.insert(result.ObservedOutput.begin(), result.ObservedOutput.end());
			}

			// Write the build state to the file stream
			OperationResultsWriter::Serialize(state, files, fileSystemState, file->GetOutStream());
		}
	};
}