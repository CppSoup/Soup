// <copyright file="SequenceMap.cpp" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

module;

#include <stdexcept>
#include <vector>

export module Soup.Core:SequenceMap;

namespace Soup::Core
{
	/// <summary>
	/// A special map that is mutated as a vector
	/// </summary>
	export template<class TKey, class TValue>
	class SequenceMap
	{
	private:
		using raw_data = std::vector<std::pair<TKey, TValue>>;
		raw_data _data;

	public:
		/// <summary>
		/// Initialize a new instance of the SequenceMap class
		/// </summary>
		SequenceMap() :
			_data()
		{
		}

		SequenceMap(SequenceMap&& other) :
			_data(std::move(other._data))
		{
		}

		SequenceMap(const SequenceMap& other) :
			_data(other._data)
		{
		}

		SequenceMap(std::initializer_list<std::pair<TKey, TValue>> init) :
			_data(init)
		{
		}

		~SequenceMap()
		{
		}

		bool Contains(const TKey& key) const
		{
			for (const auto& entry : _data)
			{
				if (entry.first == key)
					return true;
			}

			return false;
		}

		void Insert(const TKey& key, TValue value)
		{
			auto [wasInserted, valueReference] = TryInsert(key, std::move(value));
			if (!wasInserted)
			{
				throw std::runtime_error("Key already exists");
			}
		}

		std::pair<bool, TValue*> TryInsert(TKey key, TValue value)
		{
			if (Contains(key))
			{
				return std::make_pair<bool, TValue*>(false, nullptr);
			}
			else
			{
				_data.push_back(std::make_pair<TKey, TValue>(std::move(key), std::move(value)));
				auto& valueReference = _data[_data.size() - 1];
				return std::make_pair<bool, TValue*>(true, &valueReference.second);;
			}
		}

		bool TryGet(const TKey key, TValue*& value)
		{
			for (auto& entry : _data)
			{
				if (entry.first == key)
				{
					value = &entry.second;
					return true;
				}
			}

			value = nullptr;
			return false;
		}

		bool TryGet(const TKey key, const TValue*& value) const
		{
			for (const auto& entry : _data)
			{
				if (entry.first == key)
				{
					value = &entry.second;
					return true;
				}
			}

			value = nullptr;
			return false;
		}

		raw_data::const_iterator begin() const
		{
			return _data.begin();
		}
		raw_data::const_iterator end() const
		{
			return _data.end();
		}

		/// <summary>
		/// Equality operator
		/// </summary>
		bool operator ==(const SequenceMap<TKey, TValue>& rhs) const
		{
			return _data == rhs._data;
		}
		
		const TValue& operator[](const TKey& key) const
		{
			const TValue* value;
			if (TryGet(key, value))
			{
				return *value;
			}
			else
			{
				throw new std::runtime_error("Missing key");
			}
		}

		SequenceMap& operator=(const SequenceMap& other)
		{
			_data = other._data;
			return *this;
		}
	};
}
