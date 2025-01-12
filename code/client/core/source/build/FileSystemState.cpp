// <copyright file="FileSystemState.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <chrono>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

export module Soup.Core:FileSystemState;

import Opal;

using namespace Opal;

export namespace Soup::Core
{
	using FileId = uint32_t;

	struct string_hash
	{
		using hash_type = std::hash<std::string_view>;
		using is_transparent = void;
	
		std::size_t operator()(const char* str) const { return hash_type{}(str); }
		std::size_t operator()(std::string_view str) const { return hash_type{}(str); }
		std::size_t operator()(std::string const& str) const { return hash_type{}(str); }
	};

	struct DirectoryState
	{
		std::set<std::string> Files;
		std::unordered_map<std::string, DirectoryState, string_hash, std::equal_to<>> ChildDirectories;
	};

	/// <summary>
	/// The complete set of known files that tracking the active change state during execution
	/// </summary>
	class FileSystemState
	{
	private:
		// The maximum id that has been used for files
		// Used to ensure unique ids are generated across the entire system
		FileId _maxFileId;

		std::unordered_map<FileId, Path> _files;
		std::unordered_map<std::string, FileId> _fileLookup;
		
		std::unordered_map<std::string, DirectoryState, string_hash, std::equal_to<>> _directoryLookup;

		std::unordered_map<FileId, std::optional<std::chrono::time_point<std::chrono::file_clock>>> _writeCache;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="FileSystemState"/> class.
		/// </summary>
		FileSystemState() :
			_maxFileId(0),
			_files(),
			_fileLookup(),
			_directoryLookup(),
			_writeCache()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="FileSystemState"/> class.
		/// </summary>
		FileSystemState(
			FileId maxFileId,
			std::unordered_map<FileId, Path> files) :
			FileSystemState(maxFileId, std::move(files), {}, {})
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="FileSystemState"/> class.
		/// </summary>
		FileSystemState(
			FileId maxFileId,
			std::unordered_map<FileId, Path> files,
			std::unordered_map<std::string, DirectoryState, string_hash, std::equal_to<>> directoryLookup,
			std::unordered_map<FileId, std::optional<std::chrono::time_point<std::chrono::file_clock>>> writeCache) :
			_maxFileId(maxFileId),
			_files(std::move(files)),
			_fileLookup(),
			_directoryLookup(std::move(directoryLookup)),
			_writeCache(std::move(writeCache))
		{
			// Build up the reverse lookup for new files
			for (const auto& [key, value] : _files)
			{
				auto insertResult = _fileLookup.emplace(value.ToString(), key);
				if (!insertResult.second)
					throw std::runtime_error("The file was not unique in the provided set.");
			}
		}

		/// <summary>
		/// Get Files
		/// </summary>
		const std::unordered_map<FileId, Path>& GetFiles() const
		{
			return _files;
		}

		/// <summary>
		/// Get the max unique file id
		/// </summary>
		FileId GetMaxFileId() const
		{
			return _maxFileId;
		}

		/// <summary>
		/// Update the write times for the provided set of files
		/// </summary>
		void InvalidateFileWriteTimes(const std::vector<FileId>& files)
		{
			for (auto file : files)
			{
				InvalidateFileWriteTime(file);
			}
		}

		/// <summary>
		/// Find the write time for a given file id
		/// </summary>
		std::optional<std::chrono::time_point<std::chrono::file_clock>> GetLastWriteTime(FileId file)
		{
			auto findResult = _writeCache.find(file);
			if (findResult != _writeCache.end())
			{
				return findResult->second;
			}
			else
			{
				return CheckFileWriteTime(file);
			}
		}

		/// <summary>
		/// Convert a set of file paths to file ids
		/// </summary>
		std::vector<FileId> ToFileIds(const std::vector<Path>& files, const Path& workingDirectory)
		{
			auto result = std::vector<FileId>();
			for (auto& file : files)
			{
				result.push_back(ToFileId(file, workingDirectory));
			}

			return result;
		}

		/// <summary>
		/// Convert a file path to file id
		/// </summary>
		FileId ToFileId(const Path& file, const Path& workingDirectory)
		{
			auto& absolutePath = file.HasRoot() ? file : workingDirectory + file;
			return ToFileId(absolutePath);
		}

		FileId ToFileId(const Path& file)
		{
			if (!file.HasRoot())
				throw std::runtime_error("File paths must be absolute to resolve to an id");

			// Check if the file is already known
			FileId result;
			if (!TryFindFileId(file, result))
			{
				// Insert the new file
				result = ++_maxFileId;
				auto insertResult = _files.emplace(result, file);
				if (!insertResult.second)
					throw std::runtime_error("The provided file id already exists in the file system state");

				auto insertLookupResult = _fileLookup.emplace(file.ToString(), result);
				if (!insertLookupResult.second)
					throw std::runtime_error("The file was not unique even though we just failed to find it");
			}

			return result;
		}

		/// <summary>
		/// Find an file id
		/// </summary>
		bool TryFindFileId(const Path& file, FileId& fileId) const
		{
			auto findResult = _fileLookup.find(file.ToString());
			if (findResult != _fileLookup.end())
			{
				fileId = findResult->second;
				return true;
			}
			else
			{
				return false;
			}
		}

		/// <summary>
		/// Find a file path
		/// </summary>
		std::vector<Path> GetFilePaths(const std::vector<FileId>& fileIds) const
		{
			auto result = std::vector<Path>();
			for (auto& fileId : fileIds)
			{
				result.push_back(GetFilePath(fileId));
			}

			return result;
		}

