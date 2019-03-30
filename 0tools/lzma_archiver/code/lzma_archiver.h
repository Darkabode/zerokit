#ifndef __LZMA_ARCH_H_
#define __LZMA_ARCH_H_

#pragma pack(push, 1)

typedef struct _lzma_arch_header
{
    unsigned int signature; // 0x79977997
    unsigned int numFiles; // Количество файлов в архиве.
    unsigned int totalSize; // Размер всего архива, включая заголовок.
} lzma_arch_header_t, *plzma_arch_header_t;

#pragma pack(pop)

/** Формат одного элемента архива:

 - имя элемента вплоть до завершающего символа '\0'.
 - 4 байты длины элемента.
 - 4 байта с флагами.
 - данные.
*/

#define LAF_ENTRY_COMPRESSED 0x00000001

#endif // __LZMA_ARCH_H_
