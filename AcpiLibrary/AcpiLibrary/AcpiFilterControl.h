#pragma once
#include <Windows.h>
#include <atlstr.h>
#include "DriverManager.h"
#define FILE_DEVICE_ACPI                0x00000032
#define IOCTL_TDC3_INITIALIZE				CTL_CODE(FILE_DEVICE_ACPI,0xf16,METHOD_BUFFERED,FILE_ANY_ACCESS) 
#define IOCTL_TDC3_CALL_METHOD				CTL_CODE(FILE_DEVICE_ACPI,0xf17,METHOD_BUFFERED,FILE_ANY_ACCESS) 
#define IOCTL_TDC3_QUERY_ACPI_OBJECT		CTL_CODE(FILE_DEVICE_ACPI,0xf18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_QUERY_DEVICE_OBJECT		CTL_CODE(FILE_DEVICE_ACPI,0xf19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UPDATE_NOTIFY_LIST		CTL_CODE(FILE_DEVICE_ACPI,0xf20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_INITIALIZE_NOTIFY		CTL_CODE(FILE_DEVICE_ACPI,0xf21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_GET_NOTIFY_LIST			CTL_CODE(FILE_DEVICE_ACPI,0xf22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UNINITIALIZE_NOTIFY		CTL_CODE(FILE_DEVICE_ACPI,0xf26,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_TDC3_INITIALIZE_METHOD_EVENT			CTL_CODE(FILE_DEVICE_ACPI,0xf23,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_GET_METHOD_EVENT_DATA			CTL_CODE(FILE_DEVICE_ACPI,0xf24,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_TDC3_UNINITIALIZE_METHOD_EVENT		CTL_CODE(FILE_DEVICE_ACPI,0xf25,METHOD_BUFFERED,FILE_ANY_ACCESS)


#define ACPI_EVAL_INPUT_BUFFER_SIGNATURE                    'BieA'
#define ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE     'IieA'
#define ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE      'SieA'
#define ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE            'CieA'
#define ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE                   'BoeA'
#define METHOD_ARGUMENT_TYPE_INTEGER                        0x0
#define METHOD_ARGUMENT_TYPE_STRING                         0x1
#define METHOD_ARGUMENT_TYPE_BUFFER                         0x2
#define METHOD_ARGUMENT_TYPE_PACKAGE                        0x3
typedef struct  ACPI_METHOD_ARGUMENT {
	USHORT      Type;
	USHORT      DataLength;
	union {
		ULONG   Argument;
		UCHAR   Data[ANYSIZE_ARRAY];
	};
}  ACPI_METHOD_ARGUMENT, *PACPI_METHOD_ARGUMENT;
typedef struct _ACPI_EVAL_INPUT_BUFFER {
	ULONG       Signature;
	union {
		UCHAR   MethodName[4];
		ULONG   MethodNameAsUlong;
	};
} ACPI_EVAL_INPUT_BUFFER, *PACPI_EVAL_INPUT_BUFFER;

typedef struct _ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER {
	ULONG       Signature;
	union {
		UCHAR   MethodName[4];
		ULONG   MethodNameAsUlong;
	};
	ULONG       IntegerArgument;
} ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER, *PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER;

typedef struct _ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING {
	ULONG       Signature;
	union {
		UCHAR   MethodName[4];
		ULONG   MethodNameAsUlong;
	};
	ULONG       StringLength;
	UCHAR       String[ANYSIZE_ARRAY];
} ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING, *PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING;
typedef struct _ACPI_EVAL_INPUT_BUFFER_COMPLEX {
	ULONG                   Signature;
	union {
		UCHAR               MethodName[4];
		ULONG               MethodNameAsUlong;
	};
	ULONG                   Size;
	ULONG                   ArgumentCount;
	ACPI_METHOD_ARGUMENT    Argument[ANYSIZE_ARRAY];
} ACPI_EVAL_INPUT_BUFFER_COMPLEX, *PACPI_EVAL_INPUT_BUFFER_COMPLEX;
//////////////////////////
typedef struct _ACPI_EVAL_OUTPUT_BUFFER {
	ULONG                   Signature;
	ULONG                   Length;
	ULONG                   Count;
	ACPI_METHOD_ARGUMENT    Argument[ANYSIZE_ARRAY];
}  ACPI_EVAL_OUTPUT_BUFFER, *PACPI_EVAL_OUTPUT_BUFFER;
BOOL EnumACPIDeviceName(TCHAR *tszBuffer,DWORD dwSize,DWORD index);
void getACPIDeviceName(TCHAR *tszBuffer,DWORD dwSize);
void AnalyzeAcpiBuffer(CString& in_csTemp,PVOID in_pvBuffer);
struct DEVICE_LIST_ITEM
{
	PVOID pdo;
	WCHAR wszDeviceName[260];
};
struct KERNEL_DEVICE_LIST
{
	ULONG_PTR dwCount;  
	DEVICE_LIST_ITEM list[1];
};

