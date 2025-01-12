// <copyright file="OperationGraph.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup::Core
{
	/// <summary>
	/// The operation graph that represents the set of operations that need to be evaluated to perform the build
	/// </summary>
	export class OperationGraph
	{
	private:
		std::vector<OperationId> _rootOperations;
		std::map<OperationId, OperationInfo> _operations;
		std::unordered_map<CommandInfo, OperationId> _operationLookup;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="OperationGraph"/> class.
		/// </summary>
		OperationGraph() :
			_rootOperations(),
			_operations(),
			_operationLookup()
		{
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="OperationGraph"/> class.
		/// </summary>
		OperationGraph(
			std::vector<OperationId> rootOperations,
			std::vector<OperationInfo> operations) :
			_rootOperations(std::move(rootOperations)),
			_operations(),
			_operationLookup()
		{
			// Store the incoming vector of operations as a lookup for fast checks
			for (auto& info : operations)
			{
				AddOperation(std::move(info));
			}
		}

		/// <summary>
		/// Get the list of root operation ids
		/// </summary>
		const std::vector<OperationId>& GetRootOperationIds() const
		{
			return _rootOperations;
		}

		/// <summary>
		/// Set the list of root operation ids
		/// </summary>
		void SetRootOperationIds(std::vector<OperationId> value)
		{
			_rootOperations = std::move(value);
		}

		/// <summary>
		/// Get Operations
		/// </summary>
		const std::map<OperationId, OperationInfo>& GetOperations() const
		{
			return _operations;
		}

		/// <summary>
		/// Get Operations
		/// </summary>
		std::map<OperationId, OperationInfo>& GetOperations()
		{
			return _operations;
		}

		/// <summary>
		/// Find an operation info
		/// </summary>
		bool HasCommand(const CommandInfo& command)
		{
			return _operationLookup.contains(command);
		}

		/// <summary>
		/// Find an operation info
		/// </summary>
		bool TryFindOperation(
			const CommandInfo& command,
			OperationId& operationId) const
		{
			auto findResult = _operationLookup.find(command);
			if (findResult != _operationLookup.end())
			{
				operationId = findResult->second;
				return true;
			}
			else
			{
				return false;
			}
		}

		/// <summary>
		/// Get an operation info
		/// </summary>
		OperationInfo& GetOperationInfo(OperationId operationId)
		{
			auto findResult = _operations.find(operationId);
			if (findResult != _operations.end())
			{
				return findResult->second;
			}
			else
			{
				throw std::runtime_error("The provided operation id does not exist");
			}
		}
		const OperationInfo& GetOperationInfo(OperationId operationId) const
		{
			auto findResult = _operations.find(operationId);
			if (findResult != _operations.end())
			{
				return findResult->second;
			}
			else
			{
				throw std::runtime_error("The provided operation id does not exist");
			}
		}

		/// <summary>
		/// Add an operation info
		/// </summary>
		OperationInfo& AddOperation(OperationInfo info)
		{
			auto insertLookupResult = _operationLookup.emplace(info.Command, info.Id);
			if (!insertLookupResult.second)
				throw std::runtime_error("The provided command already exists in the graph");

			auto [insertIterator, wasInserted] = _operations.emplace(info.Id, std::move(info));
			if (!wasInserted)
				throw std::runtime_error("The provided operation id already exists in the graph");

			return insertIterator->second;
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const OperationGraph& rhs) const
		{
			return _rootOperations == rhs._rootOperations &&
				_operations == rhs._operations;
		}

		/// <summary>
		/// Inequality operator
		/// </summary>
		bool operator !=(const OperationGraph& rhs) const
		{
			return !(*this == rhs);
		}
	};
}
