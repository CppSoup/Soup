// <copyright file="RecipeSML.h" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

#pragma once
#include "Recipe.h"
#include "sml/SML.h"

namespace Soup::Core
{
	/// <summary>
	/// The recipe SML serialize manager
	/// </summary>
	export class RecipeSML
	{
	public:
		/// <summary>
		/// Load from stream
		/// </summary>
		static RecipeTable Deserialize(
			const Path& recipeFile,
			std::istream& stream)
		{
			try
			{
				// Read the entire file for fastest read operation
				stream.seekg(0, std::ios_base::end);
				auto size = stream.tellg();
				stream.seekg(0, std::ios_base::beg);

				auto contentBuffer = std::vector<char>(size);
				stream.read(contentBuffer.data(), size);
			
				// Read the contents of the recipe file
				auto root = SMLDocument::Parse(contentBuffer.data(), size);

				// Load the entire root table
				auto table = RecipeTable();
				Parse(table, root.GetRoot());

				return table;
			}
			catch(const std::exception& ex)
			{
				throw std::runtime_error(
					std::format("Parsing the Recipe SML failed: {} {}", ex.what(), recipeFile.ToString()));
			}
		}

		/// <summary>
		/// Save the recipe to the root file
		/// </summary>
		static void Serialize(RecipeTable& recipeTable, std::ostream& stream)
		{
			// Serialize the contents of the recipe
			auto document = SMLDocument(Build(recipeTable));

			// Write out the entire root table
			stream << document;
		}

	private:
		static RecipeValue Parse(const SMLValue& source)
		{
			switch (source.GetType())
			{
				case SMLValueType::Boolean:
				{
					return RecipeValue(source.AsBoolean());
				}
				case SMLValueType::Integer:
				{
					return RecipeValue(source.AsInteger());
				}
				case SMLValueType::Float:
				{
					return RecipeValue(source.AsFloat());
				}
				case SMLValueType::String:
				{
					return RecipeValue(source.AsString());
				}
				case SMLValueType::Array:
				{
					auto valueList = RecipeList();
					Parse(valueList, source.AsArray());
					return RecipeValue(std::move(valueList));
				}
				case SMLValueType::Table:
				{
					auto valueTable = RecipeTable();
					Parse(valueTable, source.AsTable());
					return RecipeValue(std::move(valueTable));
				}
				case SMLValueType::Version:
				{
					return RecipeValue(source.AsVersion());
				}
				case SMLValueType::PackageReference:
				{
					return RecipeValue(source.AsPackageReference());
				}
				case SMLValueType::LanguageReference:
				{
					return RecipeValue(source.AsLanguageReference());
				}
				default:
				{
					throw std::runtime_error("Unknown SML type.");
				}
			}
		}

		static void Parse(RecipeTable& target, const SMLTable& source)
		{
			for (const auto& [key, value] : source.GetValue())
			{
				auto recipeValue = Parse(value);
				target.Insert(key, std::move(recipeValue));
			}
		}

		static void Parse(RecipeList& target, const SMLArray& source)
		{
			target.reserve(source.GetSize());
			for (size_t i = 0; i < source.GetSize(); i++)
			{
				auto value = Parse(source[i]);
				target.push_back(std::move(value));
			}
		}

		static SMLValue Build(const RecipeValue& value)
		{
			switch (value.GetType())
			{
				case RecipeValueType::Table:
					return Build(value.AsTable());
				case RecipeValueType::List:
					return Build(value.AsList());
				case RecipeValueType::String:
					return SMLValue(value.AsString());
				case RecipeValueType::Integer:
					return SMLValue(value.AsInteger());
				case RecipeValueType::Float:
					return SMLValue(value.AsFloat());
				case RecipeValueType::Boolean:
					return SMLValue(value.AsBoolean());
				case RecipeValueType::Version:
					return SMLValue(value.AsVersion());
				case RecipeValueType::LanguageReference:
					return SMLValue(value.AsLanguageReference());
				case RecipeValueType::PackageReference:
					return SMLValue(value.AsPackageReference());
				default:
					throw std::runtime_error("Unknown Recipe value type.");
			}
		}

		static SMLTable Build(const RecipeTable& table)
		{
			auto result = SequenceMap<std::string, SMLValue>();

			for (const auto& [key, value] : table)
			{
				result.Insert(key, Build(value));
			}

			return SMLTable(std::move(result));
		}

		static SMLArray Build(const RecipeList& list)
		{
			auto result = std::vector<SMLValue>();

			for (auto& value : list)
			{
				result.push_back(Build(value));
			}

			return SMLArray(std::move(result));
		}
	};
}
