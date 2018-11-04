﻿// <copyright file="ICompiler.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

namespace Soup.Compiler
{
    using System.Threading.Tasks;

    /// <summary>
    /// The compiler interface definition
    /// </summary>
    public interface ICompiler
    {
        /// <summary>
        /// Gets the object file extension for the compiler
        /// </summary>
        string ObjectFileExtension { get; }

        /// <summary>
        /// Gets the module file extension for the compiler
        /// </summary>
        string ModuleFileExtension { get; }

        /// <summary>
        /// Compile
        /// </summary>
        Task CompileAsync(CompilerArguments args);

        /// <summary>
        /// Link Library
        /// </summary>
        Task LinkLibraryAsync(LinkerArguments args);

        /// <summary>
        /// Link Executable
        /// </summary>
        Task LinkExecutableAsync(LinkerArguments args);
    }
}
