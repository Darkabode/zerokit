rem @echo off

set CURRDIR=%cd%

del /s /q mods

cmd.exe /c build_platform.cmd x32 %CURRDIR%	
cmd.exe /c build_platform.cmd x64 %CURRDIR%

:end
exit /b 0