// <copyright file="Main.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

import Monitor.Host;
import Opal;
import Soup.Core;

using namespace Opal;

#include "Program.h"

int main(int argc, char** argv)
{
	std::vector<std::string> args;
	for (int i = 0; i < argc; i++)
	{
		args.push_back(argv[i]);
	}

	auto program = Soup::Client::Program();
	return program.Run(std::move(args));
}
