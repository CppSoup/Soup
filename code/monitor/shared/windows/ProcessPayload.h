#pragma once

export namespace Monitor::Windows
{
	constexpr const char* TBLOG_PIPE_NAMEA = "\\\\.\\pipe\\monitor";
	constexpr const wchar_t* TBLOG_PIPE_NAMEW = L"\\\\.\\pipe\\monitor";
	#ifdef UNICODE
	constexpr const char* TBLOG_PIPE_NAME = TBLOG_PIPE_NAMEW;
	#else
	constexpr const char* TBLOG_PIPE_NAME = TBLOG_PIPE_NAMEA;
	#endif

	struct ProcessPayload
	{
		DWORD nParentProcessId;
		DWORD nTraceProcessId;
		DWORD nGeneology;
		DWORD rGeneology[64];
		WCHAR wzParents[256];
		CHAR zWorkingDirectory[256];
		DWORD EnableAccessChecks;
		DWORD cReadAccessDirectories;
		CHAR zReadAccessDirectories[4096];
		DWORD cWriteAccessDirectories;
		CHAR zWriteAccessDirectories[4096];
	};

	// Shared payload guid used to get/set the payload when creating the child process
	constexpr GUID ProcessPayloadResourceId = { 0xd8e2dc69, 0x3004, 0x453e, { 0x94, 0x15, 0x19, 0x0e, 0x79, 0xe8, 0x93, 0x52 } };
}