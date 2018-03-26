﻿// <copyright company="Soup" file="Program.cs">
//   Copyright (c) Soup.  All rights reserved.
// </copyright>

namespace Soup.Client
{
	using System.Collections.Generic;
	using System.IO;
	using System.Threading.Tasks;
	using Soup.Api;

	/// <summary>
	/// The root of all evil
	/// </summary>
	public class Program
	{
		/// <summary>
		/// The collection of all known commands for the command line application
		/// </summary>
		private static IReadOnlyList<ICommand> _commands = new List<ICommand>()
		{
			new BuildCommand(),
			new GenerateCommand(),
			new InitializeCommand(),
			new InstallCommand(),
			new PackCommand(),
			new PublishCommand(),
			new VersionCommand(),
			new ViewCommand(),
		};

		// TODO : Convert over to using async main when C# 7.1 is available
		public static void Main(string[] args)
		{
			MainAsync(args).Wait();
		}

		/// <summary>
		/// The root of all evil - async style
		/// </summary>
		private static async Task MainAsync(string[] args)
		{
			// Load the user configuration settings
			var userConfig = new LocalUserConfig();
			var stagingDirectory = Path.Combine(userConfig.PackageStore, Constants.StagingFolderName);

			// Setup the singletons
			Singleton<ILogger>.Instance = new ConsoleLogger();
			Singleton<ISoupApi>.Instance = new SoupApi();
			Singleton<LocalUserConfig>.Instance = userConfig;

			// Ensure we are in a clean state
			if (Directory.Exists(stagingDirectory))
			{
				Log.Warning("The staging directory was not cleaned up!");
				Directory.Delete(stagingDirectory, true);
			}

			// Find the correct cammand to invoke
			bool foundCommand = false;
			if (args.Length > 0)
			{
				foreach (var command in _commands)
				{
					if (command.Name == args[0])
					{
						await command.InvokeAsync(args);
						foundCommand = true;
						break;
					}
				}
			}

			if (!foundCommand)
			{
				ShowUsage();
				return;
			}
		}

		/// <summary>
		/// Show the usage details for the general case
		/// </summary>
		private static void ShowUsage()
		{
			Log.Message("");
			Log.Message("Usage: soup <command>");
			Log.Message("");

			Log.Message("Available Commands:");
			foreach (var command in _commands)
			{
				Log.Message(string.Format("\t{0}", command.Name));
			}
		}
	}
}
