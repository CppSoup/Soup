﻿// <copyright file="BuildSystem.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup
{
	/// <summary>
	/// The build system implementation
	/// </summary>
	export class BuildSystem : public BuildEx::IBuildSystem
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildSystem"/> class.
		/// </summary>
		BuildSystem() :
			_tasks()
		{
		}

		/// <summary>
		/// Register task
		/// </summary>
		void RegisterTask(std::shared_ptr<BuildEx::IBuildTask> task) override final
		{
			Log::Diag(std::string("RegisterTask: ") + task->GetName());
			_tasks.push_back(std::move(task));
		}

		/// <summary>
		/// Get the set of added include paths
		/// </summary>
		void Execute()
		{
			for (auto& task : _tasks)
			{
				task->Execute();
			}
		}

	private:
		std::vector<std::shared_ptr<BuildEx::IBuildTask>> _tasks;
	};
}
