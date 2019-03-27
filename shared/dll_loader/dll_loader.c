/**	Перед подключением файла нужно определить следующие функции специфические для среды исполнения:
	* SYS_ALLOCATOR(sz) - выделяет память указанного размера.
	* SYS_DEALLOCATOR(ptr) - освобождает память по указанному адресу.
	* MEMCPY - копирует данные указанного размера из источника (src) в приёмник (dest).
	* GET_MODULE_BASE(hProcess, name) - возвращает базу модуля с указанным именем.
    * LOAD_LIBRARY(name) - загружает бибилиотеку в контексте целевого процесса.
	* INVALID_MODULE_BASE - константа определяющая неверную базу
	* GET_EXPORT_SYMBOL(base, name) - возвращает адрес экспортируемого символа с указанными именем
*/

#ifdef _WIN64
#define POINTER_TYPE ULONGLONG
#else
#define POINTER_TYPE UINT32
#endif

#define GET_DIRECTORY_PTR(pNtHdrs, idx)	&pNtHdrs->OptionalHeader.DataDirectory[idx]

PUCHAR dll_find_export_function(pdll_handle_t pDllHandle, HANDLE hProcess, 
#ifndef OS_RING3
                                PKAPC_STATE pApcState,
#endif // OS_RING3                                
                                PUCHAR moduleBase, char* funcName)
{
    PUCHAR pFuncPtr;
    PIMAGE_DOS_HEADER pDllDosHdr = (PIMAGE_DOS_HEADER)moduleBase;
    PIMAGE_NT_HEADERS pDllNtHdr = (PIMAGE_NT_HEADERS)(moduleBase + pDllDosHdr->e_lfanew);
    PIMAGE_DATA_DIRECTORY pED = &pDllNtHdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    USE_GLOBAL_BLOCK

    pFuncPtr = (PUCHAR)GET_EXPORT_SYMBOL(moduleBase, funcName);
    if (pFuncPtr >= (moduleBase + pED->VirtualAddress) && pFuncPtr < (moduleBase + pED->VirtualAddress + pED->Size)) {
        char *ptr, *fwdFuncName, *fwdDllName;
        void* fwdDllBase;
        uint32_t nameLen;

        ptr = (char*)pFuncPtr;

        for ( ; *ptr != '.' && *ptr != '\0'; ++ptr);
        if (*ptr == '\0') {
            return NULL;
        }

        nameLen = (uint32_t)(ptr - (char*)pFuncPtr + 5);
        fwdDllName = SYS_ALLOCATOR(nameLen);
        if (fwdDllName == NULL) {
            return NULL;
        }
        MEMSET(fwdDllName, 0, nameLen);
        MEMCPY(fwdDllName, (char*)pFuncPtr, nameLen - 5);
        *(UINT32*)(fwdDllName + nameLen - 5) = 0x6c6c642e; 
        
        fwdFuncName = ++ptr;

        fwdDllBase = GET_MODULE_BASE(pDllHandle->pep/*hProcess*/, fwdDllName);
        if (fwdDllBase == INVALID_MODULE_BASE) {
#ifndef OS_RING3
            KE_UNSTACK_DETACH_PROCESS(pApcState);
#else
            fwdDllBase = 
#endif // OS_RING3
                LOAD_LIBRARY(fwdDllName);
#ifndef OS_RING3
            KE_STACK_ATTACH_PROCESS(pDllHandle->pep, pApcState);
#endif // OS_RING3
            fwdDllBase = GET_MODULE_BASE(pDllHandle->pep/*hProcess*/, fwdDllName);
#ifndef OS_RING3
            KE_UNSTACK_DETACH_PROCESS(pApcState);
#endif // OS_RING3
            if (fwdDllBase == INVALID_MODULE_BASE) {
#ifndef OS_RING3
                KE_STACK_ATTACH_PROCESS(pDllHandle->pep, pApcState);
#endif // OS_RING3
                SYS_DEALLOCATOR(fwdDllName);
                return NULL;
            }
#ifndef OS_RING3
            KE_STACK_ATTACH_PROCESS(pDllHandle->pep, pApcState);
#endif // OS_RING3
        }
        SYS_DEALLOCATOR(fwdDllName);

        pFuncPtr = (PUCHAR)DLL_FIND_EXPORT_FUNCTION(pDllHandle, hProcess, 
#ifndef OS_RING3
            pApcState,
#endif // OS_RING3  
            fwdDllBase, fwdFuncName);
    }

    return pFuncPtr;
}

