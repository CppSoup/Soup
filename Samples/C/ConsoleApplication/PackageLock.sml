Version: 5
Closures: {
	Root: {
		C: {
			"Samples.C.ConsoleApplication": { Version: "../ConsoleApplication", Build: "Build0", Tool: "Tool0" }
		}
	}
	Build0: {
		Wren: {
			"mwasplund|Soup.C": { Version: "0.2.0" }
		}
	}
	Tool0: {
		"C++": {
			"mwasplund|copy": { Version: "1.0.0" }
			"mwasplund|mkdir": { Version: "1.0.0" }
		}
	}
}