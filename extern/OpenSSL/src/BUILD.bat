echo -------------------------------------------
echo Start compiling OpenSSL :: %date% %time%
echo -------------------------------------------
call BUILD_X86.bat
call BUILD_X64.bat
echo -------------------------------------------
echo Done compiling OpenSSL :: %date% %time%
echo -------------------------------------------