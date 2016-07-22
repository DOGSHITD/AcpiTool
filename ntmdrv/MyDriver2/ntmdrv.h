#pragma once
//#ifndef _AMD64_
//#define _AMD64_
//#endif
#define DBG 1
#ifdef __cplusplus
extern "C"
{
#endif
#include <initguid.h>
#include <NTDDK.h>
#include <acpiioct.h>
#include <wdmguid.h>
#ifdef __cplusplus
}
#endif 
#if DBG

#define TRAP()                      DbgBreakPoint()

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()

#define DebugPrint(_x_)

#endif

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#define DEVICE_NAME L"\\Device\\ntdmDevice"
#define SYMBOLINK_NAME L"\\??\\ntdmDevice"
struct KERNEL_DEVICE_INFO
{
	LIST_ENTRY pList;
	PDEVICE_OBJECT pdo;
	PDEVICE_NOTIFY_CALLBACK pCallBack;
	PVOID pParam;
	ULONG ulHooked;
	WCHAR wszNameBuffer[260];
};//CONTAINING_RECORD
struct AP_DEVICE_INFO
{
	PDEVICE_OBJECT pdo;
	WCHAR wszDeviceName[260];
};
struct AP_DEVICE_LIST
{
	ULONG_PTR dwCount;
	AP_DEVICE_INFO list[1];
};
typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pDevice;
	UNICODE_STRING ustrDeviceName;	//设备名称
	UNICODE_STRING ustrSymLinkName;	//符号链接名
	PDEVICE_OBJECT LowerDevice;
	ACPI_INTERFACE_STANDARD		AcpiInterface;
	UCHAR ucGetAcpiInterfaceOk;
	UCHAR ucRegistered;
	UCHAR ucAttachOk;
	KERNEL_DEVICE_INFO *pDeviceNofityInfo;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
#define IOCTL_TDC3_INITIALIZE			CTL_CODE(FILE_DEVICE_ACPI,0xf16,METHOD_BUFFERED,FILE_ANY_ACCESS) 
#define IOCTL_TDC3_CALL_METHOD			CTL_CODE(FILE_DEVICE_ACPI,0xf17,METHOD_BUFFERED,FILE_ANY_ACCESS) 
#define IOCTL_TDC3_QUERY_ACPI_OBJECT		CTL_CODE(FILE_DEVICE_ACPI,0xf18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_QUERY_DEVICE_OBJECT		CTL_CODE(FILE_DEVICE_ACPI,0xf19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UPDATE_NOTIFY_LIST		CTL_CODE(FILE_DEVICE_ACPI,0xf20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_INITIALIZE_NOTIFY		CTL_CODE(FILE_DEVICE_ACPI,0xf21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_GET_NOTIFY_LIST			CTL_CODE(FILE_DEVICE_ACPI,0xf22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UNINITIALIZE_NOTIFY		CTL_CODE(FILE_DEVICE_ACPI,0xf26,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_TDC3_INITIALIZE_METHOD_EVENT			CTL_CODE(FILE_DEVICE_ACPI,0xf23,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_GET_METHOD_EVENT_DATA			CTL_CODE(FILE_DEVICE_ACPI,0xf24,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UNINITIALIZE_METHOD_EVENT		CTL_CODE(FILE_DEVICE_ACPI,0xf25,METHOD_BUFFERED,FILE_ANY_ACCESS)
struct UPDATE_NOTIFY_INFO
{
	PDEVICE_OBJECT PDO;
	ULONG ulAction;
};
struct NOTIFY_DATA
{
	PDEVICE_OBJECT pdo;
	ULONG ulNotifyValue;
};
struct NOTIFY_DATA_LIST
{
	ULONG_PTR dwCount;
	NOTIFY_DATA g_NotifyArray[1];
};
struct UI_METHOD_INPUT
{
	PDEVICE_OBJECT PDO;
	ULONG ulMethod;
	ULONG ulArgumentCount;
	ULONG ulSize;	
	ACPI_METHOD_ARGUMENT Argument[1];
};
struct ACPI_OBJECT
{
	//LONGLONG llSig;//"HNSOD\0\0\0"
	//PVOID pvMemBase;
	ACPI_OBJECT *pPrev;//Main Offset
	ACPI_OBJECT *pNext;
	ACPI_OBJECT *pParent;
	ACPI_OBJECT *pChild;
	ULONG ulObject;
};
struct ACPI_OBJECT_AP
{
	ACPI_OBJECT Data;
	ACPI_OBJECT *Tag;
};
///////////////////////////////////////////////////////
NTSTATUS CreateDevice (IN PDRIVER_OBJECT pDriverObject);
PVOID ReplaceACPIHandle(PVOID in_pdo,PVOID ParentScope);
NTSTATUS EvaluateACPIMethod( PDEVICE_OBJECT   Pdo,ULONG MethodName,ULONG ArgumentCount,PVOID in_Buffer,ULONG in_iBufferSize,PVOID out_Buffer,ULONG in_oBufferSize,ULONG_PTR *out_ulRets);
ULONG GetAcpiObjectCount(ACPI_OBJECT* in_pObject,ACPI_OBJECT_AP* in_pvBuffer,ULONG in_ulSize);
ACPI_OBJECT *GetAcpiObjectRoot(ACPI_OBJECT* in_pObject);
ULONG GetDriverDeviceCount(PDRIVER_OBJECT pdrv);
VOID ntdmDrvUnload (IN PDRIVER_OBJECT pDriverObject);
PVOID GetAcpiHandle(PVOID in_pdo);
VOID ASUSACPIDeviceNotifyHandler(PVOID Context,ULONG NotifyValue);
VOID ntdmDrvIrpCancelRoutine(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS GetAcpiInterfaces (  PDEVICE_OBJECT Pdo, PACPI_INTERFACE_STANDARD PAcpiStruct);
NTSTATUS ntdmDrvDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp);

