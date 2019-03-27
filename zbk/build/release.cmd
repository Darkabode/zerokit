@ECHO OFF
cls

ECHO.
ECHO -Payload x32-+
fasm ..\code\payload_x32.asm ..\bin\payload_x32.bin
@ECHO ON
@ECHO OFF

ECHO ----------------+

ECHO.
ECHO -Payload x64-+
fasm ..\code\payload_x64.asm ..\bin\payload_x64.bin
@ECHO ON
@ECHO OFF

ECHO ----------------+

ECHO.
ECHO -Windows module-+
fasm ..\code\windows.asm ..\bin\windows.bin
@ECHO ON
@ECHO OFF

ECHO ----------------+

ECHO.
ECHO -Bootkit Module-+
fasm ..\code\module.asm ..\bin\module.bin
@ECHO ON
@ECHO OFF

ECHO ----------------+

ECHO.
ECHO -Complete bundle-+
fasm "..\code\complete.asm" "..\bin\complete.bin" -s "..\bin\complete.sym"
@ECHO ON
@ECHO OFF

ECHO -----------------+

ECHO.