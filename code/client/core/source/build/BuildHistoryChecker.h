// <copyright file="BuildHistoryChecker.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup::Core
{
	export class BuildHistoryChecker
	{
	private:
		FileSystemState& _fileSystemState;

	public:
		BuildHistoryChecker(FileSystemState& fileSystemState) :
			_fileSystemState(fileSystemState)
		{
		}

		/// <summary>
		/// Perform a check if the last evaluate time is outdated with
		/// respect to the input files
		/// </summary>
		bool IsOutdated(
			std::chrono::time_point<std::chrono::file_clock> lastEvaluateTime,
			FileId inputFile)
		{
			auto lastWriteTime = _fileSystemState.GetLastWriteTime(inputFile);

			// Perform the final check
			if (!lastWriteTime.has_value())
			{
				// The input was missing
				auto targetFilePath = _fileSystemState.GetFilePath(inputFile);
				Log::Info("Input Missing [{}]", targetFilePath.ToString());
				return true;
			}
			else
			{
				if (lastWriteTime.value() > lastEvaluateTime)
				{
					auto targetFilePath = _fileSystemState.GetFilePath(inputFile);
					Log::Info("Input altered after last evaluate [{}]", targetFilePath.ToString());
					return true;
				}
				else
				{
					return false;
				}
			}
		}

		/// <summary>
		/// Perform a check if the requested target is outdated with
		/// respect to the input files
		/// </summary>
		bool IsOutdated(
			const std::vector<FileId>& targetFiles,
			const std::vector<FileId>& inputFiles)
		{
			// If there are no input files then the output can never be outdated
			if (inputFiles.empty())
				return false;

			for (auto& targetFile : targetFiles)
			{
				if (IsOutdated(targetFile, inputFiles))
				{
					return true;
				}
			}

			return false;
		}

	private:
		/// <summary>
		/// Perform a check if the requested target is outdated with
		/// respect to the input files
		/// </summary>
		bool IsOutdated(
			FileId targetFile,
			const std::vector<FileId>& inputFiles)
		{
			// Get the output file last write time
			auto targetFileLastWriteTime = _fileSystemState.GetLastWriteTime(targetFile);

			if (!targetFileLastWriteTime.has_value())
			{
				auto targetFilePath = _fileSystemState.GetFilePath(targetFile);
				Log::Info("Output target does not exist: {}", targetFilePath.ToString());
				return true;
			}

			// Note: No need to use cache here since target files should only be analyzed once
			for (auto& inputFile : inputFiles)
			{
				// If the file is relative then combine it with the root path
				if (IsOutdated(inputFile, targetFile, targetFileLastWriteTime.value()))
				{
					return true;
				}
			}

			return false;
		}

		bool IsOutdated(
			FileId inputFile,
			FileId outputFile,
			std::chrono::time_point<std::chrono::file_clock> outputFileLastWriteTime)
		{
			// Get the file state from the cache
			auto lastWriteTime = _fileSystemState.GetLastWriteTime(inputFile);

			// Perform the final check
			if (!lastWriteTime.has_value())
			{
				// The input was missing
				auto targetFilePath = _fileSystemState.GetFilePath(inputFile);
				Log::Info("Input Missing [{}]", targetFilePath.ToString());
				return true;
			}
			else
			{
				if (lastWriteTime.value() > outputFileLastWriteTime)
				{
					auto targetFilePath = _fileSystemState.GetFilePath(inputFile);
					auto outputFilePath = _fileSystemState.GetFilePath(outputFile);
					Log::Info("Input altered after target [{}] -> [{}]", targetFilePath.ToString(), outputFilePath.ToString());
					return true;
				}
				else
				{
					return false;
				}
			}
		}
	};
}
