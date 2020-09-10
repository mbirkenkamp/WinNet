@echo off
call "C:\Program Files (x86)\Microsoft Visual C++ Build Tools\vcbuildtools_msbuild.bat"
set pathMSBuild=%CD%\src\
cd %pathMSBuild%   
msbuild.exe cryptest.sln /p:configuration=debug

xcopy "%pathMSBuild%\Win32\Output\Debug\cryptlib.lib" "%pathMSBuild%..\lib\debug\x86\MDD\" /s /i /q /y /c
xcopy "%pathMSBuild%\Win32\Output\Debug\cryptlib.pdb" "%pathMSBuild%..\lib\debug\x86\MDD\" /s /i /q /y /c

echo -------------------------------------------
echo Done compiling in Debug Mode (x86)
echo -------------------------------------------
exit