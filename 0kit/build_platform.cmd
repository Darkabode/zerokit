@echo off

set ERRCODE=0

if "%WDKPATH%"=="" goto :WdkPathNotDefined

echo.
echo Build for platform: %1
echo WDK path: %WDKPATH%
echo.

if "%1"=="" goto NotUsedAlone
if "%1"=="x32" goto x32env
if "%1"=="x64" goto x64env
if "%1"=="ia64" goto ia64env

:x32env
call %WDKPATH%\bin\setenv.bat %WDKPATH% fre x86 WXP no_oacr

goto build

:x64env
call %WDKPATH%\bin\setenv.bat %WDKPATH% fre x64 win7 no_oacr

goto build

:ia64env
call %WDKPATH%\bin\setenv.bat %WDKPATH% fre 64 win7 no_oacr

goto build

:WdkPathNotDefined
echo.
echo ERROR: WDKPATH is not defined!
echo.
set ERRCODE=1
goto end

:NotUsedAlone
echo.
echo ERROR: Run this batch file only from mods_builder.cmd!
echo.
set ERRCODE=1
goto end

:build

cd %2

cd mod_common\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_fs\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_protector\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_tasks\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_launcher\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_network\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_tcpip\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_netcomm\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_userio\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

cd mod_logic\code
del /s /q ..\obj\%1\release
call buildrv_%1.bat
cd ..\..

:end
exit %ERRCODE%