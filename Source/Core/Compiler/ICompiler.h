﻿// <copyright file="ICompiler.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "CompileArguments.h"
#include "LinkArguments.h"

namespace Soup
{
	/// <summary>
	/// The compiler interface definition
	/// </summary>
	export class ICompiler
	{
	public:
		/// <summary>
		/// Gets the unique name for the compiler
		/// </summary>
		virtual std::string_view GetName() const = 0;

		/// <summary>
		/// Gets the object file extension for the compiler
		/// </summary>
		virtual std::string_view GetObjectFileExtension() const = 0;

		/// <summary>
		/// Gets the module file extension for the compiler
		/// </summary>
		virtual std::string_view GetModuleFileExtension() const = 0;

		/// <summary>
		/// Gets the static library file extension for the compiler
		/// TODO: This is platform specific
		/// </summary>
		virtual std::string_view GetStaticLibraryFileExtension() const = 0;

		/// <summary>
		/// Gets the dynamic library file extension for the compiler
		/// TODO: This is platform specific
		/// </summary>
		virtual std::string_view GetDynamicLibraryFileExtension() const = 0;

		/// <summary>
		/// Compile
		/// </summary>
		virtual std::shared_ptr<Build::BuildGraphNode> CreateCompileNode(const CompileArguments& args) = 0;

		/// <summary>
		/// Link
		/// </summary>
		virtual std::shared_ptr<Build::BuildGraphNode> CreateLinkNode(const LinkArguments& args) = 0;
	};
}