struct LINK_NODE
{
	PDEVICE_OBJECT pdo;
	UCHAR ucFlag;
	LINK_NODE *pNext;
};


typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union {
        struct {
            ULONG TimeDateStamp;
        };
        struct {
            PVOID LoadedImports;
        };
    };
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;
struct PENDING_IRP_INFO
{
	LIST_ENTRY list;
	PIRP irp;
};

struct METHOD_PATH
{
	ULONG ulCount;
	ULONG ulObjects[1];
};
struct HOOKED_DATA
{
	LIST_ENTRY list;
	KIRQL irql;
	UCHAR ucDealed;
	UCHAR a;
	UCHAR b;
	HANDLE hThread;
	ULONG ulAddr;
	PDEVICE_OBJECT DeviceObject;
	PIRP irp;
	TIME_FIELDS time_in;//ExSystemTimeToLocalTime
	ULONG ulPathSize;
	ULONG ulInputSize;
	ULONG ulOutputSize;
	ULONG ulControlCode;
	/////////////////////
	ULONG_PTR ulInformation;
	ULONG ulStatus;
	TIME_FIELDS time_out;
	WCHAR wszDesc[260];
	WCHAR wszHid[260];
	//DWORD *MethodPath;
	//UCHAR *InputData;
	//UCHAR *OutputData;
};
///////////////////////////////////////////////////////////
void __cdecl  SystemMtThread(PVOID pvContext);
void DealTrace(PDEVICE_OBJECT in_pdo,PIRP in_irp,UCHAR ucIsInput);
UCHAR CreateTraceData();
UCHAR DeleteTraceData();
VOID AcpiHalHook();
VOID AcpiHalUnHook();

