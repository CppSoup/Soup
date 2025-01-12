// <copyright file="WrenHost.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

#include "sml/SML.h"
#include "WrenHelpers.h"

export namespace Soup::Core
{
	class WrenHost
	{
	private:
		static inline const char BundleSeparator = ':';

	private:
		std::map<std::string, Path> _bundles;

	protected:
		Path _scriptFile;
		std::optional<Path> _bundlesFile;
		WrenVM* _vm;

	public:
		WrenHost(Path scriptFile, std::optional<Path> bundlesFile) :
			_scriptFile(std::move(scriptFile)),
			_bundlesFile(std::move(bundlesFile)),
			_bundles(),
			_vm(nullptr)
		{
			// Configure the Wren Virtual Machine
			WrenConfiguration config;
			wrenInitConfiguration(&config);
			config.resolveModuleFn = &WrenResolveModule;
			config.loadModuleFn = &WrenLoadModule;
			config.bindForeignMethodFn = &WrenBindForeignMethod;
			config.writeFn = &WrenWriteCallback;
			config.errorFn = &WrenErrorCallback;
			config.userData = this;

			// Create the VM
			_vm = wrenNewVM(&config);
		}

		virtual ~WrenHost()
		{
			wrenFreeVM(_vm);
			_vm = nullptr;
		}

		void InterpretMain()
		{
			// Load the bundles
			if (_bundlesFile.has_value())
			{
				std::ifstream bundlesFile(_bundlesFile.value().ToString());
				if (!bundlesFile.is_open())
					throw std::runtime_error("Bundles does not exist");

				auto bundlesDocument = SMLDocument::Parse(bundlesFile);
				if (!bundlesDocument.GetRoot().Contains("Bundles"))
				{
					throw std::runtime_error("Bundles file is missing Bundles element");
				}

				for (auto& [bundleName, bundle] : bundlesDocument.GetRoot()["Bundles"].AsTable().GetValue())
				{
					auto& bundleTable = bundle.AsTable();
					if (!bundleTable.Contains("Root"))
					{
						throw std::runtime_error("Bundle missing Root property");
					}

					_bundles.emplace(bundleName, Path(bundleTable["Root"].AsString()));
				}
			}

			// Load the script
			std::ifstream scriptFile(_scriptFile.ToString());
			if (!scriptFile.is_open())
				throw std::runtime_error(std::format("Script does not exist {0}", _scriptFile.ToString()));

			auto script = std::string(
				std::istreambuf_iterator<char>(scriptFile),
				std::istreambuf_iterator<char>());

			// Interpret the script
			WrenHelpers::ThrowIfFailed(wrenInterpret(_vm, _scriptFile.ToString().c_str(), script.c_str()));
		}

	private:
		virtual bool IsBuiltInModule(std::string_view moduleName) = 0;
		virtual bool TryGetBuiltInModule(std::string_view moduleName, const char*& source) = 0;
		virtual WrenForeignMethodFn TryBindForeignMethod(
			std::string_view moduleName,
			std::string_view className,
			bool isStatic,
			std::string_view signature) = 0;

		WrenForeignMethodFn BindForeignMethod(
			std::string_view moduleName,
			std::string_view className,
			bool isStatic,
			std::string_view signature)
		{
			WrenForeignMethodFn result = nullptr;

			// Attempt to bind to the built in Module methods
			result = TryBindForeignMethod(moduleName, className, isStatic, signature);
			if (result != nullptr)
				return result;

			return nullptr;
		}

