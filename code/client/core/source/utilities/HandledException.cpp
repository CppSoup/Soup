// <copyright file="HandledException.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <exception>

export module Soup.Core:HandledException;

namespace Soup::Core
{
	/// <summary>
	/// A special exception overload that indicates an early exit for the application that was handled
	/// </summary>
	export class HandledException : public std::exception
	{
	public:
		/// <summary>
		/// Initialize a new instance of the HandledException class
		/// </summary>
		HandledException(int exitCode) :
			std::exception(),
			m_exitCode(exitCode)
		{
		}

		HandledException(const HandledException& other) :
			std::exception(),
			m_exitCode(other.m_exitCode)
		{
		}

		virtual ~HandledException() noexcept
		{
		}

		int GetExitCode() const
		{
			return m_exitCode;
		}

	private:
		int m_exitCode;
	};
}