PVOID dll_mem_load(HANDLE hProcess, unsigned char* realBase, const char* pDllBuffer, pdll_handle_t pDllHandle)
{
	PIMAGE_DOS_HEADER dosHdr;
	PIMAGE_NT_HEADERS origNtHdrs, newNtHdrs;
	unsigned char* fakeBase = NULL;
	UINT32 locationDelta;
	UINT32 i;
	PIMAGE_SECTION_HEADER pSection;
	UINT16 numberOfSections;
	PIMAGE_DATA_DIRECTORY pDirectory;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc;
	PUCHAR moduleBase;
	POINTER_TYPE* thunkRef;
	POINTER_TYPE* funcRef;
	POINTER_TYPE* dwAddressArray;
	PIMAGE_DATA_DIRECTORY dwNameArray;
	unsigned char* dwExportDir;
	PIMAGE_BASE_RELOCATION pReloc;
	int offset, type;
#ifndef OS_RING3
    KAPC_STATE apcState;
#endif // OS_RING3
    
    USE_GLOBAL_BLOCK

	dosHdr = (PIMAGE_DOS_HEADER)pDllBuffer;
	if (dosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
		return NULL;
	}

	origNtHdrs = (PIMAGE_NT_HEADERS)(pDllBuffer + dosHdr->e_lfanew);
	if (origNtHdrs->Signature != IMAGE_NT_SIGNATURE) {
		return NULL;
	}

	// Резервируем память для нашего образа
	fakeBase = (unsigned char*)SYS_ALLOCATOR(origNtHdrs->OptionalHeader.SizeOfImage);

	if (fakeBase == NULL) {
		return NULL;
	}

	// Копируем PE-заголовок, включая MZ-заголовк со DOS-стабом.
	MEMCPY(fakeBase, pDllBuffer, origNtHdrs->OptionalHeader.SizeOfHeaders);
	newNtHdrs = (PIMAGE_NT_HEADERS)(fakeBase + dosHdr->e_lfanew);

	// Обновляем базу
#ifdef OS_RING3
	newNtHdrs->OptionalHeader.ImageBase = (POINTER_TYPE)(realBase == NULL ? fakeBase : realBase);
    pDllHandle->moduleBase = fakeBase;
    pDllHandle->pNtHeaders = newNtHdrs;
#else
	newNtHdrs->OptionalHeader.ImageBase = (POINTER_TYPE)realBase;
#endif

	// Копируем все секции.
	pSection = IMAGE_FIRST_SECTION(newNtHdrs);
	numberOfSections = newNtHdrs->FileHeader.NumberOfSections;

	for (i = 0; i < numberOfSections; ++i, ++pSection) {
		if (pSection->SizeOfRawData > 0) {
			MEMCPY(fakeBase + pSection->VirtualAddress, pDllBuffer + pSection->PointerToRawData, pSection->SizeOfRawData);
		}
	}

	// Обрабатываем таблицу импорта.
	pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_IMPORT);

	pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)(fakeBase + pDirectory->VirtualAddress);
	for ( ; pImportDesc->Name; ++pImportDesc) {
        char* dllName = (char*)(fakeBase + pImportDesc->Name);

#ifndef OS_RING3
        KE_STACK_ATTACH_PROCESS(pDllHandle->pep, &apcState);
#endif // OS_RING3
		moduleBase = GET_MODULE_BASE(pDllHandle->pep/*hProcess*/, dllName);
		if (moduleBase == INVALID_MODULE_BASE) {
#ifndef OS_RING3
            KE_UNSTACK_DETACH_PROCESS(&apcState);
#endif // OS_RING3
#ifdef OS_RING3
            moduleBase = 
#endif // OS_RING3
                LOAD_LIBRARY(dllName);
#ifndef OS_RING3
            KE_STACK_ATTACH_PROCESS(pDllHandle->pep, &apcState);
#endif // OS_RING3
            moduleBase = GET_MODULE_BASE(pDllHandle->pep/*hProcess*/, dllName);
#ifndef OS_RING3
            KE_UNSTACK_DETACH_PROCESS(&apcState);
#endif // OS_RING3
            if (moduleBase == INVALID_MODULE_BASE) {
#ifndef OS_RING3
                KE_STACK_ATTACH_PROCESS(pDllHandle->pep, &apcState);
#endif // OS_RING3
			    goto exit;
            }
#ifndef OS_RING3
            KE_STACK_ATTACH_PROCESS(pDllHandle->pep, &apcState);
#endif // OS_RING3

		}
		thunkRef = (POINTER_TYPE*)(fakeBase + pImportDesc->OriginalFirstThunk);
		funcRef = (POINTER_TYPE*)(fakeBase + pImportDesc->FirstThunk);
		for ( ; *funcRef; funcRef++) {
			if (thunkRef && ((PIMAGE_THUNK_DATA)thunkRef)->u1.Ordinal & IMAGE_ORDINAL_FLAG32) {
				// get the VA of the modules NT Header
				dwExportDir = (unsigned char*)moduleBase + ((PIMAGE_DOS_HEADER)moduleBase)->e_lfanew;

				// dwNameArray = the address of the modules export directory entry
				dwNameArray = &((PIMAGE_NT_HEADERS32)dwExportDir)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

				// get the VA of the export directory
				dwExportDir = (unsigned char*)moduleBase + ((PIMAGE_DATA_DIRECTORY)dwNameArray)->VirtualAddress;

				// get the VA for the array of addresses
				dwAddressArray = (POINTER_TYPE*)((unsigned char*)moduleBase + ((PIMAGE_EXPORT_DIRECTORY)dwExportDir)->AddressOfFunctions);

				// use the import ordinal (- export ordinal base) as an index into the array of addresses
				dwAddressArray += ((IMAGE_ORDINAL32(((PIMAGE_THUNK_DATA)thunkRef)->u1.Ordinal) - ((PIMAGE_EXPORT_DIRECTORY)dwExportDir)->Base) * sizeof(UINT32));

				// patch in the address for this imported function
				*funcRef = (POINTER_TYPE)((unsigned char*)moduleBase + *dwAddressArray);
			}
			else {                
				PIMAGE_IMPORT_BY_NAME thunkData = (PIMAGE_IMPORT_BY_NAME)(fakeBase + *funcRef);
                *funcRef = (POINTER_TYPE)DLL_FIND_EXPORT_FUNCTION(pDllHandle, hProcess, 
#ifndef OS_RING3
                    &apcState,
#endif // OS_RING3
                    moduleBase, thunkData->Name);
			}
		}
#ifndef OS_RING3
        KE_UNSTACK_DETACH_PROCESS(&apcState);
#endif // OS_RING3
		if (thunkRef)
			thunkRef++;
	}

	// Обрабатываем релоки.
