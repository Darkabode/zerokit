@ECHO OFF
cls

ECHO.
ECHO -MBR-+
nasm "..\code\zmbr.asm" -o "..\bin\zmbr.bin" -f bin -l "..\bin\zmbr.lst" -Ox -w-orphan-labels
@ECHO ON
@ECHO OFF

ECHO -----+

ECHO.
ECHO -Windows module-+
nasm "..\code\windows\windows.asm" -o "..\bin\windows.bin" -f bin -l "..\bin\windows.lst" -Ox -w-orphan-labels -i ..\code\ -i ..\code\windows\ -d _DEBUG
@ECHO ON
@ECHO OFF

ECHO ----------------+

ECHO.
ECHO -ZBK-+
nasm "..\code\zbk.asm" -o "..\bin\zbk.bin" -f bin -l "..\bin\zbk.lst" -Ox -w-orphan-labels -i ..\code\ -i ..\bin\ -d _DEBUG
@ECHO ON
@ECHO OFF

ECHO -----+

ECHO.