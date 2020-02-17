﻿// <copyright file="IHttpClient.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "HttpResponse.h"

namespace Opal::Network
{
	/// <summary>
	/// The Http Client interface
	/// Interface mainly used to allow for unit testing client code
	/// </summary>
	export class IHttpClient
	{
	public:
		/// <summary>
		/// Perform an Http Get request
		/// </summary>
		virtual HttpResponse Get(std::string_view requests) = 0;
	};
}