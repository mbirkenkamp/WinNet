@echo off
title Compiling CryptoPP x64 [RELEASE] - [%date% %time%] - [4/4]

set /p TOOL=<../../Config/BUILDTOOLS_PATH
call "%TOOL%\Community\VC\Auxiliary\Build\vcvars64.bat"

set pathMSBuild=%CD%\src\
rmdir %pathMSBuild%\x64\Output\ /s /q
cd %pathMSBuild%
msbuild.exe cryptest.sln /property:Configuration=Release /property:Platform=x64 /property:RuntimeLibrary=MultiThreadedDLL /target:cryptlib:Rebuild
cd ..

xcopy "%pathMSBuild%\x64\Output\Release\cryptlib.lib" "%pathMSBuild%..\lib\release\x64\MD\" /s /i /q /y /c
xcopy "%pathMSBuild%\x64\Output\Release\cryptlib.pdb" "%pathMSBuild%..\lib\release\x64\MD\" /s /i /q /y /c

rmdir %pathMSBuild%\x64\Output\ /s /q
cd %pathMSBuild%
msbuild.exe cryptest.sln /property:Configuration=Release /property:Platform=x64 /property:RuntimeLibrary=MultiThreaded /target:cryptlib:Rebuild
cd ..

xcopy "%pathMSBuild%\x64\Output\Release\cryptlib.lib" "%pathMSBuild%..\lib\release\x64\MT\" /s /i /q /y /c
xcopy "%pathMSBuild%\x64\Output\Release\cryptlib.pdb" "%pathMSBuild%..\lib\release\x64\MT\" /s /i /q /y /c

echo -------------------------------------------
echo Done compiling in Release Mode (x64)
echo -------------------------------------------

exit