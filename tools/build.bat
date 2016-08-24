
:: Set project relevant settings
set VCPROJECT="..\project\HTML5.vcxproj"
:: set VCTOOLS="%VS110COMNTOOLS%..\..\VC\vcvarsall.bat"
:: set VCTOOLS="C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\Tools\..\..\VC\vcvarsall.bat"


:: Compile x86
:: call %VCTOOLS% x86

:: MSBuild %VCPROJECT% /t:Rebuild /p:Configuration=Release
:: IF ERRORLEVEL 1 GOTO COMPILERROR


:: Compile x64
:: call %VCTOOLS% x64

MSBuild %VCPROJECT% /t:Rebuild /p:Configuration=Release
IF ERRORLEVEL 1 GOTO COMPILERROR

:: End
GOTO ENDOK

:COMPILERROR

::Trigger a Syntax error
--ERROR_DETECTED--

:ENDOK
