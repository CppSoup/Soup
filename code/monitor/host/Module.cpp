module;

#ifdef _WIN32
#include <windows.h>

#ifdef CreateProcess
#undef CreateProcess
#endif

#pragma warning(push)
#if _MSC_VER > 1400
#pragma warning(disable:6102 6103)
#endif
#include <strsafe.h>
#pragma warning(pop)

#elif defined(__linux__)

#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>

#include <elf.h>

#include <seccomp.h>

// The existing environment for this process
extern char **environ;

#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <atomic>
#include <array>
#include <codecvt>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

export module Monitor.Host;
#if defined(_WIN32)
import Detours;
#endif
import Monitor.Shared;
import Opal;

using namespace Opal;

#define MONITOR_IMPLEMENTATION

#include "mock/MockMonitorProcessManager.h"
#include "ScopedMonitorProcessManagerRegister.h"

#if defined(_WIN32)
#include "Windows/WindowsMonitorProcessManager.h"
#include "Windows/WindowsSystemLoggerMonitor.h"
#include "Windows/WindowsSystemMonitorFork.h"
#elif defined(__linux__)
#include "linux/LinuxMonitorProcessManager.h"
#endif
