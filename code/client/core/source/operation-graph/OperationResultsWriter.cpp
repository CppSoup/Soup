// <copyright file="OperationResultsWriter.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <chrono>
#include <set>

export module Soup.Core:OperationResultsWriter;

import Opal;
import :FileSystemState;
import :OperationResults;

using namespace Opal;

namespace Soup::Core
{
	/// <summary>
	/// The operation results state writer
	/// </summary>
	export class OperationResultsWriter
	{
	private:
		// Binary Operation results file format
		static constexpr uint32_t FileVersion = 2;

		// The time duration that represents how we store the values in the file using 64 bit integer with resolution of 100 nanoseconds
		// Note: Unix Time, time since 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970, not counting leap seconds
		using ContentTimePeriod = std::ratio<1, 10'000'000>;
		using ContentDuration = std::chrono::duration<long long, ContentTimePeriod>;

	public:
		static void Serialize(
			const OperationResults& state,
			const std::set<FileId>& files,
			const FileSystemState& fileSystemState,
			std::ostream& stream)
		{
			// Write the File Header with version
			stream.write("BOR\0", 4);
			WriteValue(stream, FileVersion);

			// Write out the set of files
			stream.write("FIS\0", 4);
			WriteValue(stream, static_cast<uint32_t>(files.size()));
			for (auto fileId : files)
			{
				// Write the file id + path length + path
				WriteValue(stream, fileId);
				WriteValue(stream, fileSystemState.GetFilePath(fileId).ToString());
			}

			// Write out the set of results
			auto& results = state.GetResults();
			stream.write("RTS\0", 4);
			WriteValue(stream, static_cast<uint32_t>(results.size()));
			for (const auto& [key, value] : results)
			{
				WriteOperationResult(stream, key, value);
			}
		}

	private:
		static void WriteOperationResult(std::ostream& stream, OperationId operationId, const OperationResult& result)
		{
			// Write out the operation id
			WriteValue(stream, operationId);

			// Write out the value indicating if there was a successful run
			WriteValue(stream, result.WasSuccessfulRun);

			// Use system clock with a known epoch
			#ifdef _WIN32
			auto evaluateTimeSystem = std::chrono::clock_cast<std::chrono::system_clock>(result.EvaluateTime);
			#else
			auto evaluateTimeSystem = std::chrono::file_clock::to_sys(result.EvaluateTime);
			#endif

			// Write the tick offset of the system clock since its epoch
			auto evaluateTimeDuration = std::chrono::duration_cast<ContentDuration>(evaluateTimeSystem.time_since_epoch());
			int64_t evaluateTimeCount = evaluateTimeDuration.count();
			WriteValue(stream, evaluateTimeCount);

			// Write out the observed input files
			WriteValues(stream, result.ObservedInput);

			// Write out the observed output files
			WriteValues(stream, result.ObservedOutput);
		}

		static void WriteValue(std::ostream& stream, uint32_t value)
		{
			stream.write(reinterpret_cast<char*>(&value), sizeof(uint32_t));
		}

		static void WriteValue(std::ostream& stream, int64_t value)
		{
			stream.write(reinterpret_cast<char*>(&value), sizeof(int64_t));
		}

		static void WriteValue(std::ostream& stream, bool value)
		{
			uint32_t integerValue = value ? 1u : 0u;
			stream.write(reinterpret_cast<char*>(&integerValue), sizeof(uint32_t));
		}

		static void WriteValue(std::ostream& stream, std::string_view value)
		{
			WriteValue(stream, static_cast<uint32_t>(value.size()));
			stream.write(value.data(), value.size());
		}

		static void WriteValues(std::ostream& stream, const std::vector<uint32_t>& values)
		{
			WriteValue(stream, static_cast<uint32_t>(values.size()));
			for (auto& value : values)
			{
				WriteValue(stream, value);
			}
		}
	};
}