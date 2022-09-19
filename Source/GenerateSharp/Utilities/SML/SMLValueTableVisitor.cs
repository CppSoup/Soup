// <copyright file="SMLValueTableVisitor.cs" company="Soup">
// Copyright (c) Soup. All rights reserved.
// </copyright>

using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Soup.Build.Utilities
{
	/// <summary>
	/// This class provides an implementation of <see cref="ISMLVisitor{Result}"/>,
	/// which converts the input SML into a <see cref="SMLValue"/>
	/// </summary>
	public class SMLValueTableVisitor : AbstractParseTreeVisitor<object>, ISMLVisitor<object>
	{
		private CommonTokenStream _tokens;

		public SMLValueTableVisitor(CommonTokenStream tokens)
		{
			_tokens = tokens;
		}

		public virtual object VisitDocument(SMLParser.DocumentContext context)
		{
			var tableContent = (Dictionary<string, SMLTableValue>)context.tableContent().Accept(this);
			return new SMLDocument(tableContent);
		}

		public virtual object VisitTable(SMLParser.TableContext context)
		{
			var tableContent = (Dictionary<string, SMLTableValue>)context.tableContent().Accept(this);
			return new SMLTable(
				new SMLToken(context.OPEN_BRACE().GetText()),
				tableContent,
				new SMLToken(context.CLOSE_BRACE().GetText()));
		}

		public virtual object VisitTableContent(SMLParser.TableContentContext context)
		{
			var tableContent = new Dictionary<string, SMLTableValue>();
			foreach (var value in context.tableValue())
			{
				var tableValue = (SMLTableValue)value.Accept(this);
				tableContent.Add(tableValue.Key.Text, tableValue);
			}

			return tableContent;
		}

		public virtual object VisitTableValue(SMLParser.TableValueContext context)
		{
			return new SMLTableValue(
				BuildToken(context.KEY()),
				(SMLValue)context.value().Accept(this));
		}

		public virtual object VisitArray(SMLParser.ArrayContext context)
		{
			var arrayContent = (List<SMLValue>)context.arrayContent().Accept(this);
			return new SMLArray(
				BuildToken(context.OPEN_BRACKET()),
				arrayContent,
				BuildToken(context.CLOSE_BRACKET()));
		}

		public virtual object VisitArrayContent(SMLParser.ArrayContentContext context)
		{
			var arrayContent = new List<SMLValue>();
			foreach (var value in context.value())
			{
				arrayContent.Add((SMLValue)value.Accept(this));
			}

			return arrayContent;
		}

		public virtual object VisitValueInteger(SMLParser.ValueIntegerContext context)
		{
			var integerToken = context.INTEGER().Symbol;
			var value = long.Parse(integerToken.Text);
			return new SMLValue(
				new SMLIntegerValue(value));
		}

		public virtual object VisitValueString(SMLParser.ValueStringContext context)
		{
			var literal = context.STRING_LITERAL().Symbol.Text;
			var content = literal.Substring(1, literal.Length - 2);

			return new SMLValue(new SMLStringValue(
				content,
				new SMLToken("\"")
				{
					LeadingTrivia = GetLeadingTrivia(context.STRING_LITERAL()),
				},
				new SMLToken(content),
				new SMLToken("\"")
				{
					TrailingTrivia = GetTrailingTrivia(context.STRING_LITERAL()),
				}));
		}

		public virtual object VisitValueTrue(SMLParser.ValueTrueContext context)
		{
			return new SMLValue(new SMLBooleanValue(true));
		}

		public virtual object VisitValueFalse(SMLParser.ValueFalseContext context)
		{
			return new SMLValue(new SMLBooleanValue(false));
		}

		public virtual object VisitValueTable(SMLParser.ValueTableContext context)
		{
			var table = (SMLTable)context.table().Accept(this);
			return new SMLValue(table);
		}

		public virtual object VisitValueArray(SMLParser.ValueArrayContext context)
		{
			var array = (SMLArray)context.array().Accept(this);
			return new SMLValue(array);
		}

		public virtual object VisitDelimiter(SMLParser.DelimiterContext context)
		{
			throw new NotImplementedException();
		}

		private SMLToken BuildToken(ITerminalNode node)
		{
			return new SMLToken(
				GetLeadingTrivia(node),
				node.Symbol.Text,
				GetTrailingTrivia(node));
		}

		private List<string> GetLeadingTrivia(ITerminalNode node)
		{
			var left = _tokens.GetHiddenTokensToLeft(node.Symbol.TokenIndex);
			var leadingTrivia = left != null ? left.Select(value => value.Text).ToList() : new List<string>();
			return leadingTrivia;
		}

		private List<string> GetTrailingTrivia(ITerminalNode node)
		{
			var right = _tokens.GetHiddenTokensToRight(node.Symbol.TokenIndex);
			var trailingTrivia = right != null ? right.Select(value => value.Text).ToList() : new List<string>();
			return trailingTrivia;
		}
	}
}