// <copyright file="OperationGraphManager.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "OperationGraph.h"
#include "OperationGraphReader.h"
#include "OperationGraphWriter.h"

namespace Soup::Core
{
	/// <summary>
	/// The operation state manager
	/// </summary>
	export class OperationGraphManager
	{
	public:
		/// <summary>
		/// Load the operation state from the provided directory
		/// </summary>
		static bool TryLoadState(
			const Path& operationGraphFile,
			OperationGraph& result,
			FileSystemState& fileSystemState)
		{
			// Open the file to read from
			std::shared_ptr<System::IInputFile> file;
			if (!System::IFileSystem::Current().TryOpenRead(operationGraphFile, true, file))
			{
				Log::Info("Operation graph file does not exist");
				return false;
			}

			// Read the contents of the build state file
			try
			{
				result = OperationGraphReader::Deserialize(file->GetInStream(), fileSystemState);
				return true;
			}
			catch(std::runtime_error& ex)
			{
				Log::Error(ex.what());
				return false;
			}
			catch(...)
			{
				Log::Error("Failed to parse operation graph");
				return false;
			}
		}

		/// <summary>
		/// Save the operation state for the provided directory
		/// </summary>
		static void SaveState(
			const Path& operationGraphFile,
			OperationGraph& state,
			const FileSystemState& fileSystemState)
		{
			auto targetFolder = operationGraphFile.GetParent();

			// Update the operation graph referenced files
			auto files = std::set<FileId>();
			for (auto& operationReference : state.GetOperations())
			{
				auto& operation = operationReference.second;
				files.insert(operation.DeclaredInput.begin(), operation.DeclaredInput.end());
				files.insert(operation.DeclaredOutput.begin(), operation.DeclaredOutput.end());
				files.insert(operation.ReadAccess.begin(), operation.ReadAccess.end());
				files.insert(operation.WriteAccess.begin(), operation.WriteAccess.end());
			}

			// Open the file to write to
			auto file = System::IFileSystem::Current().OpenWrite(operationGraphFile, true);

			// Write the build state to the file stream
			OperationGraphWriter::Serialize(state, files, fileSystemState, file->GetOutStream());
		}
	};
}
