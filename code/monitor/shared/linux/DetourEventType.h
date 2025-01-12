#pragma once

namespace Monitor::Linux
{
	export enum class DetourEventType : uint32_t
	{
		// FileApi
		open,
		creat,
		openat,
		link,
		linkat,
		rename,
		unlink,
		remove,
		fopen,
		fdopen,
		freopen,
		mkdir,
		rmdir,

		// ProcessApi
		system,
		fork,
		vfork,
		clone,
		__clone2,
		clone3,
		execl,
		execlp,
		execle,
		execv,
		execvp,
		execvpe,
		execve,
		execveat,
		fexecve,
	};
}