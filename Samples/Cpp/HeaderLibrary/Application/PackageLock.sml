Version: 5
Closures: {
	Root: {
		'C++': {
			'Samples.Cpp.HeaderLibrary.Application': { Version: '../Application', Build: 'Build0', Tool: 'Tool0' }
			'Samples.Cpp.HeaderLibrary.Library': { Version: '../Library/', Build: 'Build0', Tool: 'Tool0' }
		}
	}
	Build0: {
		Wren: {
			'mwasplund|Soup.Cpp': { Version: '0.13.1' }
		}
	}
	Tool0: {
		'C++': {
			'mwasplund|copy': { Version: '1.1.0' }
			'mwasplund|mkdir': { Version: '1.1.0' }
		}
	}
}