#pragma once

namespace Monitor
{
	export enum class MessageType : uint32_t
	{
		Initialize,
		Shutdown,
		Error,

		Detour,
	};
}