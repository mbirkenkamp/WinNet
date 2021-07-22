@echo off
title Compiling OpenSSL x86 Debug - [%date% %time%] - [7/8]

set CURPARTITION=%CD:~0,2%
set CURDIR=%CD%

set /p TOOL=<../../../Config/BUILDTOOLS_PATH
call "%TOOL%\Community\VC\Auxiliary\Build\vcvars32.bat"

%CURPARTITION%
cd %CURDIR%

cd openssl-master

REM USE ALL CPU CORES
set CL=/MP

nmake clean
perl Configure VC-WIN32 --debug --prefix=%CD%\..\BIN\Lib\x86\Debug --openssldir=%CD%\..\BIN\SSL no-shared
nmake
nmake install_sw

exit