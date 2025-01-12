#pragma once 

namespace Monitor
{
	export class ISystemAccessMonitor
	{
	public:
		virtual void OnCreateProcess(std::string_view applicationName, bool wasDetoured) = 0;
		virtual void TouchFileRead(Path filePath, bool exists, bool wasBlocked) = 0;
		virtual void TouchFileWrite(Path filePath, bool wasBlocked) = 0;
		virtual void TouchFileDelete(Path filePath, bool wasBlocked) = 0;
		virtual void TouchFileDeleteOnClose(Path filePath) = 0;
		virtual void SearchPath(std::string_view path, std::string_view filename) = 0;
	};
}
