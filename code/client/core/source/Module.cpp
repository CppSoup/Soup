module;

// TODO: Add a converter level to Opal?
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <any>
#include <array>
#include <chrono>
#include <codecvt>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <regex>
#include <optional>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#ifdef _WIN32

#include <format>

#include <Windows.h>
#undef max
#undef min
#undef CreateDirectory
#undef CreateProcess
#undef GetCurrentTime
#undef GetClassName

#elif defined(__linux__)

#include <spawn.h>
#include <sys/wait.h>

#endif

// TODO module
// TODO: Treat wren as C code
#include "wren/wren.h"

export module Soup.Core;

import reflex;
import CryptoPP;
import Monitor.Host;
import Opal;

// Build
export import :BuildConstants;
export import :BuildFailedException;
export import :DependencyTargetSet;
export import :FileSystemState;
export import :KnownLanguage;
export import :MacroManager;

// Operation Graph
export import :CommandInfo;
export import :OperationInfo;
export import :OperationResults;
export import :OperationResultsManager;
export import :OperationResultsReader;
export import :OperationResultsWriter;

// Utilities
export import :HandledException;
export import :SequenceMap;

// Value Table
export import :Value;
export import :ValueTableReader;
export import :ValueTableWriter;

using namespace Opal;

#define CLIENT_CORE_IMPLEMENTATION

#include "build/RecipeBuildLocationManager.h"
#include "build/BuildEngine.h"
#include "local-user-config/LocalUserConfigExtensions.h"
#include "package/PackageManager.h"
#include "wren/WrenHost.h"
#include "wren/WrenValueTable.h"