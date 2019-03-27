ml64.exe /Fo "..\obj\x64\release\private.obj" /c amd64\private.asm
ml64.exe /Fo "..\obj\x64\release\mod_protectorApi.obj" /c amd64\mod_protectorApi.asm
cl.exe @cl_x64.rsp main.c
link.exe @link_x64.rsp