// <copyright file="OperationInfo.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <chrono>
#include <string>

export module Soup.Core:OperationInfo;

import Opal;
import :CommandInfo;
import :FileSystemState;

using namespace Opal;
using namespace std::chrono_literals;

export namespace Soup::Core
{
	using OperationId = uint32_t;

	/// <summary>
	/// A node in the build graph that is the smallest single operation that will be evaluated
	/// </summary>
	class OperationInfo
	{
	public:
		OperationId Id;
		std::string Title;
		CommandInfo Command;
		std::vector<FileId> DeclaredInput;
		std::vector<FileId> DeclaredOutput;
		std::vector<FileId> ReadAccess;
		std::vector<FileId> WriteAccess;
		std::vector<OperationId> Children;
		uint32_t DependencyCount;

	public:
		OperationInfo() :
			Id(0),
			Title(),
			Command(),
			DeclaredInput(),
			DeclaredOutput(),
			ReadAccess(),
			WriteAccess(),
			Children(),
			DependencyCount(0)
		{
		}

		OperationInfo(
			OperationId id,
			std::string title,
			CommandInfo command,
			std::vector<FileId> declaredInput,
			std::vector<FileId> declaredOutput,
			std::vector<FileId> readAccess,
			std::vector<FileId> writeAccess) :
			Id(id),
			Title(std::move(title)),
			Command(std::move(command)),
			DeclaredInput(std::move(declaredInput)),
			DeclaredOutput(std::move(declaredOutput)),
			ReadAccess(std::move(readAccess)),
			WriteAccess(std::move(writeAccess)),
			Children(),
			DependencyCount(0)
		{
		}

		OperationInfo(
			OperationId id,
			std::string title,
			CommandInfo command,
			std::vector<FileId> declaredInput,
			std::vector<FileId> declaredOutput,
			std::vector<FileId> readAccess,
			std::vector<FileId> writeAccess,
			std::vector<OperationId> children,
			uint32_t dependencyCount) :
			Id(id),
			Title(std::move(title)),
			Command(std::move(command)),
			DeclaredInput(std::move(declaredInput)),
			DeclaredOutput(std::move(declaredOutput)),
			ReadAccess(std::move(readAccess)),
			WriteAccess(std::move(writeAccess)),
			Children(std::move(children)),
			DependencyCount(dependencyCount)
		{
		}

		bool operator ==(const OperationInfo& rhs) const
		{
			return Id == rhs.Id &&
				Title == rhs.Title &&
				Command == rhs.Command &&
				DeclaredInput == rhs.DeclaredInput &&
				DeclaredOutput == rhs.DeclaredOutput &&
				ReadAccess == rhs.ReadAccess &&
				WriteAccess == rhs.WriteAccess &&
				Children == rhs.Children &&
				DependencyCount == rhs.DependencyCount;
		}
	};
}