typedef struct  METHOD_OUTPUT_BUFFER
{
	ULONG                   Signature;
	ULONG                   Length;
	ULONG                   Count;
	ACPI_METHOD_ARGUMENT    Argument[1];    
} METHOD_OUTPUT_BUFFER;
struct UI_METHOD_INPUT
{
	PVOID PDO;
	ULONG ulMethod;
	ULONG ulArgumentCount;
	ULONG ulSize;
	ACPI_METHOD_ARGUMENT Argument[1];
};

struct BUFFER_VALUE
{
	union{
		DWORD_PTR Number;
		BYTE Datas[sizeof(DWORD_PTR)];
	};
};
struct ACPI_OBJECT
{
	//LONGLONG llSig;//"HNSOD\0\0\0"
	//PVOID pvMemBase;
	PVOID Prev;//Main Offset
	PVOID Next;
	PVOID Parent;
	PVOID Child;
	ULONG ulObject;
};
struct ACPI_OBJECT_AP
{
	ACPI_OBJECT Data;
	ACPI_OBJECT *Tag;
};

struct NOTIFY_DATA
{
	PVOID pdo;
	ULONG ulNotifyValue;
};
struct NOTIFY_RET_DATA
{
	ULONG_PTR dwCount;
	NOTIFY_DATA g_NotifyArray[256];
};
struct UPDATE_NOTIFY_INFO
{
	PVOID PDO;
	ULONG ulAction;
};
typedef struct _TIME_FIELDS {
	SHORT Year;        // range [1601...]
	SHORT Month;       // range [1..12]
	SHORT Day;         // range [1..31]
	SHORT Hour;        // range [0..23]
	SHORT Minute;      // range [0..59]
	SHORT Second;      // range [0..59]
	SHORT Milliseconds;// range [0..999]
	SHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
namespace AcpiLibrary
{
	public enum class ARGUMENT_TYPE :USHORT
	{
		UNKNOWN = -1,
		INT = 0,
		STRING,
		BUFFER,
		PACKAGE
	};
	typedef public ref struct  METHOD_ARGUMENT {
		ARGUMENT_TYPE			Type;
		System::String			^Argument;	
	}  METHOD_ARGUMENT;

	public ref class MethodNode
	{
	public:
		System::String ^methodName;
		LONG methodIndex;
		LONG next;
		LONG parent;
		LONG child;
	};
	public ref struct DeviceNode
	{
		ULONG index;
		System::String ^devicename;
	};
	public ref class MethodEventArgs : System::EventArgs
	{
	public:
		MethodEventArgs();
		System::String ^methodPath;
		System::Collections::Generic::Dictionary<System::String^,System::String^> ^methodInformation;
	};
	public delegate void MethodEventHandler(System::Object^ sender, MethodEventArgs^ e);
	public ref class NotifyEventArgs : System::EventArgs
	{
	public:
		NotifyEventArgs();
		ULONG deviceIndex;
		ULONG notifyInformation;
	};
	public delegate void NotifyEventHandler(System::Object^ sender, NotifyEventArgs^ e);
	public ref class AcpiFilterControl
	{
	public:
		AcpiFilterControl(void);
		~AcpiFilterControl();
		BOOL Initialize();
		BOOL CallMethod(ULONG methodIndex, System::String ^methodName, array<METHOD_ARGUMENT^> ^inputArg, array<METHOD_ARGUMENT^>^ %outputArg);
		BOOL GetMethodTree(array<MethodNode^>^ %methodTree);
		LONG GetMethodIndex(System::String ^methodName);
		LONG GetRootIndex();
		LONG GetDeviceList(array<DeviceNode^>^ %deviceList);
		System::Boolean StartMethodMonitor();
		System::Boolean StartNotifyMonitor();
		void StopNotifyMonitor();
		void StopMethodMonitor();
		event MethodEventHandler ^Method;
		event NotifyEventHandler ^Notify;
		System::Boolean setDeviceChecked(ULONG index,System::Boolean isSelected);
	private:
		static ULONG NameToUlong(System::String ^name);
		static BOOL UlongToName(ULONG ulName,System::String ^%name);
		LONG GetMethodIndex(ACPI_OBJECT *tag);
		LONG GetDeviceIndex(PVOID pdo);
		HANDLE hAcpiFilterDevice;
		ACPI_OBJECT_AP* methodList;
		ULONG methodNum;
		KERNEL_DEVICE_LIST *kdList;
		HANDLE hMethodEvent;
		HANDLE hNotifyEvent;
		HANDLE hMethodThread;
		HANDLE hNotifyThread;
		//LPTHREAD_START_ROUTINE NotifyThreadProc;
		static void NotifyThreadProc(System::Object ^param);
		static void MethodThreadProc(System::Object ^param);
		BOOL isNotifyMonitorRunning;
		BOOL isMethodMonitorRunning;
		DriverManager ^driverManager;
		BOOL CreateAcpiFilterDevice(void);
	};
}