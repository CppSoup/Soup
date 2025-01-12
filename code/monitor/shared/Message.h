#pragma once
#include "MessageType.h"

namespace Monitor
{
	export struct Message
	{
	public:
		MessageType Type;
		uint32_t ContentSize;
		uint8_t Content[2048 - sizeof(Type) - sizeof(ContentSize)];
	};
}