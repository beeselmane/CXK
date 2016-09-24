#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>

#define CXSystemTable               EFI_SYSTEM_TABLE
#define CXKey                       EFI_INPUT_KEY
#define CXStatus                    EFI_STATUS
#define CXEFI                       EFIAPI
#define CXHandle                    EFI_HANDLE
#define CXGUID                      EFI_GUID
#define CXProtocol                  CXGUID
#define input                       IN

#define reset                       Reset
#define stdout                      ConOut
#define stdin                       ConIn
#define print                       OutputString
#define readKey                     ReadKeyStroke
#define handleProtocol              HandleProtocol
#define loadOptionsSize             LoadOptionsSize
#define loadOptions                 LoadOptions
#define strlen                      StrLen
#define getMemoryMap                GetMemoryMap
#define allocatePages               AllocatePages
#define allocate                    AllocatePool
#define free                        FreePool

#define CXInitializeEFILibrary      InitializeLib
#define bootServices                BootServices

#define UTF16Char                   CHAR16
#define UTF16String                 CHAR16 *

#define CXUTF16String(s)            L ## s
#define CXCheckIsError              EFI_ERROR

#define type                        Type
#define physicalBase                PhysicalStart
#define virtualBase                 VirtualStart
#define pagesCount                  NumberOfPages
#define attributes                  Attribute
#define printf                      Print

#define CXMemoryDescriptor          EFI_MEMORY_DESCRIPTOR

#define kCXNotReady                 EFI_NOT_READY
#define kCXSuccess                  EFI_SUCCESS

#define kCXPageSize                 4096

#define kCXAllocAnyPages            AllocateAnyPages
#define kCXAllocMaxAddress          AllocateMaxAddress
#define kCXAllocAtAddress           AllocateAddress

#define kCXMemoryTypeReserved       EfiReservedMemoryType
#define kCXMemoryTypeLoaderCode     EfiLoaderCode
#define kCXMemoryTypeLoaderData     EfiLoaderData
#define kCXMemoryTypeBootCode       EfiBootServicesCode
#define kCXMemoryTypeBootData       EfiBootServicesData
#define kCXMemoryTypeRuntimeCode    EfiRuntimeServicesCode
#define kCXMemoryTypeRuntimeData    EfiRuntimeServicesData
#define kCXMemoryTypeConventional   EfiConventionalMemory
#define kCXMemoryTypeUnusable       EfiUnusableMemory
#define kCXMemoryTypeACPIReclaimed  EfiACPIReclaimMemory
#define kCXMemoryTypeACPINVS        EfiACPIMemoryNVS
#define kCXMemoryTypeMappedIO       EfiMemoryMappedIO
#define kCXMemoryTypeMappedIOPorts  EfiMemoryMappedIOPortSpace
#define kCXMemoryTypePALCode        EfiPalCode
#define kCXMemorytypePersistent     EfiPersistentMemory
#define kCXMemoryTypeMaxMemory      EfiMaxMemoryType

#define CXMemoryType                EFI_MEMORY_TYPE
#define CXAddress                   EFI_PHYSICAL_ADDRESS

#define kCXLoadedImageProtocol      LOADED_IMAGE_PROTOCOL
#define CXLoadedImage               EFI_LOADED_IMAGE
#define CXDevicePathToString        ConvertDevicePathToText
#define filePath                    FilePath
#define deviceHandle                DeviceHandle

#define character                   UnicodeChar
#define exit                        Exit

#define CXFileHandle                EFI_FILE_HANDLE
#define CXVolumeHandle              EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
#define kCXVolumeProtocol           EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID
#define openRoot                    OpenVolume
#define open                        Open
#define kCXFileModeRead             EFI_FILE_MODE_READ
#define setOffset                   SetPosition
#define read                        Read
#define clear                       ClearScreen

#define disable                     ExitBootServices

typedef unsigned long long CXSize;
