// <copyright file="RecipeBuildGeneratorTests.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

namespace Soup::UnitTests
{
	class RecipeBuildGeneratorTests
	{
	public:
		[[Fact]]
		void Initialize_Success()
		{
			auto compiler = std::make_shared<Compiler::Mock::Compiler>();
			auto uut = RecipeBuildGenerator(compiler);
		}

		[[Fact]]
		void Initialize_NullCompilerThrows()
		{
			Assert::ThrowsRuntimeError([]() {
				auto uut = RecipeBuildGenerator(nullptr);
			});
		}

		[[Fact]]
		void EnsureExecutableBuilt()
		{
			// Register the test listener
			auto testListener = std::make_shared<TestTraceListener>();
			Log::RegisterListener(testListener);

			// Register the test file system
			auto fileSystem = std::make_shared<MockFileSystem>();
			IFileSystem::Register(fileSystem);

			// Register the mock Soup.Core dependency
			fileSystem->CreateMockFile(
				Path("D:/Repos/Soup/Source/Core/Recipe.json"),
				MockFileState(std::stringstream(R"({
					"name": "Soup.Core",
					"version": "1.2.3"
				})")));

			auto compiler = std::make_shared<Compiler::Mock::Compiler>();
			auto uut = RecipeBuildGenerator(compiler);

			auto workingDirectory = Path("Root");
			auto recipe = Recipe();
			uut.EnsureExecutableBuilt(workingDirectory, recipe);

			// Verify expected logs
			Assert::AreEqual(
				std::vector<std::string>({
					"VERB: Create Directory: Root/.soup/Build",
					"VERB: Generate Build File: Build.cpp",
					"VERB: Compiling Build Executable",
					"VERB: TargetName = Soup.RecipeBuild",
					"VERB: TargetType = Executable",
					"VERB: WorkingDirectory = Root/.soup/Build",
					"VERB: ObjectDirectory = out/obj/MockCompiler",
					"VERB: BinaryDirectory = out/bin/MockCompiler",
					"VERB: ModuleInterfaceSourceFile = ",
					"VERB: IsIncremental = true",
					"VERB: IncludeDirectories = ",
					"VERB: IncludeModules = D:/Repos/Soup/Source/Core/out/bin/MockCompiler/Soup.Core.mock.bmi",
					"VERB: PreprocessorDefinitions = ",
					"VERB: Task: CoreCompile",
					"VERB: Loading previous build state",
					"VERB: BuildState file does not exist",
					"VERB: No previous state found, full rebuild required",
					"VERB: Create Directory: out/obj/MockCompiler",
					"VERB: Create Directory: out/bin/MockCompiler",
					"VERB: Task: CompileSourceFiles",
					"VERB: Compiling source files",
					"VERB: Build.cpp",
					"VERB: Saving updated build state",
					"VERB: Create Directory: .soup",
					"VERB: Task: CoreLink",
					"VERB: Linking target",
					"VERB: out/bin/MockCompiler/Soup.RecipeBuild.exe",
				}),
				testListener->GetMessages(),
				"Verify log messages match expected.");

			// Verify expected file system requests
			Assert::AreEqual(
				std::vector<std::pair<std::string, FileSystemRequestType>>({
					std::make_pair("Root/.soup/Build", FileSystemRequestType::Exists),
					std::make_pair("Root/.soup/Build", FileSystemRequestType::CreateDirectory),
					std::make_pair("Root/.soup/Build/Build.cpp", FileSystemRequestType::OpenWrite),
					std::make_pair("D:/Repos/Soup/Source/Core/Recipe.json", FileSystemRequestType::Exists),
					std::make_pair("D:/Repos/Soup/Source/Core/Recipe.json", FileSystemRequestType::OpenRead),
					std::make_pair("Root/.soup/Build/.soup/BuildState.json", FileSystemRequestType::Exists),
					std::make_pair("Root/.soup/Build/out/obj/MockCompiler", FileSystemRequestType::Exists),
					std::make_pair("Root/.soup/Build/out/obj/MockCompiler", FileSystemRequestType::CreateDirectory),
					std::make_pair("Root/.soup/Build/out/bin/MockCompiler", FileSystemRequestType::Exists),
					std::make_pair("Root/.soup/Build/out/bin/MockCompiler", FileSystemRequestType::CreateDirectory),
					std::make_pair("Root/.soup/Build/.soup", FileSystemRequestType::Exists),
					std::make_pair("Root/.soup/Build/.soup", FileSystemRequestType::CreateDirectory),
					std::make_pair("Root/.soup/Build/.soup/BuildState.json", FileSystemRequestType::OpenWrite),
				}),
				fileSystem->GetRequests(),
				"Verify file system requests match expected.");

			// Verify the contents of the build file
			std::string expectedBuildFile = 
R"(import Soup.Core;

int main()
{
})";
			auto& mockBuildFile = fileSystem->GetMockFile(Path("Root/.soup/Build/Build.cpp"));
			Assert::AreEqual(expectedBuildFile, mockBuildFile.Contents->str(), "Verify build file contents.");
		}

		// TODO: Way more of this
	};
}
