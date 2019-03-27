#ifndef __SHARED_PE_H_
#define __SHARED_PE_H_

#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_OS2_SIGNATURE                 0x454E      // NE
#define IMAGE_OS2_SIGNATURE_LE              0x454C      // LE
#define IMAGE_VXD_SIGNATURE                 0x454C      // LE
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#pragma pack(push,2)
typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
	uint16_t   e_magic;                     // Magic number
	uint16_t   e_cblp;                      // UINT8s on last page of file
	uint16_t   e_cp;                        // Pages in file
	uint16_t   e_crlc;                      // Relocations
	uint16_t   e_cparhdr;                   // Size of header in paragraphs
	uint16_t   e_minalloc;                  // Minimum extra paragraphs needed
	uint16_t   e_maxalloc;                  // Maximum extra paragraphs needed
	uint16_t   e_ss;                        // Initial (relative) SS value
	uint16_t   e_sp;                        // Initial SP value
	uint16_t   e_csum;                      // Checksum
	uint16_t   e_ip;                        // Initial IP value
	uint16_t   e_cs;                        // Initial (relative) CS value
	uint16_t   e_lfarlc;                    // File address of relocation table
	uint16_t   e_ovno;                      // Overlay number
	uint16_t   e_res[4];                    // Reserved UINT16s
	uint16_t   e_oemid;                     // OEM identifier (for e_oeminfo)
	uint16_t   e_oeminfo;                   // OEM information; e_oemid specific
	uint16_t   e_res2[10];                  // Reserved UINT16s
	long_t   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;
#pragma pack(pop)

