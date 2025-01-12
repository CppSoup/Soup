Name: 'Soup.Core'
Language: 'C++|0'
Version: 0.1.1
Defines: [
	# 'TRACE_SYSTEM_ACCESS'
	# 'TRACE_FILE_SYSTEM_STATE'
]
IncludePaths: [
	'source/'
]
Interface: 'source/Module.cpp'
Source: [
	'source/recipe/LanguageReferenceParser.cpp'
	'source/sml/SMLParser.cpp'
]
Partitions: [
	{ Source: 'source/build/BuildConstants.cpp' }
	{ Source: 'source/build/BuildFailedException.cpp' }
	{ Source: 'source/build/DependencyTargetSet.cpp' }
	{ Source: 'source/build/FileSystemState.cpp' }
	{ Source: 'source/build/KnownLanguage.cpp' }
	{ Source: 'source/build/MacroManager.cpp' }
	{ Source: 'source/operation-graph/CommandInfo.cpp' }
	{ Source: 'source/operation-graph/OperationInfo.cpp', Imports: [ 'source/build/FileSystemState.cpp', 'source/operation-graph/CommandInfo.cpp' ] }
	{ Source: 'source/operation-graph/OperationResult.cpp', Imports: [ 'source/build/FileSystemState.cpp' ] }
	{ Source: 'source/operation-graph/OperationResults.cpp', Imports: [ 'source/operation-graph/OperationInfo.cpp', 'source/operation-graph/OperationResult.cpp' ] }
	{ Source: 'source/operation-graph/OperationResultsManager.cpp', Imports: [ 'source/build/FileSystemState.cpp', 'source/operation-graph/OperationResultsReader.cpp', 'source/operation-graph/OperationResultsWriter.cpp' ] }
	{ Source: 'source/operation-graph/OperationResultsReader.cpp', Imports: [ 'source/build/FileSystemState.cpp', 'source/operation-graph/OperationResults.cpp' ] }
	{ Source: 'source/operation-graph/OperationResultsWriter.cpp', Imports: [ 'source/build/FileSystemState.cpp', 'source/operation-graph/OperationResults.cpp' ] }
	{ Source: 'source/recipe/LanguageReference.cpp' }
	{ Source: 'source/recipe/PackageIdentifier.cpp', Imports: [ 'source/recipe/PackageName.cpp' ] }
	{ Source: 'source/recipe/PackageName.cpp' }
	{ Source: 'source/recipe/PackageReference.cpp', Imports: [ 'source/recipe/PackageIdentifier.cpp' ] }
	{ Source: 'source/utilities/HandledException.cpp' }
	{ Source: 'source/utilities/SequenceMap.cpp' }
	{ Source: 'source/value-table/Value.cpp', Imports: [ 'source/recipe/LanguageReference.cpp', 'source/recipe/PackageReference.cpp' ] }
	{ Source: 'source/value-table/ValueTableManager.cpp', Imports: [ 'source/value-table/Value.cpp', 'source/value-table/ValueTableReader.cpp', 'source/value-table/ValueTableWriter.cpp' ] }
	{ Source: 'source/value-table/ValueTableReader.cpp', Imports: [ 'source/value-table/Value.cpp' ] }
	{ Source: 'source/value-table/ValueTableWriter.cpp', Imports: [ 'source/value-table/Value.cpp' ] }
]
Dependencies: {
	Build: [
		'mwasplund|Soup.Test.Cpp@0'
	]
	Runtime: [
		'mwasplund|Opal@0'
		'mwasplund|reflex@1'
		'../../monitor/host/'
		'mwasplund|wren@1'
		'mwasplund|CryptoPP@1'
	]
	Test: [
		'mwasplund|Soup.Test.Assert@0'
	]
}
Tests: {
	Source: [
		'tests/gen/Main.cpp'
	]
	IncludePaths: [
		'tests/'
	]
}