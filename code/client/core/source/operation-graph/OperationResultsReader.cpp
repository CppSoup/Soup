// <copyright file="OperationResultsReader.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <array>
#include <chrono>

export module Soup.Core:OperationResultsReader;

import Opal;
import :FileSystemState;
import :OperationResults;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The operation results state reader
	/// </summary>
	export class OperationResultsReader
	{
	private:
		// Binary Operation Results file format
		static constexpr uint32_t FileVersion = 2;

		// The time duration that represents how we store the values in the file using 64 bit integer with resolution of 100 nanoseconds
		// Note: Unix Time, time since 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970, not counting leap seconds
		using ContentTimePeriod = std::ratio<1, 10'000'000>;
		using ContentDuration = std::chrono::duration<long long, ContentTimePeriod>;

	public:
		static OperationResults Deserialize(std::istream& stream, FileSystemState& fileSystemState)
		{
			// Read the entire file for fastest read operation
			stream.seekg(0, std::ios_base::end);
			auto size = stream.tellg();
			stream.seekg(0, std::ios_base::beg);

			auto contentBuffer = std::vector<char>(size);
			stream.read(contentBuffer.data(), size);
			auto data = contentBuffer.data();
			size_t offset = 0;

			auto result = Deserialize(data, size, offset, fileSystemState);

			if (offset != contentBuffer.size())
			{
				throw std::runtime_error("Operation results file corrupted - Did not read the entire file");
			}

			return result;
		}

	private:
		static OperationResults Deserialize(
			char* data,
			size_t size,
			size_t& offset,
			FileSystemState& fileSystemState)
		{
			// Read the File Header with version
			auto headerBuffer = std::array<char, 4>();
			Read(data, size, offset, headerBuffer.data(), 4);
			if (headerBuffer[0] != 'B' ||
				headerBuffer[1] != 'O' ||
				headerBuffer[2] != 'R' ||
				headerBuffer[3] != '\0')
			{
				throw std::runtime_error("Invalid operation results file header");
			}

			auto fileVersion = ReadUInt32(data, size, offset);
			if (fileVersion != FileVersion)
			{
				throw std::runtime_error("Operation results file version does not match expected");
			}

			// Read the set of files
			Read(data, size, offset, headerBuffer.data(), 4);
			if (headerBuffer[0] != 'F' ||
				headerBuffer[1] != 'I' ||
				headerBuffer[2] != 'S' ||
				headerBuffer[3] != '\0')
			{
				throw std::runtime_error("Invalid operation results files header");
			}

			// Map up the incoming file ids to the active file system state ids
			auto fileCount = ReadUInt32(data, size, offset);
			auto activeFileIdMap = std::unordered_map<FileId, FileId>();
			for (auto i = 0u; i < fileCount; i++)
			{
				// Read the command working directory
				auto fileId = ReadUInt32(data, size, offset);

				auto fileString = ReadString(data, size, offset);
				auto file = Path(std::move(fileString));

				auto activeFileId = fileSystemState.ToFileId(file);
				auto insertMapResult = activeFileIdMap.emplace(fileId, activeFileId);
				if (!insertMapResult.second)
					throw std::runtime_error("Failed to insert file id lookup");
			}

			// Read the set of operations
			Read(data, size, offset, headerBuffer.data(), 4);
			if (headerBuffer[0] != 'R' ||
				headerBuffer[1] != 'T' ||
				headerBuffer[2] != 'S' ||
				headerBuffer[3] != '\0')
			{
				throw std::runtime_error("Invalid operation results results header");
			}

			auto resultCount = ReadUInt32(data, size, offset);
			auto results = OperationResults();
			for (auto i = 0u; i < resultCount; i++)
			{
				ReadOperationResult(data, size, offset, activeFileIdMap, results);
			}

			return results;
		}

		static void ReadOperationResult(
			char* data,
			size_t size,
			size_t& offset,
			const std::unordered_map<FileId, FileId>& activeFileIdMap,
			OperationResults& results)
		{
			// Read the operation id
			auto operationId = ReadUInt32(data, size, offset);

			// Read the value indicating if there was a successful run
			auto wasSuccessfulRun = ReadBoolean(data, size, offset);

			// Read the tick offset of the system clock since its epoch
			auto evaluateTimeTicks = ReadInt64(data, size, offset);
			auto evaluateTimeDuration = ContentDuration(evaluateTimeTicks);

			// Use system clock with a known epoch
			auto evaluateTimeSystem = std::chrono::time_point<std::chrono::system_clock>(evaluateTimeDuration);
			#ifdef _WIN32
			auto evaluateTimeFile = std::chrono::clock_cast<std::chrono::file_clock>(evaluateTimeSystem);
			#else
			auto evaluateTimeFile = std::chrono::file_clock::from_sys(evaluateTimeSystem);
			#endif

			// Read the observed input files
			auto observedInput = ReadFileIdList(data, size, offset, activeFileIdMap);

			// Read the observed output files
			auto observedOutput = ReadFileIdList(data, size, offset, activeFileIdMap);

			auto result = OperationResult(
				wasSuccessfulRun,
				evaluateTimeFile,
				std::move(observedInput),
				std::move(observedOutput));

			results.AddOrUpdateOperationResult(operationId, std::move(result));
		}

		static uint32_t ReadUInt32(char* data, size_t size, size_t& offset)
		{
			uint32_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(uint32_t));

			return result;
		}

		static int64_t ReadInt64(char* data, size_t size, size_t& offset)
		{
			int64_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(int64_t));

			return result;
		}

		static bool ReadBoolean(char* data, size_t size, size_t& offset)
		{
			uint32_t result = 0;
			Read(data, size, offset, reinterpret_cast<char*>(&result), sizeof(uint32_t));

			return result != 0;
		}

		static std::string ReadString(char* data, size_t size, size_t& offset)
		{
			auto stringLength = ReadUInt32(data, size, offset);
			auto result = std::string(stringLength, '\0');
			Read(data, size, offset, result.data(), stringLength);

			return result;
		}

		static std::vector<FileId> ReadFileIdList(
			char* data, size_t size, size_t& offset, const std::unordered_map<FileId, FileId>& activeFileIdMap)
		{
			auto listLength = ReadUInt32(data, size, offset);
			auto result = std::vector<FileId>(listLength);
			for (auto i = 0u; i < listLength; i++)
			{
				auto fileId = ReadUInt32(data, size, offset);

				// Find the active file id that maps to the cached file id
				auto findActiveFileId = activeFileIdMap.find(fileId);
				if (findActiveFileId == activeFileIdMap.end())
					throw std::runtime_error("Could not find file id in active map");

				result[i] = findActiveFileId->second;
			}

			return result;
		}

		static std::vector<OperationId> ReadOperationIdList(
			char* data, size_t size, size_t& offset)
		{
			auto listLength = ReadUInt32(data, size, offset);
			auto result = std::vector<OperationId>(listLength);
			for (auto i = 0u; i < listLength; i++)
			{
				result[i] = ReadUInt32(data, size, offset);
			}

			return result;
		}

		static void Read(char* data, size_t size, size_t& offset, char* buffer, size_t count)
		{
			if (offset + count > size)
				throw new std::runtime_error("Tried to read past end of data");
			memcpy(buffer, data + offset, count);
			offset += count;
		}
	};
}