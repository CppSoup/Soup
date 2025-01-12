@echo off
SETLOCAL
SET ScriptsDir=%~dp0
SET RootDir=%ScriptsDir%..\..
SET OutDir=%RootDir%\out
SET RunDir=%OutDir%\run

SET SIGN_COMMAND=signtool sign /n "Open Source Developer, Matthew Asplund" /t http://time.certum.pl /fd sha1 /v

SET SignFiles=%RunDir%\*.exe
SET SignFiles=%RunDir%\Soup\*.dll %SignFiles%
SET SignFiles=%RunDir%\Soup\*.exe %SignFiles%
SET SignFiles=%RunDir%\Soup\PackageManager\*.dll %SignFiles%
SET SignFiles=%RunDir%\Soup\PackageManager\*.exe %SignFiles%
SET SignFiles=%RunDir%\Soup\View\*.dll %SignFiles%
SET SignFiles=%RunDir%\Soup\View\*.exe %SignFiles%

%SIGN_COMMAND% %SignFiles%