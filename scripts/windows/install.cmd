SETLOCAL
SET ScriptsDir=%~dp0
SET RootDir=%ScriptsDir%..\..
SET OutDir=%RootDir%\out

SET SOUP_VERSION=0.41.4

pushd %OutDir%\release
msiexec /package soup-build-%SOUP_VERSION%-windows-x64.msi /passive
popd