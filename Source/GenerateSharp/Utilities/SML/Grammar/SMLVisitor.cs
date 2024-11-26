//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     ANTLR Version: 4.11.1
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

// Generated from SML.g4 by ANTLR 4.11.1

// Unreachable code detected
#pragma warning disable 0162
// The variable '...' is assigned but its value is never used
#pragma warning disable 0219
// Missing XML comment for publicly visible type or member '...'
#pragma warning disable 1591
// Ambiguous reference in cref attribute
#pragma warning disable 419

using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using IToken = Antlr4.Runtime.IToken;

/// <summary>
/// This interface defines a complete generic visitor for a parse tree produced
/// by <see cref="SMLParser"/>.
/// </summary>
/// <typeparam name="Result">The return type of the visit operation.</typeparam>
[System.CodeDom.Compiler.GeneratedCode("ANTLR", "4.11.1")]
// [System.CLSCompliant(false)]
public interface ISMLVisitor<Result> : IParseTreeVisitor<Result> {
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.document"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitDocument([NotNull] SMLParser.DocumentContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.table"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitTable([NotNull] SMLParser.TableContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.tableContent"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitTableContent([NotNull] SMLParser.TableContentContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.tableValue"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitTableValue([NotNull] SMLParser.TableValueContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>keyLiteral</c>
	/// labeled alternative in <see cref="SMLParser.key"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitKeyLiteral([NotNull] SMLParser.KeyLiteralContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>keyString</c>
	/// labeled alternative in <see cref="SMLParser.key"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitKeyString([NotNull] SMLParser.KeyStringContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.array"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitArray([NotNull] SMLParser.ArrayContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.arrayContent"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitArrayContent([NotNull] SMLParser.ArrayContentContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.languageName"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitLanguageName([NotNull] SMLParser.LanguageNameContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.userName"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitUserName([NotNull] SMLParser.UserNameContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.packageName"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitPackageName([NotNull] SMLParser.PackageNameContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.language"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitLanguage([NotNull] SMLParser.LanguageContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.languageReference"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitLanguageReference([NotNull] SMLParser.LanguageReferenceContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.packageReference"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitPackageReference([NotNull] SMLParser.PackageReferenceContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueFloat</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueFloat([NotNull] SMLParser.ValueFloatContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueInteger</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueInteger([NotNull] SMLParser.ValueIntegerContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueLanguageReference</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueLanguageReference([NotNull] SMLParser.ValueLanguageReferenceContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valuePackageReference</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValuePackageReference([NotNull] SMLParser.ValuePackageReferenceContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueVersion</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueVersion([NotNull] SMLParser.ValueVersionContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueString</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueString([NotNull] SMLParser.ValueStringContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueTrue</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueTrue([NotNull] SMLParser.ValueTrueContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueFalse</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueFalse([NotNull] SMLParser.ValueFalseContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueTable</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueTable([NotNull] SMLParser.ValueTableContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>valueArray</c>
	/// labeled alternative in <see cref="SMLParser.value"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitValueArray([NotNull] SMLParser.ValueArrayContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>newlineDelimiter</c>
	/// labeled alternative in <see cref="SMLParser.delimiter"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitNewlineDelimiter([NotNull] SMLParser.NewlineDelimiterContext context);
	/// <summary>
	/// Visit a parse tree produced by the <c>commaDelimiter</c>
	/// labeled alternative in <see cref="SMLParser.delimiter"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitCommaDelimiter([NotNull] SMLParser.CommaDelimiterContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.leadingNewlines"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitLeadingNewlines([NotNull] SMLParser.LeadingNewlinesContext context);
	/// <summary>
	/// Visit a parse tree produced by <see cref="SMLParser.trailingNewlines"/>.
	/// </summary>
	/// <param name="context">The parse tree.</param>
	/// <return>The visitor result.</return>
	Result VisitTrailingNewlines([NotNull] SMLParser.TrailingNewlinesContext context);
}