		const char* ResolveModule(
			std::string_view importer,
			std::string_view moduleName)
		{
			// Logical import strings are used as-is and need no resolution.
			if (IsBuiltInModule(moduleName))
				return moduleName.data();

			// Check if it is a bundled reference
			Path currentDirectory;
			Path moduleReference;
			auto bundleSeparator = moduleName.find_first_of(BundleSeparator);
			if (bundleSeparator != std::string::npos)
			{
				if (!_bundlesFile.has_value())
				{
					// Nothing we can do about it
					return moduleName.data();
				}

				auto bundleName = moduleName.substr(0, bundleSeparator);
				auto findBundle = _bundles.find(std::string(bundleName));
				if (findBundle == _bundles.end())
				{
					// Nothing we can do about it
					return moduleName.data();
				}

				currentDirectory = findBundle->second;

				// If relative path in root then resolve to bundles file location
				if (!currentDirectory.HasRoot())
				{
					currentDirectory = _bundlesFile.value().GetParent() + currentDirectory;
				}

				moduleReference = Path(moduleName.substr(bundleSeparator + 1));
			}
			else
			{
				// Get the directory containing the importing module.
				auto modulePath = Path(importer);
				currentDirectory = modulePath.GetParent();

				moduleReference = Path(moduleName);
			}

			// Ensure the module is a relative path
			if (moduleReference.HasRoot())
				return moduleName.data();

			// Load the relative path from the current module folder
			auto resolvedModule = currentDirectory + moduleReference;
			
			// Automatically append wren file extension
			resolvedModule.SetFileExtension("wren");

			return ReturnRawString(resolvedModule.ToString());
		}

		WrenLoadModuleResult LoadModule(std::string_view moduleName)
		{
			WrenLoadModuleResult result = {0};
			result.onComplete = nullptr;

			// Inject Soup module
			const char* builtInSource = nullptr;
			if (TryGetBuiltInModule(moduleName, builtInSource))
			{
				result.source = builtInSource;
			}
			else
			{
				// Attempt to load the module as a script file
				std::ifstream scriptFile(moduleName.data());
				if (scriptFile.is_open())
				{
					auto script = std::string(
						std::istreambuf_iterator<char>(scriptFile),
						std::istreambuf_iterator<char>());
						
					result.onComplete = &FreeSourceOnLoaded;
					result.source = ReturnRawString(script);
				}
			}

			return result;
		}

		void WriteCallback(std::string_view text)
		{
			std::cout << text;
		}

		void ErrorCallback(
			WrenErrorType errorType,
			std::optional<std::string_view> moduleName,
			int line,
			std::string_view message)
		{
			switch (errorType)
			{
				case WREN_ERROR_COMPILE:
				{
					printf("[%s(%d)] [Error] %s\n", moduleName.value().data(), line, message.data());
					break;
				}
				case WREN_ERROR_STACK_TRACE:
				{
					printf("[%s(%d)] in %s\n", moduleName.value().data(), line, message.data());
					break;
				}
				case WREN_ERROR_RUNTIME:
				{
					printf("[Runtime Error] %s\n", message.data());
					break;
				}
			}
		}

		static const char* ReturnRawString(const std::string& string)
		{
			char* rawString = (char*)malloc(string.size() + 1);
			memcpy(rawString, string.c_str(), string.size());
			rawString[string.size()] = '\0';
			return rawString;
		}

		static void FreeSourceOnLoaded(WrenVM* /*vm*/, const char* /*name*/, struct WrenLoadModuleResult result)
		{
			if (result.source != nullptr)
				free((void*)result.source);
		}

		static WrenForeignMethodFn WrenBindForeignMethod(
			WrenVM* vm,
			const char* moduleName,
			const char* className,
			bool isStatic,
			const char* signature)
		{
			auto host = (WrenHost*)wrenGetUserData(vm);
			return host->BindForeignMethod(moduleName, className, isStatic, signature);
		}

		static const char* WrenResolveModule(
			WrenVM* vm,
			const char* importer,
			const char* moduleName)
		{
			auto host = (WrenHost*)wrenGetUserData(vm);
			return host->ResolveModule(importer, moduleName);
		}

		static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* moduleName)
		{
			auto host = (WrenHost*)wrenGetUserData(vm);
			return host->LoadModule(moduleName);
		}

		static void WrenWriteCallback(WrenVM* vm, const char* text)
		{
			auto host = (WrenHost*)wrenGetUserData(vm);
			host->WriteCallback(text);
		}

		static void WrenErrorCallback(
			WrenVM* vm,
			WrenErrorType errorType,
			const char* module,
			const int line,
			const char* msg)
		{
			auto host = (WrenHost*)wrenGetUserData(vm);
			std::optional<std::string_view> moduleName = std::nullopt;
			if (module != nullptr)
				moduleName = module;
			host->ErrorCallback(errorType, moduleName, line, msg);
		}
	};
}
