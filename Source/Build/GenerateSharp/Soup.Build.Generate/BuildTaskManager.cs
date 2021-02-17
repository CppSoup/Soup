﻿// <copyright file="BuildSystem.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

using System;
using System.Collections.Generic;
using System.Text;

namespace Soup.Build.Generate
{
	internal class BuildTaskContainer
	{
		public BuildTaskContainer(
			string name,
			IBuildTask task,
			IList<string> runBeforeList,
			IList<string> runAfterList)
		{
			Name = name;
			Task = task;
			RunBeforeList = runBeforeList;
			RunAfterList = runAfterList;
			RunAfterClosureList = new List<string>();
			HasRun = false;
		}

		public string Name { get; init; }
		public IBuildTask Task { get; init; }
		public IList<string> RunBeforeList { get; init; }
		public IList<string> RunAfterList { get; init; }
		public List<string> RunAfterClosureList { get; init; }
		public bool HasRun { get; set; }
	}

	/// <summary>
	/// The build system implementation
	/// </summary>
	internal class BuildTaskManager
	{
		/// <summary>
		/// Initializes a new instance of the <see cref="BuildTaskManager"/> class.
		/// </summary>
		public BuildTaskManager()
		{
			_tasks = new Dictionary<string, BuildTaskContainer>();
		}

		/// <summary>
		/// Register task
		/// </summary>
		public void RegisterTask(
			string name,
			IBuildTask task,
			IList<string> runBeforeList,
			IList<string> runAfterList)
		{
			Log.Diag("RegisterTask: " + name);

			var taskContainer = new BuildTaskContainer(
				name,
				task,
				runBeforeList,
				runAfterList);

			var runBeforeMessage = new StringBuilder();
			runBeforeMessage.Append("RunBefore [");
			bool isFirst = true;
			foreach (var value in taskContainer.RunBeforeList)
			{
				if (!isFirst)
					runBeforeMessage.Append(", ");

				runBeforeMessage.Append("\"");
				runBeforeMessage.Append(value);
				runBeforeMessage.Append("\"");
				isFirst = false;
			}

			runBeforeMessage.Append("]");
			Log.Diag(runBeforeMessage.ToString());

			var runAfterMessage = new StringBuilder();
			runAfterMessage.Append("RunAfter [");
			isFirst = true;
			foreach (var value in taskContainer.RunAfterList)
			{
				if (!isFirst)
					runBeforeMessage.Append(", ");

				runBeforeMessage.Append("\"");
				runBeforeMessage.Append(value);
				runBeforeMessage.Append("\"");
				isFirst = false;
			}

			runAfterMessage.Append("]");
			Log.Diag(runAfterMessage.ToString());

			var insertResult = _tasks.TryAdd(name, taskContainer);
			if (!insertResult)
			{
				Log.HighPriority("A task with the provided name has already been registered: " + name);
				throw new InvalidOperationException("A task with the provided name has already been registered");
			}
		}

		/// <summary>
		/// Get the set of added include paths
		/// </summary>
		public void Execute(BuildState state)
		{
			// Setup each task to have a complete list of tasks that must run before itself
			// Note: this is required to combine other tasks run before lists with the tasks
			// own run after list
			foreach (var task in _tasks.Values)
			{
				// Copy their own run after list
				task.RunAfterClosureList.AddRange(task.RunAfterList);

				// Add ourself to all tasks in our run before list
				foreach (var runBefore in task.RunBeforeList)
				{
					// Try to find the other task
					if (_tasks.TryGetValue(runBefore, out var beforeTaskContainer))
					{
						beforeTaskContainer.RunAfterClosureList.Add(task.Name);
					}
				}
			}

			// Run all tasks in the order they were registered
			// ensuring they are run in the correct dependency order
			while (TryFindNextTask(out var currentTask))
			{
				if (ReferenceEquals(currentTask, null))
					throw new InvalidOperationException();

				Log.Info("TaskStart: " + currentTask.Name);
				currentTask.Task.Execute();
				Log.Info("TaskDone: " + currentTask.Name);

				// TODO : state.LogActive();
				currentTask.HasRun = true;
			}
		}

		/// <summary>
		/// Try to find the next task that has yet to be run and is ready
		/// Returns false if all tasks have been run
		/// Throws error if we hit a deadlock
		/// </summary>
		private bool TryFindNextTask(out BuildTaskContainer? task)
		{
			// Find the next task that is ready to be run
			bool hasAnyStillWaiting = false;
			foreach (var activeTask in _tasks)
			{
				// Check if this task has run already, 
				// if not check if all if all upstream tasks have finished
				var taskContainer = activeTask.Value;
				if (!taskContainer.HasRun)
				{
					hasAnyStillWaiting = true;

					// Check if all of their run after dependencies have already finished
					bool hasDependencyPending = false;
					foreach (var runBefore in taskContainer.RunAfterClosureList)
					{
						if (_tasks.TryGetValue(runBefore, out var findResult) && !findResult.HasRun)
						{
							// Found a dependency that hasn't run, keep trying
							hasDependencyPending = true;
							break;
						}
					}

					// All dependencies have finished
					// Let's run this one
					if (!hasDependencyPending)
					{
						task = taskContainer;
						return true;
					}
				}
			}

			if (hasAnyStillWaiting)
				throw new InvalidOperationException("Hit deadlock in build task dependencies.");

			task = null;
			return false;
		}

		private IDictionary<string, BuildTaskContainer> _tasks;
	};
}