		/// <summary>
		/// Find a file path
		/// </summary>
		const Path& GetFilePath(FileId fileId) const
		{
			auto findResult = _files.find(fileId);
			if (findResult != _files.end())
			{
				return findResult->second;
			}
			else
			{
				throw std::runtime_error("The provided file id does not exist in the files set.");
			}
		}

		void PreloadDirectory(const Path& directory, bool trackDirectories)
		{
			#ifdef TRACE_FILE_SYSTEM_STATE
			std::cout << "PreloadDirectory: " << directory.ToString() << std::endl;
			#endif

			FileId directoryId;
			if (!TryFindFileId(directory, directoryId))
			{
				directoryId = ToFileId(directory);
				
				// Add the requested file as null
				// This will be replaced if the file exists with the find all callback
				auto insertResult = _writeCache.insert_or_assign(directoryId, std::nullopt);

				std::function<void(const Path& file, std::chrono::time_point<std::chrono::file_clock>)> callback =
					[&](const Path& file, std::chrono::time_point<std::chrono::file_clock> lastWriteTime)
					{
						auto& absolutePath = file.HasRoot() ? file : directory + file;

						#ifdef TRACE_FILE_SYSTEM_STATE
						std::cout << "PreloadDirectory: File " << file.ToString() << std::endl;
						#endif

						// Recursively load child directories
						if (!file.IsEmpty() && !absolutePath.HasFileName())
						{
							PreloadDirectory(absolutePath, trackDirectories);
						}

						if (trackDirectories)
						{
							UpdateDirectoryLookup(absolutePath);
						}

						FileId fileId = ToFileId(absolutePath);
						auto insertResult = _writeCache.insert_or_assign(fileId, lastWriteTime);
					};

				// Load the write times for all files in the directory
				// This optimization assumes that most files in a directory are relevant to the build
				// and on windows it is a lot faster to iterate over the files instead of making individual calls
				if (!System::IFileSystem::Current().TryGetDirectoryFilesLastWriteTime(
					directory,
					callback))
				{
					Log::Info("Preload Directory Missing: {}", directory.ToString());
				}
			}
		}

		DirectoryState& GetDirectoryState(const Path& directory)
		{
			auto activeDirectory = GetDirectoryState(_directoryLookup, directory.GetRoot());
			const auto directories = directory.DecomposeDirectories();
			for (auto currentDirectory : directories)
			{
				activeDirectory = GetDirectoryState(activeDirectory->ChildDirectories, currentDirectory);
			}

			return *activeDirectory;
		}

	private:
		DirectoryState* GetDirectoryState(
			std::unordered_map<std::string, DirectoryState, string_hash, std::equal_to<>>& activeDirectory,
			const std::string_view name)
		{
			auto findResult = activeDirectory.find(name);
			if (findResult != activeDirectory.end())
			{
				return &findResult->second;
			}
			else
			{
				throw std::runtime_error(std::format("Missing directory state {}", name));
			}
		}

		DirectoryState* EnsureDirectoryExists(
			std::unordered_map<std::string, DirectoryState, string_hash, std::equal_to<>>& activeDirectory,
			const std::string_view name)
		{
			auto findResult = activeDirectory.find(name);
			if (findResult != activeDirectory.end())
			{
				return &findResult->second;
			}
			else
			{
				auto insertResult = activeDirectory.emplace(name, DirectoryState());
				return &insertResult.first->second;
			}
		}

		void UpdateDirectoryLookup(const Path& file)
		{
			auto activeDirectory = EnsureDirectoryExists(_directoryLookup, file.GetRoot());

			const auto directories = file.DecomposeDirectories();
			for (auto directory : directories)
			{
				activeDirectory = EnsureDirectoryExists(activeDirectory->ChildDirectories, directory);
			}

			if (file.HasFileName())
			{
				activeDirectory->Files.insert(
					std::string(file.GetFileName()));
			}
		}

		/// <summary>
		/// Invalidate the write time for the provided file
		/// </summary>
		void InvalidateFileWriteTime(FileId fileId)
		{
			_writeCache.erase(fileId);
		}

		std::string format(std::chrono::time_point<std::chrono::file_clock> time)
		{
		#ifdef _WIN32
			return std::format("{:%Y-%m-%d %H:%M:%S %z}", time);
		#else
			auto systemTime = std::chrono::file_clock::to_sys(time);
			auto timeT = std::chrono::system_clock::to_time_t(systemTime);

			std::stringstream ss;
			ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S %z");

			return ss.str();
		#endif
		}

		/// <summary>
		/// Update the write times for the provided file
		/// </summary>
		std::optional<std::chrono::time_point<std::chrono::file_clock>> CheckFileWriteTime(FileId fileId)
		{
			auto& filePath = GetFilePath(fileId);

			// The file does not exist in the cache
			// Load the actual value and save it for later
			std::optional<std::chrono::time_point<std::chrono::file_clock>> lastWriteTime = std::nullopt;
			std::chrono::time_point<std::chrono::file_clock> lastWriteTimeValue;
			if (System::IFileSystem::Current().TryGetLastWriteTime(filePath, lastWriteTimeValue))
			{
				lastWriteTime = lastWriteTimeValue;
			}

#ifdef TRACE_FILE_SYSTEM_STATE
			if (lastWriteTime.has_value())
				std::cout << "CheckFileWriteTime: " << filePath.ToString() << " " << format(lastWriteTime.value()) << std::endl;
			else
				std::cout << "CheckFileWriteTime: " << filePath.ToString() << " NONE" << std::endl;
#endif

			auto insertResult = _writeCache.insert_or_assign(fileId, lastWriteTime);
			return lastWriteTime;
		}
	};
}
