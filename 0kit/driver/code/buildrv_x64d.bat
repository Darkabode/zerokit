ml64.exe /Fo "objchk_win7_amd64\amd64\private.obj" /c amd64\private.asm
ml64.exe /Fo "objchk_win7_amd64\amd64\mod_commonApi.obj" /c ..\..\mod_common\code\amd64\mod_commonApi.asm
ml64.exe /Fo "objchk_win7_amd64\amd64\mod_launcherApi.obj" /c ..\..\mod_launcher\code\amd64\mod_launcherApi.asm
ml64.exe /Fo "objchk_win7_amd64\amd64\mod_networkApi.obj" /c ..\..\mod_network\code\amd64\mod_networkApi.asm
cl.exe @cl_x64d.rsp main.c
link.exe @link_x64d.rsp