typedef struct _IMAGE_FILE_HEADER {
	uint16_t    Machine;
	uint16_t    NumberOfSections;
	uint32_t   TimeDateStamp;
	uint32_t   PointerToSymbolTable;
	uint32_t   NumberOfSymbols;
	uint16_t    SizeOfOptionalHeader;
	uint16_t    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP            0x01a3
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5               0x01a8  // SH5
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_AM33              0x01d3
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP         0x01f1
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE           0x0520  // Infineon
#define IMAGE_FILE_MACHINE_CEF               0x0CEF
#define IMAGE_FILE_MACHINE_EBC               0x0EBC  // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R              0x9041  // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE               0xC0EE

typedef struct _IMAGE_DATA_DIRECTORY {
	uint32_t VirtualAddress;
	uint32_t Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	uint32_t   Characteristics;
	uint32_t   TimeDateStamp;
	uint16_t    MajorVersion;
	uint16_t    MinorVersion;
	uint32_t   Name;
	uint32_t   Base;
	uint32_t   NumberOfFunctions;
	uint32_t   NumberOfNames;
	uint32_t   AddressOfFunctions;     // RVA from base of image
	uint32_t   AddressOfNames;         // RVA from base of image
	uint32_t   AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

// Import Format
typedef struct _IMAGE_IMPORT_BY_NAME {
	uint16_t    Hint;
	uint8_t    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

#pragma pack(push,8)
typedef struct _IMAGE_THUNK_DATA64 {
	union {
		uint64_t ForwarderString;  // PBYTE 
		uint64_t Function;         // PUINT32
		uint64_t Ordinal;
		uint64_t AddressOfData;    // PIMAGE_IMPORT_BY_NAME
	} u1;
} IMAGE_THUNK_DATA64;
typedef IMAGE_THUNK_DATA64 * PIMAGE_THUNK_DATA64;
#pragma pack(pop)

typedef struct _IMAGE_THUNK_DATA32 {
	union {
		uint32_t ForwarderString;      // PBYTE 
		uint32_t Function;             // PUINT32
		uint32_t Ordinal;
		uint32_t AddressOfData;        // PIMAGE_IMPORT_BY_NAME
	} u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;

#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL64(Ordinal) (Ordinal & 0xffff)
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)
#define IMAGE_SNAP_BY_ORDINAL64(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG64) != 0)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) ((Ordinal & IMAGE_ORDINAL_FLAG32) != 0)

// Thread Local Storage
//typedef VOID (*PIMAGE_TLS_CALLBACK)(pvoid_t DllHandle, uint32_t Reason, pvoid_t Reserved);

typedef struct _IMAGE_TLS_DIRECTORY64 {
	uint64_t   StartAddressOfRawData;
	uint64_t   EndAddressOfRawData;
	uint64_t   AddressOfIndex;         // PUINT32
	uint64_t   AddressOfCallBacks;     // PIMAGE_TLS_CALLBACK *;
	uint32_t   SizeOfZeroFill;
	uint32_t   Characteristics;
} IMAGE_TLS_DIRECTORY64;
typedef IMAGE_TLS_DIRECTORY64 * PIMAGE_TLS_DIRECTORY64;

typedef struct _IMAGE_TLS_DIRECTORY32 {
	uint32_t   StartAddressOfRawData;
	uint32_t   EndAddressOfRawData;
	uint32_t   AddressOfIndex;             // PUINT32
	uint32_t   AddressOfCallBacks;         // PIMAGE_TLS_CALLBACK *
	uint32_t   SizeOfZeroFill;
	uint32_t   Characteristics;
} IMAGE_TLS_DIRECTORY32;
typedef IMAGE_TLS_DIRECTORY32 * PIMAGE_TLS_DIRECTORY32;

#ifdef _WIN64
#define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG64
#define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL64(Ordinal)
typedef IMAGE_THUNK_DATA64              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA64             PIMAGE_THUNK_DATA;
#define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL64(Ordinal)
typedef IMAGE_TLS_DIRECTORY64           IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY64          PIMAGE_TLS_DIRECTORY;
#else
#define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG32
#define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL32(Ordinal)
typedef IMAGE_THUNK_DATA32              IMAGE_THUNK_DATA;
typedef PIMAGE_THUNK_DATA32             PIMAGE_THUNK_DATA;
#define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL32(Ordinal)
typedef IMAGE_TLS_DIRECTORY32           IMAGE_TLS_DIRECTORY;
typedef PIMAGE_TLS_DIRECTORY32          PIMAGE_TLS_DIRECTORY;
#endif

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		uint32_t   Characteristics;            // 0 for terminating null import descriptor
		uint32_t   OriginalFirstThunk;         // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
	};
	uint32_t   TimeDateStamp;                  // 0 if not bound,
	// -1 if bound, and real date\time stamp
	//     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
	// O.W. date/time stamp of DLL bound to (Old BIND)

	uint32_t   ForwarderChain;                 // -1 if no forwarders
	uint32_t   Name;
	uint32_t   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

typedef struct _IMAGE_OPTIONAL_HEADER
{
	// Standard fields.
	uint16_t    Magic;
	uint8_t    MajorLinkerVersion;
	uint8_t    MinorLinkerVersion;
	uint32_t   SizeOfCode;
	uint32_t   SizeOfInitializedData;
	uint32_t   SizeOfUninitializedData;
	uint32_t   AddressOfEntryPoint;
	uint32_t   BaseOfCode;
	uint32_t   BaseOfData;

	// NT additional fields.
	uint32_t   ImageBase;
	uint32_t   SectionAlignment;
	uint32_t   FileAlignment;
	uint16_t    MajorOperatingSystemVersion;
	uint16_t    MinorOperatingSystemVersion;
	uint16_t    MajorImageVersion;
	uint16_t    MinorImageVersion;
	uint16_t    MajorSubsystemVersion;
	uint16_t    MinorSubsystemVersion;
	uint32_t   Win32VersionValue;
	uint32_t   SizeOfImage;
	uint32_t   SizeOfHeaders;
	uint32_t   CheckSum;
	uint16_t    Subsystem;
	uint16_t    DllCharacteristics;
	uint32_t   SizeOfStackReserve;
	uint32_t   SizeOfStackCommit;
	uint32_t   SizeOfHeapReserve;
	uint32_t   SizeOfHeapCommit;
	uint32_t   LoaderFlags;
	uint32_t   NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64
{
	uint16_t        Magic;
	uint8_t        MajorLinkerVersion;
	uint8_t        MinorLinkerVersion;
	uint32_t       SizeOfCode;
	uint32_t       SizeOfInitializedData;
	uint32_t       SizeOfUninitializedData;
	uint32_t       AddressOfEntryPoint;
	uint32_t       BaseOfCode;
	uint64_t   ImageBase;
	uint32_t       SectionAlignment;
	uint32_t       FileAlignment;
	uint16_t        MajorOperatingSystemVersion;
	uint16_t        MinorOperatingSystemVersion;
	uint16_t        MajorImageVersion;
	uint16_t        MinorImageVersion;
	uint16_t        MajorSubsystemVersion;
	uint16_t        MinorSubsystemVersion;
	uint32_t       Win32VersionValue;
	uint32_t       SizeOfImage;
	uint32_t       SizeOfHeaders;
	uint32_t       CheckSum;
	uint16_t        Subsystem;
	uint16_t        DllCharacteristics;
	uint64_t   SizeOfStackReserve;
	uint64_t   SizeOfStackCommit;
	uint64_t   SizeOfHeapReserve;
	uint64_t   SizeOfHeapCommit;
	uint32_t       LoaderFlags;
	uint32_t       NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64
{
	uint32_t Signature;						// 4
	IMAGE_FILE_HEADER FileHeader;			// 20
	IMAGE_OPTIONAL_HEADER64 OptionalHeader;	// 112 + 16 * sizeof(IMAGE_DATA_DIRECTORY)
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS
{
	uint32_t Signature;							// 0x00 (  0)
	IMAGE_FILE_HEADER FileHeader;				// 0x04	(  4) 
	IMAGE_OPTIONAL_HEADER32 OptionalHeader;		// 0x18 ( 24)
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

#ifdef _AMD64_
typedef IMAGE_NT_HEADERS64 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32 IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
#endif

#ifdef _WIN64
typedef IMAGE_OPTIONAL_HEADER64             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR64_MAGIC
#else
typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR32_MAGIC
#endif


#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
	((ULONG_PTR)ntheader +                                              \
	FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
	((PIMAGE_NT_HEADERS)(ntheader))->FileHeader.SizeOfOptionalHeader   \
	))

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER
{
	uint8_t Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		ulong_t PhysicalAddress;
		ulong_t VirtualSize;
	} Misc;
	ulong_t VirtualAddress;
	ulong_t SizeOfRawData;
	ulong_t PointerToRawData;
	ulong_t PointerToRelocations;
	ulong_t PointerToLinenumbers;
	uint16_t NumberOfRelocations;
	uint16_t NumberOfLinenumbers;
	ulong_t Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define IMAGE_SIZEOF_SECTION_HEADER 40

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000
#define IMAGE_SCN_ALIGN_MASK                 0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

// Based relocation format.
typedef struct _IMAGE_BASE_RELOCATION {
	uint32_t   VirtualAddress;
	uint32_t   SizeOfBlock;
	//  WORD    TypeOffset[1];
} IMAGE_BASE_RELOCATION;
typedef IMAGE_BASE_RELOCATION UNALIGNED * PIMAGE_BASE_RELOCATION;

// Based relocation types.
#define IMAGE_REL_BASED_ABSOLUTE              0
#define IMAGE_REL_BASED_HIGH                  1
#define IMAGE_REL_BASED_LOW                   2
#define IMAGE_REL_BASED_HIGHLOW               3
#define IMAGE_REL_BASED_HIGHADJ               4
#define IMAGE_REL_BASED_MIPS_JMPADDR          5
#define IMAGE_REL_BASED_MIPS_JMPADDR16        9
#define IMAGE_REL_BASED_IA64_IMM64            9
#define IMAGE_REL_BASED_DIR64                 10


/*-----------------------------------------------------------------------------
  _IMAGE_SYMBOL - Object files may contain an array of COFF Symbols. This array
  is pointed to by the _IMAGE_FILE_HEADER->PointerToSymbolTable field and has
  _IMAGE_FILE_HEADER->NumberOfSymbols entries. There are no restrictions on 
  the exact location of this table within the object.

  Do note that there is a distinction between normal and auxiliary COFF Symbol
  records. Certain normal symbols may be followed by auxiliary symbol records.
  These auxiliary symbol records contain extra information about the symbol.

  Refer to Section 4.5, "Auxiliary Symbol Records", in the PE/COFF Spec v8 for
  additional information on auxiliary symbols.

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)              // Symbols, relocs and linenumbers are 2-byte packed
typedef struct _IMAGE_SYMBOL 
{
  // N can be either a name by itself or an offset into the string table
  union 
  {
    uint8_t    ShortName[8];
    struct 
    {
      uint32_t   Short;               // if 0, use LongName
      uint32_t   Long;                // offset into string table
    } Name;
    uint32_t   LongName[2];           // PBYTE [2]
  } N;
  uint32_t   Value;
  int16_t   SectionNumber;
  uint16_t    Type;
  uint8_t    StorageClass;
  uint8_t    NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef IMAGE_SYMBOL UNALIGNED *PIMAGE_SYMBOL;
#pragma pack(pop)

/*-----------------------------------------------------------------------------
  Symbol Section Numbers

  Normally, the PIMAGE_SYMBOL->SectionNumber field points to the COFF section
  in which the symbol is defined. These are the special values the
  SectionNumber can have.
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_UNDEFINED                             (int16_t)0      // Symbol is undefined or is common.
#define IMAGE_SYM_ABSOLUTE                              (int16_t)-1     // Symbol is an absolute value.
#define IMAGE_SYM_DEBUG                                 (int16_t)-2     // Symbol is a special debug item.
#define IMAGE_SYM_SECTION_MAX                           0xFEFF        // Values 0xFF00-0xFFFF are special
#define IMAGE_SYM_SECTION_MAX_EX                        0x7FFFFFFF    // = MAXLONG

/*-----------------------------------------------------------------------------
  Symbol Types
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_TYPE_NULL                             0x0000        // no type.
#define IMAGE_SYM_TYPE_VOID                             0x0001        //
#define IMAGE_SYM_TYPE_CHAR                             0x0002        // type character.
#define IMAGE_SYM_TYPE_SHORT                            0x0003        // type short integer.
#define IMAGE_SYM_TYPE_INT                              0x0004        //
#define IMAGE_SYM_TYPE_LONG                             0x0005        //
#define IMAGE_SYM_TYPE_FLOAT                            0x0006        //
#define IMAGE_SYM_TYPE_DOUBLE                           0x0007        //
#define IMAGE_SYM_TYPE_STRUCT                           0x0008        //
#define IMAGE_SYM_TYPE_UNION                            0x0009        //
#define IMAGE_SYM_TYPE_ENUM                             0x000A        // enumeration.
#define IMAGE_SYM_TYPE_MOE                              0x000B        // member of enumeration.
#define IMAGE_SYM_TYPE_BYTE                             0x000C        //
#define IMAGE_SYM_TYPE_WORD                             0x000D        //
#define IMAGE_SYM_TYPE_UINT                             0x000E        //
#define IMAGE_SYM_TYPE_DWORD                            0x000F        //
#define IMAGE_SYM_TYPE_PCODE                            0x8000        //

/*-----------------------------------------------------------------------------
  Symbol Derived Types
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_DTYPE_NULL                            0             // no derived type.
#define IMAGE_SYM_DTYPE_POINTER                         1             // pointer.
#define IMAGE_SYM_DTYPE_FUNCTION                        2             // function.
#define IMAGE_SYM_DTYPE_ARRAY                           3             // array.

/*-----------------------------------------------------------------------------
  Symbol Storage Classes
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_CLASS_END_OF_FUNCTION                 (uint8_t)-1
#define IMAGE_SYM_CLASS_NULL                            0x0000
#define IMAGE_SYM_CLASS_AUTOMATIC                       0x0001
#define IMAGE_SYM_CLASS_EXTERNAL                        0x0002
#define IMAGE_SYM_CLASS_STATIC                          0x0003
#define IMAGE_SYM_CLASS_REGISTER                        0x0004
#define IMAGE_SYM_CLASS_EXTERNAL_DEF                    0x0005
#define IMAGE_SYM_CLASS_LABEL                           0x0006
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL                 0x0007
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT                0x0008
#define IMAGE_SYM_CLASS_ARGUMENT                        0x0009
#define IMAGE_SYM_CLASS_STRUCT_TAG                      0x000A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION                 0x000B
#define IMAGE_SYM_CLASS_UNION_TAG                       0x000C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION                 0x000D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC                0x000E
#define IMAGE_SYM_CLASS_ENUM_TAG                        0x000F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM                  0x0010
#define IMAGE_SYM_CLASS_REGISTER_PARAM                  0x0011
#define IMAGE_SYM_CLASS_BIT_FIELD                       0x0012
#define IMAGE_SYM_CLASS_FAR_EXTERNAL                    0x0044
#define IMAGE_SYM_CLASS_BLOCK                           0x0064
#define IMAGE_SYM_CLASS_FUNCTION                        0x0065
#define IMAGE_SYM_CLASS_END_OF_STRUCT                   0x0066
#define IMAGE_SYM_CLASS_FILE                            0x0067
#define IMAGE_SYM_CLASS_SECTION                         0x0068
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL                   0x0069
#define IMAGE_SYM_CLASS_CLR_TOKEN                       0x006B

/*-----------------------------------------------------------------------------
  _IMAGE_RELOCATION - Each section in a COFF file may have a number of 
  relocations associated with it. The array of relocations is pointed to by
  the _IMAGE_SECTION_HEADER->PointerToRelocations field and contains
  _IMAGE_SECTION_HEADER->NumberOfRelocations entries.

  Note that Image files do not contain COFF relocations. If Image files contain
  position dependent code or data, the dynamic linker will adjust the code/data
  using the fixups (aka dynamic relocations) in the PE base relocation table.

  Present in:
  * Object files only
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)
typedef struct _IMAGE_RELOCATION 
{
  union 
  {
    uint32_t   VirtualAddress;
    uint32_t   RelocCount;            // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
  } DUMMYUNIONNAME;
  uint32_t   SymbolTableIndex;
  uint16_t    Type;
} IMAGE_RELOCATION;
typedef IMAGE_RELOCATION UNALIGNED *PIMAGE_RELOCATION;
#pragma pack(pop)

//
// I386 relocation types.
//
#define IMAGE_REL_I386_ABSOLUTE         0x0000  // Reference is absolute, no relocation is necessary
#define IMAGE_REL_I386_DIR16            0x0001  // Direct 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_REL16            0x0002  // PC-relative 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32            0x0006  // Direct 32-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32NB          0x0007  // Direct 32-bit reference to the symbols virtual address, base not included
#define IMAGE_REL_I386_SEG12            0x0009  // Direct 16-bit reference to the segment-selector bits of a 32-bit virtual address
#define IMAGE_REL_I386_SECTION          0x000A
#define IMAGE_REL_I386_SECREL           0x000B
#define IMAGE_REL_I386_TOKEN            0x000C  // clr token
#define IMAGE_REL_I386_SECREL7          0x000D  // 7 bit offset from base of section containing target
#define IMAGE_REL_I386_REL32            0x0014  // PC-relative 32-bit reference to the symbols virtual address


int FixHeaderCheckSum(char* szFileName);

typedef VOID (*PIMAGE_TLS_CALLBACK) (PVOID DllHandle, DWORD Reason, PVOID Reserved);

#endif // __SHARED_PE_H_
