@ECHO OFF

set KEY_PATH=

:parsecommandline
if '%1' == '/key' goto set_key_path
goto build

:set_key_path
rem echo AFF_ID
set KEY_PATH=%2
shift
shift
goto parsecommandline

:build

zpacker.exe -f="overlord32.dll*overlord32.dll*svchost.exe*33554446" -f="overlord64.dll*overlord64.dll*svchost.exe*33554447" -f="conf.z*conf.z**50331662" -k="%KEY_PATH%" -o="." -n="overlord" -u=1440