#ifdef OS_RING3
	locationDelta = (UINT32)((realBase == NULL ? fakeBase : realBase) - (unsigned char*)origNtHdrs->OptionalHeader.ImageBase);
#else
	locationDelta = (UINT32)(realBase - (unsigned char*)origNtHdrs->OptionalHeader.ImageBase);
#endif // OS_RING3
	pDirectory = GET_DIRECTORY_PTR(newNtHdrs, IMAGE_DIRECTORY_ENTRY_BASERELOC);

	if (pDirectory->Size > 0) {
		pReloc = (PIMAGE_BASE_RELOCATION)(fakeBase + pDirectory->VirtualAddress);
		for ( ; pReloc->SizeOfBlock != 0; ) {
			unsigned char* dest = fakeBase + pReloc->VirtualAddress;
			unsigned short* relInfo = (unsigned short *)((unsigned char *)pReloc + sizeof(IMAGE_BASE_RELOCATION));
			for (i = 0; i < ((pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / 2); i++, relInfo++) {
				// 12 битное смещение
				offset = *relInfo & 0xfff;
				type = *relInfo >> 12;

#ifdef _WIN64
				if (type == IMAGE_REL_BASED_DIR64) {
					*(ULONGLONG*)(dest + offset) += locationDelta;
				}
				else
#endif
				if (type == IMAGE_REL_BASED_HIGHLOW) {
					*(UINT32*)(dest + offset) += locationDelta;
				}
				else if (type == IMAGE_REL_BASED_HIGH) {
					*(UINT16*)(dest + offset) += HIWORD(locationDelta);
				}
				else if (type == IMAGE_REL_BASED_LOW) {
					*(UINT16*)(dest + offset) += LOWORD(locationDelta);
				}
			}

			// Переходим к следующей таблице с релоками.
			pReloc = (PIMAGE_BASE_RELOCATION)((char*)pReloc + pReloc->SizeOfBlock);
		}
	}

	pDllHandle->initialized = 1;
	return fakeBase;

exit:
	if (fakeBase != NULL) {
		SYS_DEALLOCATOR(fakeBase);
	}

	return NULL;
}
