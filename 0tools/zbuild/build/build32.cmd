@ECHO OFF

set SIMPLE_MODE=
set OUT_FILE="..\locker32r0.dll"
set CUSTOM_LIBS=..\..\..\..\..\loader\userio_api\zuserio\bin\zuserio.lib
set DEF=/DEF:"..\..\code\locker.def"

:parsecommandline
if '%1' == '/simple_mode' goto simple_mode
goto build

:simple_mode
rem echo SIMPLE_MODE ON
set SIMPLE_MODE=/D "SIMPLE_MODE"
set OUT_FILE="..\locker32.dll"
set CUSTOM_LIBS=
set DEF=
shift
goto parsecommandline

:build

call c:\_zer0wave\__builder\bin\ms_compiler\bin\setenv.bat c:\_zer0wave\__builder\bin\ms_compiler\ fre x86 WXP no_oacr >NUL
set LIB=c:\_zer0wave\__builder\bin\ms_compiler\lib\wxp\i386;%LIB%
set INCLUDE=c:\_zer0wave\__builder\bin\ms_compiler\inc\crt;c:\_zer0wave\__builder\bin\ms_compiler\inc\sdk;%INCLUDE%

cd ..\..\..\0kit\locker\client\code

cl.exe /O1 /Ob1 /Oi /Os /GL %SIMPLE_MODE% /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /GF /FD /MT /GS- /Zc:forScope- /GR- /Fo"..\autobuild\x32\\" /Fd"..\autobuild\x32\vc90.pdb" /W4 /nologo /c /Gd /TP /errorReport:prompt main.cpp >NUL

IF ERRORLEVEL 2 GOTO EXIT
IF ERRORLEVEL 1 GOTO EXIT
IF ERRORLEVEL 0 GOTO COMPILER_OK

:COMPILER_OK

cd "..\autobuild\x32"

link.exe /OUT:%OUT_FILE% /INCREMENTAL:NO /NOLOGO /DLL %DEF% /MANIFEST:NO /NODEFAULTLIB /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /SAFESEH:NO /OPT:NOWIN98 /LTCG /ENTRY:"DllMain" /DYNAMICBASE /MACHINE:X86 /ERRORREPORT:PROMPT %CUSTOM_LIBS% kernel32.lib user32.lib gdi32.lib advapi32.lib ole32.lib ..\..\code\ntdll32.lib gdiplus.lib uuid.lib Comctl32.lib strmiids.lib Winmm.lib ..\..\code\libcntpr32.lib main.obj >NUL

IF ERRORLEVEL 2 GOTO EXIT
IF ERRORLEVEL 1 GOTO EXIT
IF ERRORLEVEL 0 GOTO LINKER_OK

:EXIT
ECHO failed

:LINKER_OK
del .\*.obj
del .\*.idb
del ..\*.exp
del ..\*.lib

cd ..\..

exit