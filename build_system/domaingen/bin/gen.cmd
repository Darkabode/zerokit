@ECHO OFF
:parsecommandline
if '%1' == '' goto bad
if '%2' == '' goto bad
goto ok

:bad
domaingen.exe
goto exit


:OK
domaingen.exe -n=%1 -l=7 -h=32 -p=24 -d=%2 -k="key.public" -v

:exit