//typedef struct _IRP {
//    CSHORT Type;//offset=0,size=2
//    USHORT Size;//offset=2,size=2
//
//    //
//    // Define the common fields used to control the IRP.
//    //
//
//    //
//    // Define a pointer to the Memory Descriptor List (MDL) for this I/O
//    // request.  This field is only used if the I/O is "direct I/O".
//    //
//
//    PMDL MdlAddress;//offset=4,size=4
//
//    //
//    // Flags word - used to remember various flags.
//    //
//
//    ULONG Flags;//offset=8,size=4
//
//    //
//    // The following union is used for one of three purposes:
//    //
//    //    1. This IRP is an associated IRP.  The field is a pointer to a master
//    //       IRP.
//    //
//    //    2. This is the master IRP.  The field is the count of the number of
//    //       IRPs which must complete (associated IRPs) before the master can
//    //       complete.
//    //
//    //    3. This operation is being buffered and the field is the address of
//    //       the system space buffer.
//    //
//
//    union {
//        struct _IRP *MasterIrp;
//        LONG IrpCount;
//        PVOID SystemBuffer;
//    } AssociatedIrp;//offset=c,size=4
//
//    //
//    // Thread list entry - allows queueing the IRP to the thread pending I/O
//    // request packet list.
//    //
//
//    LIST_ENTRY ThreadListEntry;//offset=10,size=8
//
//    //
//    // I/O status - final status of operation.
//    //
//
//    IO_STATUS_BLOCK IoStatus;//offset=18,size=c
//
//    //
//    // Requestor mode - mode of the original requestor of this operation.
//    //
//
//    KPROCESSOR_MODE RequestorMode;//offset=24,size=1
//
//    //
//    // Pending returned - TRUE if pending was initially returned as the
//    // status for this packet.
//    //
//
//    BOOLEAN PendingReturned;//offset=25,size=1
//
//    //
//    // Stack state information.
//    //
//
//    CHAR StackCount;//offset=26,size=1
//    CHAR CurrentLocation;//offset=27,size=1
//
//    //
//    // Cancel - packet has been canceled.
//    //
//
//    BOOLEAN Cancel;//offset=28,size=1
//
//    //
//    // Cancel Irql - Irql at which the cancel spinlock was acquired.
//    //
//
//    KIRQL CancelIrql;//offset=29,size=1
//
//    //
//    // ApcEnvironment - Used to save the APC environment at the time that the
//    // packet was initialized.
//    //
//
//    CCHAR ApcEnvironment;//offset=2a,size=1
//
//    //
//    // Allocation control flags.
//    //
//
//    UCHAR AllocationFlags;//offset=2b,size=1
//
//    //
//    // User parameters.
//    //
//
//    PIO_STATUS_BLOCK UserIosb;//offset=2c,size=4
//    PKEVENT UserEvent;
//    union {
//        struct {
//            PIO_APC_ROUTINE UserApcRoutine;
//            PVOID UserApcContext;
//        } AsynchronousParameters;
//        LARGE_INTEGER AllocationSize;
//    } Overlay;//offset=30,size=8
//
//    //
//    // CancelRoutine - Used to contain the address of a cancel routine supplied
//    // by a device driver when the IRP is in a cancelable state.
//    //
//
//    PDRIVER_CANCEL CancelRoutine;//offset=38,size=4
//
//    //
//    // Note that the UserBuffer parameter is outside of the stack so that I/O
//    // completion can copy data back into the user's address space without
//    // having to know exactly which service was being invoked.  The length
//    // of the copy is stored in the second half of the I/O status block. If
//    // the UserBuffer field is NULL, then no copy is performed.
//    //
//
//    PVOID UserBuffer;//offset=3c,size=4
//
//    //
//    // Kernel structures
//    //
//    // The following section contains kernel structures which the IRP needs
//    // in order to place various work information in kernel controller system
//    // queues.  Because the size and alignment cannot be controlled, they are
//    // placed here at the end so they just hang off and do not affect the
//    // alignment of other fields in the IRP.
//    //
//
//    union {
//
//        struct {
//
//            union {
//
//                //
//                // DeviceQueueEntry - The device queue entry field is used to
//                // queue the IRP to the device driver device queue.
//                //
//
//                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;
//
//                struct {
//
//                    //
//                    // The following are available to the driver to use in
//                    // whatever manner is desired, while the driver owns the
//                    // packet.
//                    //
//
//                    PVOID DriverContext[4];
//
//                } ;//offset=40,size=10
//
//            } ;
//
//            //
//            // Thread - pointer to caller's Thread Control Block.
//            //
//
//            PETHREAD Thread;//offset=50,size=4
//
//            //
//            // Auxiliary buffer - pointer to any auxiliary buffer that is
//            // required to pass information to a driver that is not contained
//            // in a normal buffer.
//            //
//
//            PCHAR AuxiliaryBuffer;//offset=54,size=4
//
//            //
//            // The following unnamed structure must be exactly identical
//            // to the unnamed structure used in the minipacket header used
//            // for completion queue entries.
//            //
//
//            struct {
//
//                //
//                // List entry - used to queue the packet to completion queue, among
//                // others.
//                //
//
//                LIST_ENTRY ListEntry;//offset=58,size=8
//
//                union {
//
//                    //
//                    // Current stack location - contains a pointer to the current
//                    // IO_STACK_LOCATION structure in the IRP stack.  This field
//                    // should never be directly accessed by drivers.  They should
//                    // use the standard functions.
//                    //
//
//                    struct _IO_STACK_LOCATION *CurrentStackLocation;//offset=60,size=4
//
//                    //
//                    // Minipacket type.
//                    //
//
//                    ULONG PacketType;//offset=60,size=4
//                };
//            };
//
//            //
//            // Original file object - pointer to the original file object
//            // that was used to open the file.  This field is owned by the
//            // I/O system and should not be used by any other drivers.
//            //
//
//            PFILE_OBJECT OriginalFileObject;//offset=64,size=4
//
//        } Overlay;
//
//        //
//        // APC - This APC control block is used for the special kernel APC as
//        // well as for the caller's APC, if one was specified in the original
//        // argument list.  If so, then the APC is reused for the normal APC for
//        // whatever mode the caller was in and the "special" routine that is
//        // invoked before the APC gets control simply deallocates the IRP.
//        //
//
//        KAPC Apc;//offset=68,size=30-1?
//
//        //
//        // CompletionKey - This is the key that is used to distinguish
//        // individual I/O operations initiated on a single file handle.
//        //
//
//        PVOID CompletionKey;//offset=98,size=4-1?
//
//    } Tail;
//
//} IRP, *PIRP;
//typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _DEVICE_OBJECT {
//    CSHORT Type;//offset=0,size=2
//    USHORT Size;//offset=2,size=2
//    LONG ReferenceCount;//offset=4,size=4
//    struct _DRIVER_OBJECT *DriverObject;//offset=8,size=4
//    struct _DEVICE_OBJECT *NextDevice;//offset=c,size=4
//    struct _DEVICE_OBJECT *AttachedDevice;//offset=10,size=4
//    struct _IRP *CurrentIrp;//offset=14,size=4
//    PIO_TIMER Timer;//offset=18,size=4
//    ULONG Flags;       //offset=1c,size=4                         // See above:  DO_...
//    ULONG Characteristics;     //offset=20,size=4                 // See ntioapi:  FILE_...
//    PVPB Vpb;			//offset=24,size=4
//    PVOID DeviceExtension;	//offset=28,size=4
//    DEVICE_TYPE DeviceType;	//offset=2c,size=4
//    CCHAR StackSize;
//    union {
//        LIST_ENTRY ListEntry;
//        WAIT_CONTEXT_BLOCK Wcb;
//    } Queue;			//offset=30,size=18+1?
//    ULONG AlignmentRequirement;//offset=48,size=4+1?
//    KDEVICE_QUEUE DeviceQueue;//offset=4c,size=14+2?
//    KDPC Dpc;//offset=60,size=8+2?
//
//    //
//    //  The following field is for exclusive use by the filesystem to keep
//    //  track of the number of Fsp threads currently using the device
//    //
//
//    ULONG ActiveThreadCount;//offset=68,size=4+2?
//    PSECURITY_DESCRIPTOR SecurityDescriptor;//offset=6c,size=4+2?
//    KEVENT DeviceLock;//offset=70,size=10+2?
//
//    USHORT SectorSize;//offset=80,size=2+2?
//    USHORT Spare1;//offset=82,size=2+2?
//
//    struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;//offset=84,size=4+2?
//    PVOID  Reserved;//offset=88,size=4+2?
//} DEVICE_OBJECT;


NTSTATUS
SendDownStreamIrp(
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            Ioctl,
    IN PVOID            InputBuffer,
    IN ULONG            InputSize,
    IN PVOID            OutputBuffer,
    IN ULONG            OutputSize
);

#define IO_STACK_BUFFER_MAX_SIZE 10