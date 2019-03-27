@ECHO OFF

set SIMPLE_MODE=

:parsecommandline
if '%1' == '/simple_mode' goto simple_mode

goto build

:simple_mode
rem echo SIMPLE_MODE ON
set SIMPLE_MODE=/simple_mode
shift
goto parsecommandline
:build

cd code

start /wait ..\build64.cmd %SIMPLE_MODE%
start /wait ..\build32.cmd %SIMPLE_MODE%

cd ..

