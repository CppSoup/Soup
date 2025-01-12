# SWhere
To allow external SDKs to be injected into Soup builds the [Local User Config](../architecture/local-user-config.md) file allows for the user to define a set of know SDKs with a list of directories and properties. These properties can be manually defined, but to make setup easier this tool will discover a set of shared SDKs and register them in the config file.

## List Of SDKs
* MSVC - Find the latest Visual C++ compiler using VSWhere.
* Roslyn - Find the latest Roslyn compiler for C# using VSWhere.
* DotNet - Set the known location and version for the dotnet CLR.
* Windows - Find the latest Windows SDK by looking in a known folder location.
* NetFXTools - Set the known location for the NetFX Tools.
