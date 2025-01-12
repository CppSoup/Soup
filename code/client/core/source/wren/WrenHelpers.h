// <copyright file="WrenHelpers.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

export namespace Soup::Core
{
	class SmartHandle
	{
	private:
		WrenVM* _vm;
		WrenHandle* _handle;

	public:
		SmartHandle(WrenVM* vm, WrenHandle* handle)
		{
			if (vm == nullptr)
				throw std::runtime_error("VM cannot be null");
			if (handle == nullptr)
				throw std::runtime_error("Handle cannot be null");

			_vm = vm;
			_handle = handle;
		}

		~SmartHandle()
		{
			wrenReleaseHandle(_vm, _handle);
			_handle = nullptr;
		}

		operator WrenHandle*() const
		{
			return _handle;
		}
	};

	class WrenHelpers
	{
	public:
		static void ThrowIfFailed(WrenInterpretResult result)
		{
			switch (result)
			{
				case WREN_RESULT_COMPILE_ERROR:
				{
					throw std::runtime_error("Compile Error!");
				}
				case WREN_RESULT_RUNTIME_ERROR:
				{
					throw std::runtime_error( "Runtime Error!");
				}
				case WREN_RESULT_SUCCESS:
				{
					break;
				}
			}
		}

		static void GenerateRuntimeError(WrenVM* vm, std::string_view message)
		{
			wrenEnsureSlots(vm, 1);
			wrenSetSlotString(vm, 0, message.data());
			wrenAbortFiber(vm, 0);
		}

		static bool HasParentType(WrenVM* vm, WrenHandle* classHandle, std::string_view parentClassType)
		{
			wrenSetSlotHandle(vm, 0, classHandle);

			// Create call handle for supertype and toString methods
			auto supertypeHandle = SmartHandle(vm, wrenMakeCallHandle(vm, "supertype"));
			auto toStringHandle = SmartHandle(vm, wrenMakeCallHandle(vm, "toString"));

			// Call the supertype method
			ThrowIfFailed(wrenCall(vm, supertypeHandle));

			// This returns the class reference for the supertype in slot 0.
			// Now call its toString method
			ThrowIfFailed(wrenCall(vm, toStringHandle));

			// Retrieve the return value (a string) from slot 0.
			auto parentType = std::string_view(wrenGetSlotString(vm, 0));

			return parentClassType == parentType;
		}

		static std::string GetType(WrenVM* vm, WrenHandle* classHandle)
		{
			wrenSetSlotHandle(vm, 0, classHandle);

			// Create call handle for type and toString methods
			auto typeHandle = SmartHandle(vm, wrenMakeCallHandle(vm, "type"));
			auto toStringHandle = SmartHandle(vm, wrenMakeCallHandle(vm, "toString"));

			// Call the type method
			ThrowIfFailed(wrenCall(vm, typeHandle));

			// This returns the class reference for the type in slot 0.
			// Now call its toString method
			ThrowIfFailed(wrenCall(vm, toStringHandle));

			// Retrieve the return value (a string) from slot 0.
			auto type = std::string(wrenGetSlotString(vm, 0));

			return type;
		}

		static std::string GetClassName(WrenVM* vm, WrenHandle* classHandle)
		{
			wrenSetSlotHandle(vm, 0, classHandle);

			// Create call handle for name method
			auto nameHandle = SmartHandle(vm, wrenMakeCallHandle(vm, "name"));

			// Call the name method
			ThrowIfFailed(wrenCall(vm, nameHandle));

			// Retrieve the return value (a string) from slot 0.
			auto type = std::string(wrenGetSlotString(vm, 0));

			return type;
		}

		static std::vector<std::string> GetSlotStringList(WrenVM* vm, int listSlot, int valueSlot)
		{
			auto responseType = wrenGetSlotType(vm, listSlot);
			if (responseType != WREN_TYPE_LIST) {
				throw std::runtime_error("Expected a list type");
			}

			auto listCount = wrenGetListCount(vm, listSlot);

			// Load each element into the second slot
			wrenEnsureSlots(vm, valueSlot + 1);
			auto result = std::vector<std::string>();
			for (auto i = 0; i < listCount; i++)
			{
				wrenGetListElement(vm, listSlot, i, valueSlot);
				
				auto elementType = wrenGetSlotType(vm, valueSlot);
				if (elementType != WREN_TYPE_STRING) {
					throw std::runtime_error("Element in list must be string");
				}

				auto element = std::string(wrenGetSlotString(vm, valueSlot));
				result.push_back(std::move(element));
			}

			return result;
		}
	};
}