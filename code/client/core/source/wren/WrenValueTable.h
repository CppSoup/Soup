// <copyright file="WrenValueTable.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once

export namespace Soup::Core
{
	class InvalidTypeException : public std::runtime_error
	{
	public:
		InvalidTypeException(const std::string& references) :
			std::runtime_error(std::move(references))
		{
		}
	};

	class WrenValueTable
	{
	public:
		static void SetSlotTable(WrenVM* vm, int slot, const ValueTable& table)
		{
			// Assume lower slots are in use
			int mapSlot = slot;
			int keySlot = slot + 1;
			int valueSlot = slot + 2;
			wrenEnsureSlots(vm, slot + 3);
			wrenSetSlotNewMap(vm, mapSlot);

			for (const auto& [key, value] : table)
			{
				wrenSetSlotString(vm, keySlot, key.c_str());
				SetSlotValue(vm, valueSlot, value);
				wrenSetMapValue(vm, mapSlot, keySlot, valueSlot);
			}
		}

		static void SetSlotList(WrenVM* vm, int slot, const ValueList& list)
		{
			// Assume lower slots are in use
			int listSlot = slot;
			int valueSlot = slot + 1;
			wrenEnsureSlots(vm, slot + 2);
			wrenSetSlotNewList(vm, listSlot);

			for (const auto& value : list)
			{
				SetSlotValue(vm, valueSlot, value);
				wrenInsertInList(vm, listSlot, -1, valueSlot);
			}
		}

		static void SetSlotValue(WrenVM* vm, int slot, const Value& value)
		{
			// Assume lower slots are in use
			wrenEnsureSlots(vm, slot + 1);
			
			switch (value.GetType())
			{
				case ValueType::Table:
					SetSlotTable(vm, slot, value.AsTable());
					break;
				case ValueType::List:
					SetSlotList(vm, slot, value.AsList());
					break;
				case ValueType::String:
					wrenSetSlotString(vm, slot, value.AsString().c_str());
					break;
				case ValueType::Integer:
					// Wren does not have a true integer type
					// TODO: Warn of too large errors.
					wrenSetSlotDouble(vm, slot, static_cast<double>(value.AsInteger()));
					break;
				case ValueType::Float:
					wrenSetSlotDouble(vm, slot, value.AsFloat());
					break;
				case ValueType::Boolean:
					wrenSetSlotBool(vm, slot, value.AsBoolean());
					break;
				case ValueType::Version:
					wrenSetSlotString(vm, slot, value.AsVersion().ToString().c_str());
					break;
				case ValueType::PackageReference:
					wrenSetSlotString(vm, slot, value.AsPackageReference().ToString().c_str());
					break;
				case ValueType::LanguageReference:
					wrenSetSlotString(vm, slot, value.AsLanguageReference().ToString().c_str());
					break;
				default:
					throw std::runtime_error("Unkown ValueType.");
			}
		}

		static ValueTable GetSlotTable(WrenVM* vm, int slot)
		{
			int mapSlot = slot;
			int keySlot = slot + 1;
			int valueSlot = slot + 2;

			auto mapType = wrenGetSlotType(vm, mapSlot);
			if (mapType != WREN_TYPE_MAP) {
				throw std::runtime_error("Type must be a map");
			}

			wrenEnsureSlots(vm, slot + 3);
			auto mapCount = wrenGetMapCount(vm, mapSlot);

			auto result = ValueTable();
			for (auto i = 0; i < mapCount; i++)
			{
				wrenGetMapKeyValueAt(vm, mapSlot, i, keySlot, valueSlot);

				auto keyType = wrenGetSlotType(vm, keySlot);
				if (keyType != WREN_TYPE_STRING) {
					auto stringBuilder = std::stringstream();
					stringBuilder << "KEY[" << i << "]";
					throw InvalidTypeException(stringBuilder.str());
				}

				auto key = wrenGetSlotString(vm, keySlot);
				try
				{
					auto value = GetSlotValue(vm, valueSlot);
					result.emplace(key, std::move(value));
				}
				catch(const InvalidTypeException& exception)
				{
					// Unwrap the type error
					auto stringBuilder = std::stringstream();
					stringBuilder << "[" << key << "]" << exception.what();
					throw InvalidTypeException(stringBuilder.str());
				}
			}

			return result;
		}

		static ValueList GetSlotList(WrenVM* vm, int slot)
		{
			int listSlot = slot;
			int valueSlot = slot + 1;

			auto listType = wrenGetSlotType(vm, listSlot);
			if (listType != WREN_TYPE_LIST) {
				throw std::runtime_error("Type must be a list");
			}

			wrenEnsureSlots(vm, slot + 2);
			auto listCount = wrenGetListCount(vm, listSlot);

			auto result = ValueList();
			for (auto i = 0; i < listCount; i++)
			{
				try
				{
					wrenGetListElement(vm, listSlot, i, valueSlot);
					auto value = GetSlotValue(vm, valueSlot);
					result.push_back(std::move(value));
				}
				catch(const InvalidTypeException& exception)
				{
					// Unwrap the type error
					auto stringBuilder = std::stringstream();
					stringBuilder << "[" << i << "]" << exception.what();
					throw InvalidTypeException(stringBuilder.str());
				}
			}

			return result;
		}

		static Value GetSlotValue(WrenVM* vm, int slot)
		{
			auto type = wrenGetSlotType(vm, slot);
			switch (type)
			{
				case WREN_TYPE_BOOL:
					return Value(wrenGetSlotBool(vm, slot));
				case WREN_TYPE_NUM:
					return Value(wrenGetSlotDouble(vm, slot));
				case WREN_TYPE_FOREIGN:
					throw std::runtime_error("Value cannot be foreign.");
				case WREN_TYPE_LIST:
					return Value(GetSlotList(vm, slot));
				case WREN_TYPE_MAP:
					return Value(GetSlotTable(vm, slot));
				case WREN_TYPE_NULL:
					throw std::runtime_error("Value cannot be null.");
				case WREN_TYPE_STRING:
					return Value(std::string(wrenGetSlotString(vm, slot)));
				case WREN_TYPE_UNKNOWN:
				{
					// The object is of a type that isn't accessible by the C API.
					auto handle = SmartHandle(vm, wrenGetSlotHandle(vm, slot));
					auto typeName = WrenHelpers::GetType(vm, handle);
					auto stringBuilder = std::stringstream();
					stringBuilder << "NON-PRIMITIVE:" << typeName;
					throw InvalidTypeException(stringBuilder.str());
				}
				default:
					throw std::runtime_error("Unkown SlotType.");
			}
		}
	};
}