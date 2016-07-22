#include "stdafx.h"
#include "AcpiFilterControl.h"
#include <SetupAPI.h>
#pragma comment(lib,"setupapi.lib")

#include <vcclr.h>
#include <atlstr.h>
using namespace System;
using namespace AcpiLibrary;
using namespace System::Threading;
using namespace System::Collections::Generic;
AcpiFilterControl::AcpiFilterControl(void)
{
	hAcpiFilterDevice = NULL;
	kdList = NULL;
	driverManager = gcnew DriverManager();
	driverManager->LoadNTDriver();
}
AcpiFilterControl::~AcpiFilterControl()
{
	StopMethodMonitor();
	StopNotifyMonitor();
	if(hAcpiFilterDevice!= NULL)
	{
		CloseHandle(hAcpiFilterDevice);
	}
	driverManager->UnloadNTDriver();
	driverManager = nullptr;
}
BOOL AcpiFilterControl::CreateAcpiFilterDevice(void)
{
	BOOL status = TRUE;
	hAcpiFilterDevice=CreateFile(DEVCIE_NAME,FILE_ALL_ACCESS,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);//FILE_FLAG_OVERLAPPED
	if(hAcpiFilterDevice==INVALID_HANDLE_VALUE || hAcpiFilterDevice == NULL)
	{
		status = FALSE;
	}
	return status;
}
BOOL AcpiFilterControl::Initialize()
{	
	TCHAR tszTemp[MAX_PATH]={0};
	DWORD dwRet=0;
	ULONG dwCount=0;
	UINT index = 0;
	BOOL status = TRUE;
	//hAcpiFilterDevice=CreateFile(DEVCIE_NAME,FILE_ALL_ACCESS,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);//FILE_FLAG_OVERLAPPED
	if(CreateAcpiFilterDevice()==FALSE || hAcpiFilterDevice==INVALID_HANDLE_VALUE || hAcpiFilterDevice == NULL)
	{
		status = FALSE;
	}
	else
	{
		while((status = EnumACPIDeviceName(tszTemp,MAX_PATH,index)))
		{
			if(DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_INITIALIZE,tszTemp,MAX_PATH*sizeof(TCHAR),tszTemp,MAX_PATH*sizeof(TCHAR),&dwRet,0))
			{
				PVOID pvTemp=0;
				DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_QUERY_ACPI_OBJECT,0,0,&dwCount,sizeof(ULONG),&dwRet,0);
				if(dwCount)
				{
					methodNum=dwCount/sizeof(ACPI_OBJECT_AP);
					pvTemp=new BYTE[dwCount];
					if(pvTemp)
					{
						if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_QUERY_ACPI_OBJECT,0,0,pvTemp,dwCount,&dwRet,0))
						{
						}
						else
						{
							methodList = (ACPI_OBJECT_AP*)pvTemp;
							break;
						}
						delete []pvTemp;
					}
				}
			}
			index++;
		}
		if(status == FALSE)
		{
		}
	}
	return status;
}

BOOL AcpiFilterControl::CallMethod(ULONG methodIndex, String ^methodName, array<METHOD_ARGUMENT^> ^inputArg, array<METHOD_ARGUMENT^> ^ %outputArg)
{
	BOOL status = TRUE;
	DWORD dwRet;
	UCHAR* pIoputBuffer = (UCHAR *)new CHAR[4096];
	if(hAcpiFilterDevice==INVALID_HANDLE_VALUE || hAcpiFilterDevice == NULL)
	{
		status = FALSE;
	}
	else 
	{
		if((methodList+methodIndex)->Data.ulObject != NameToUlong(methodName))
		{
			status = FALSE;
		}
		else
		{
			UI_METHOD_INPUT* pInputBuffer = (UI_METHOD_INPUT*)pIoputBuffer;
			pInputBuffer->PDO = (methodList+methodIndex)->Data.Parent;
			pInputBuffer->ulMethod = NameToUlong(methodName);
			pInputBuffer->ulArgumentCount = inputArg->Length;
			pInputBuffer->ulSize=0;
			BUFFER_VALUE bufferValue={0};
			DWORD_PTR pdata=0;
			DWORD dwIndex=sizeof(UI_METHOD_INPUT)-sizeof(ACPI_METHOD_ARGUMENT);
			ACPI_METHOD_ARGUMENT *pArgument=0;
							CString str ;
				 str.Format( L"argument count 0x%0x",pInputBuffer->ulArgumentCount);
							//MessageBox(0,str,0,0);

			for(DWORD i=0;i<pInputBuffer->ulArgumentCount;i++)
			{
				CString csTemp;
				DWORD dwTemp;
				int pos;
				int index;
				//BYTE *temp = new BYTE[inputArg[i]->Argument->Length];
				if(inputArg[i])
				{
				pin_ptr <const wchar_t> temp = PtrToStringChars(inputArg[i]->Argument);
					pInputBuffer->Argument[i].Type=(UINT16)inputArg[i]->Type;
					switch(pInputBuffer->Argument[i].Type)
					{
					case METHOD_ARGUMENT_TYPE_INTEGER:
						{
						//MessageBox(0,temp,0,0);
						pInputBuffer->Argument[i].DataLength=sizeof(ULONG);
							
						swscanf_s( temp, L"%x",&pInputBuffer->Argument[i].Argument);
						CString str ;
				 str.Format( L"METHOD_ARGUMENT_TYPE_INTEGER 0x%0x",pInputBuffer->Argument[i].Argument);
						//MessageBox(0,str,0,0);
						}
						break;
					case METHOD_ARGUMENT_TYPE_STRING:
						if(sizeof(TCHAR)>1)
						{
							pInputBuffer->Argument[i].DataLength=WideCharToMultiByte(CP_ACP,0,(LPCWSTR)temp,inputArg[i]->Argument->Length,(LPSTR)pInputBuffer->Argument[i].Data,0,0,0);
							WideCharToMultiByte(CP_ACP,0,(LPCWSTR)temp,inputArg[i]->Argument->Length,(LPSTR)pInputBuffer->Argument[i].Data,pInputBuffer->Argument[i].DataLength,0,0);
						}else
						{
							pInputBuffer->Argument[i].DataLength=inputArg[i]->Argument->Length;
							memcpy_s((TCHAR*)pInputBuffer->Argument[i].Data,4096- sizeof(UI_METHOD_INPUT)+1,temp,inputArg[i]->Argument->Length);
						}
						break;
					case METHOD_ARGUMENT_TYPE_BUFFER:
					//case METHOD_ARGUMENT_TYPE_PACKAGE:
						
						if(inputArg[i]->Argument->Length % sizeof(ULONG) == 0)
						{
							pInputBuffer->Argument[i].DataLength=0;
							pos = 0;
							index= 0;
							while(pos < inputArg[i]->Argument->Length && pos > 0)
							{
								swscanf_s( temp + pos, L"%2x",&pInputBuffer->Argument[i].Data[index]);
								pos = inputArg[i]->Argument->IndexOf(L',',pos) + 1;
								pInputBuffer->Argument[i].DataLength += 1;
								index ++;
							}
						}
						break;
					default:break;
					}
					dwIndex+=sizeof(USHORT)*2;
					dwIndex+=pInputBuffer->Argument[i].DataLength;
				}
			}
			pInputBuffer->ulSize=dwIndex-sizeof(UI_METHOD_INPUT)+sizeof(ACPI_METHOD_ARGUMENT);

			status = DeviceIoControl(hAcpiFilterDevice, IOCTL_TDC3_CALL_METHOD, pIoputBuffer, dwIndex, pIoputBuffer, 4096, &dwRet, NULL);
			if(status == TRUE)
			{				//m_cRetValueEdt
				CString csValue;
				DWORD dwIndex=sizeof(ULONG)*3;
				METHOD_OUTPUT_BUFFER *pOutput=(METHOD_OUTPUT_BUFFER*)pIoputBuffer;
				CString csTemp ;
				wchar_t tszTemp[1000];
				//_stprintf_s(tszTemp,MAX_PATH,TEXT("AP Get Data %ld Bytes\n"),dwTemp);
				//::OutputDebugString(tszTemp);
				if(dwRet>sizeof(ACPI_METHOD_ARGUMENT))
				{

					//ACPI_METHOD_ARGUMENT *pArgument=0;
					outputArg = gcnew array<METHOD_ARGUMENT^>(pOutput->Count);
					for(UINT i=0;i<pOutput->Count;i++)
					{/*
						pArgument=(METHOD_ARGUMENT*)&g_bIOBuffer[dwIndex];*/
						//pin_ptr <const wchar_t> temp = PtrToStringChars(outputArg[i]->Argument);
						outputArg[i] = gcnew METHOD_ARGUMENT();
						outputArg[i]->Type = (ARGUMENT_TYPE)pOutput->Argument[i].Type;
						switch(pOutput->Argument[i].Type)
						{
						case METHOD_ARGUMENT_TYPE_INTEGER:
							swprintf_s(tszTemp,MAX_PATH,TEXT("0x%08lx"),pOutput->Argument[i].Argument);
							outputArg[i]->Argument = gcnew String(tszTemp);
							break;
						case METHOD_ARGUMENT_TYPE_STRING:
							memset(tszTemp,0,1000);
							MultiByteToWideChar(CP_UTF8,0,(LPCSTR)pOutput->Argument[i].Data,strlen((LPCSTR)pOutput->Argument[i].Data),tszTemp,MAX_PATH*sizeof(TCHAR));
							outputArg[i]->Argument = gcnew String(tszTemp);
							break;
						case METHOD_ARGUMENT_TYPE_BUFFER:
						case METHOD_ARGUMENT_TYPE_PACKAGE:
							for(int j=0;j<pOutput->Argument[i].DataLength;j++)
							{
								swprintf_s(tszTemp,MAX_PATH,TEXT("0x%02x"),pOutput->Argument[i].Data[j]);
								csTemp.Append(tszTemp);
								if(j<pOutput->Argument[i].DataLength-1)csTemp.Append(TEXT(","));
							}
							outputArg[i]->Argument = gcnew String(csTemp);
							break;
						default:break;
						}
						//m_cResultListCombo.AddString(csTemp.GetBuffer());
						//m_cResultListCombo.SetCurSel(0);
						dwIndex+=sizeof(USHORT)*2;
						dwIndex+=pOutput->Argument[i].DataLength;
						csValue.Append(csTemp);
					}
				}
			}
		}
	}
	return status;
}
BOOL AcpiFilterControl::GetMethodTree(array<MethodNode^>^ %methodTree)
{
	methodTree = gcnew array<MethodNode^>(methodNum);
	for (ULONG i = 0; i < methodNum; i++)
	{
		methodTree[i] = gcnew MethodNode();
		UlongToName(methodList[i].Data.ulObject, (methodTree[i]->methodName));
		methodTree[i]->methodIndex = i;
		methodTree[i]->parent = GetMethodIndex((ACPI_OBJECT*)methodList[i].Data.Parent);
		methodTree[i]->child = GetMethodIndex((ACPI_OBJECT*)methodList[i].Data.Child);
		methodTree[i]->next = GetMethodIndex((ACPI_OBJECT*)methodList[i].Data.Next);
	}
	return TRUE;
}
LONG AcpiFilterControl::GetMethodIndex(String ^methodName)
{
	ULONG ulName;
	ULONG index;
	if(methodName->Length != 4)
	{
		return -2;
	}
	ulName = NameToUlong(methodName);
	//return ulName;
	//CString temp;
	//temp.Format(L"ulName %ld, methodNum %ld",ulName,methodNum);
	//OutputDebugString(L"ulName %ld, methodNum %ld");
	for (index = 0; index < methodNum; index++)
	{
		if(methodList[index].Data.ulObject == ulName)
		{
			return index;
		}
	}
	return -1;
}
LONG AcpiFilterControl::GetRootIndex()
{
	ULONG index;
	for (index = 0; index < methodNum; index++)
	{
		if(methodList[index].Data.Parent == NULL)
		{
			return index;
		}
	}
	return -1;
}
LONG AcpiFilterControl::GetMethodIndex(ACPI_OBJECT *tag)
{
	ULONG index;
	for (index = 0; index < methodNum; index++)
	{
		if(methodList[index].Tag == tag)
		{
			return index;
		}
	}
	return -1;
}

LONG AcpiFilterControl::GetDeviceIndex(PVOID pdo)
{
	for (ULONG i = 0; i < kdList->dwCount; i++)
	{
		if(kdList->list[i].pdo == pdo)
		{
			return i;
		}
	}
	return -1;
}
LONG AcpiFilterControl::GetDeviceList(array<DeviceNode^>^ %deviceList)
{
	PVOID pvTemp=0;
	DWORD dwRet=0;
	ULONG dwCount=0;
	DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_QUERY_DEVICE_OBJECT,0,0,&dwCount,sizeof(ULONG),&dwRet,0);
	if(dwCount)
	{
		pvTemp=new BYTE[dwCount];
		if(pvTemp)
		{
			if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_QUERY_DEVICE_OBJECT,0,0,pvTemp,dwCount,&dwRet,0))
			{
							//_stprintf_s(tszTemp,MAX_PATH,TEXT("DeviceIoControl IOCTL_TDC3_GPE_INIT Failed EC=%ld\n"),GetLastError());
							//::MessageBox(0,tszTemp,0,0);
				delete []pvTemp;
				return -1;
			}
			else
			{
				kdList=(KERNEL_DEVICE_LIST*)pvTemp;
			}
		}
	}
	else
	{
		return -1;
	}
	if(kdList == NULL)
	{
		return -1;
	}
	deviceList = gcnew array<DeviceNode^>(kdList->dwCount);
	for (ULONG i = 0; i < kdList->dwCount; i++)
	{
		deviceList[i] = gcnew DeviceNode();
		deviceList[i]->index = i;
		deviceList[i]->devicename = gcnew String(kdList->list[i].wszDeviceName);
	}
	return kdList->dwCount;
}
Boolean AcpiFilterControl::StartMethodMonitor()
{
	Boolean status = true;
	DWORD dwRet;
	pin_ptr<HANDLE> pMethodEventHandle;
	
	if(hMethodEvent == NULL)
	hMethodEvent = CreateEvent(0,0,0,0);
	if(!hMethodEvent)
	{
		status == false;
	}
	else
	{
		pMethodEventHandle = &hMethodEvent;
		if(hAcpiFilterDevice==INVALID_HANDLE_VALUE || hAcpiFilterDevice == NULL)
		{
			status = false;
		}
		else 
		{
			DWORD dwData=0;
			if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_INITIALIZE_METHOD_EVENT,pMethodEventHandle,sizeof(HANDLE),&dwData,sizeof(DWORD),&dwRet,0))
			{
				 CString str ;
				 str.Format( L"%d",GetLastError());
				//MessageBox(NULL,str,L"",0);
				status = false;
			}
			else
			{
				Thread^ methodMonitorThread = gcnew Thread( gcnew ParameterizedThreadStart( &MethodThreadProc ) );
				if(!methodMonitorThread)
				{
			//MessageBox(NULL,L"2",L"",0);
					status = false;
				}
				else
				{
					methodMonitorThread->Start(this);
					isMethodMonitorRunning = TRUE;
				}
			}
		}
	}
	return status;
}
Boolean AcpiFilterControl::StartNotifyMonitor()
{
	DWORD dwData[10]={0},dwRet=0;
	pin_ptr<HANDLE> pNotifyEventHandle;
	Boolean status = true;
	hNotifyEvent = CreateEvent(0,0,0,0);
	if(!hNotifyEvent)
	{
		status = false;
	}
	else
	{
		pNotifyEventHandle = &hNotifyEvent;
		if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_INITIALIZE_NOTIFY,pNotifyEventHandle,sizeof(HANDLE),dwData,sizeof(DWORD),&dwRet,0))
		{
			status = false;
		}
		else
		{
			Thread^ notifyMonitorThread = gcnew Thread( gcnew ParameterizedThreadStart( &NotifyThreadProc ) );
			if(!notifyMonitorThread)
			{
				status = false;
			}
			else
			{
				notifyMonitorThread->Start(this);
				isNotifyMonitorRunning = TRUE;
			}
		}
	}
	return status;
}
void AcpiFilterControl::StopMethodMonitor()
{
	DWORD ret;
	isMethodMonitorRunning = FALSE;
	if(hAcpiFilterDevice!=INVALID_HANDLE_VALUE && hAcpiFilterDevice != NULL)
	{
		if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_UNINITIALIZE_METHOD_EVENT,NULL,0,NULL,0,&ret,0))
		{
		}
	}
	SetEvent(hMethodEvent);
}
void AcpiFilterControl::StopNotifyMonitor()
{
	DWORD ret;
	isNotifyMonitorRunning = FALSE;
	if(hAcpiFilterDevice!=INVALID_HANDLE_VALUE && hAcpiFilterDevice != NULL)
	{
		if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_UNINITIALIZE_NOTIFY,NULL,0,NULL,0,&ret,0))
		{
		}
	}
	SetEvent(hNotifyEvent);
}

Boolean AcpiFilterControl::setDeviceChecked(ULONG index,Boolean isSelected)
{
	Boolean status;
	if(hAcpiFilterDevice==INVALID_HANDLE_VALUE|| hAcpiFilterDevice==NULL)
	{
		status = false;
	}
	else
	{
		if(!kdList || kdList->dwCount <= index)
		{
			status = false;
		}
		else
		{
			UPDATE_NOTIFY_INFO Data={0};
			DWORD dwRet=0;
			Data.PDO=kdList->list[index].pdo;
			Data.ulAction=isSelected;
			if(!DeviceIoControl(hAcpiFilterDevice,IOCTL_TDC3_UPDATE_NOTIFY_LIST,&Data,sizeof(UPDATE_NOTIFY_INFO),&Data,sizeof(UPDATE_NOTIFY_INFO),&dwRet,0))
			{
				status = false;
			}
		}
	}
	return status;
}


MethodEventArgs::MethodEventArgs()
{
	methodInformation = gcnew Dictionary<String^,String^>();
}
NotifyEventArgs::NotifyEventArgs()
{
}




ULONG AcpiFilterControl::NameToUlong(String ^name)
{
	CHAR temp[5];
	ULONG ret = 0;
	if(name->Length > 4)
	{
		return 0;
	}
	else
	{
		pin_ptr<const wchar_t> wch = PtrToStringChars(name);
		WideCharToMultiByte(CP_ACP,0,wch,4,temp,5,0,0);
		memcpy_s(&ret,4,temp,4);
	}
	return ret;
}
BOOL AcpiFilterControl::UlongToName(ULONG ulName,System::String ^%name)
{
	CHAR temp[5] = {0};
	BOOL status = TRUE;
	if(ulName == 0)
	{
		status = FALSE;
	}
	memcpy_s(temp,4,&ulName,4);
	name = gcnew String(temp);
	return status;
}
BOOL EnumACPIDeviceName(TCHAR *tszBuffer,DWORD dwSize,DWORD index)
{
	HDEVINFO hdi=SetupDiGetClassDevsEx(0,TEXT("ACPI_HAL"),0,DIGCF_ALLCLASSES ,0,0,0);
	BOOL ret = FALSE;
	if(INVALID_HANDLE_VALUE !=hdi)
	{
		SP_DEVINFO_DATA devinfo={0};
		devinfo.cbSize=sizeof(SP_DEVINFO_DATA);
		int i=0;
		if(SetupDiEnumDeviceInfo(hdi,index,&devinfo))
		{
			TCHAR tszTemp[MAX_PATH*2]={0};
			DWORD dwRSize=0;
			if(SetupDiGetDeviceInstanceId(hdi,&devinfo,tszTemp,1024,&dwRSize))
			{
				//_tprintf(TEXT("InstanceId=%s\n"),tszTemp);
				if(SetupDiGetDeviceRegistryProperty(hdi,&devinfo,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME ,0,(PBYTE)tszTemp,1024,&dwRSize))
				{
			//AfxMessageBox(tszTemp);
					//_tprintf(TEXT("DeviceName=%s\n"),tszTemp);
					_tcscpy_s(tszBuffer,dwSize,tszTemp);
					ret = TRUE;
				}
			}
		}//ERROR_NO_MORE_ITEMS
		SetupDiDestroyDeviceInfoList(hdi);
	}
	return ret;
}

void getACPIDeviceName(TCHAR *tszBuffer,DWORD dwSize)
{
	HDEVINFO hdi=SetupDiGetClassDevsEx(0,TEXT("ACPI_HAL"),0,DIGCF_ALLCLASSES ,0,0,0);
	if(INVALID_HANDLE_VALUE !=hdi)
	{
		SP_DEVINFO_DATA devinfo={0};
		devinfo.cbSize=sizeof(SP_DEVINFO_DATA);
		int i=0;
		while(SetupDiEnumDeviceInfo(hdi,i++,&devinfo))
		{
			TCHAR tszTemp[MAX_PATH*2]={0};
			DWORD dwRSize=0;
			if(SetupDiGetDeviceInstanceId(hdi,&devinfo,tszTemp,1024,&dwRSize))
			{
				//_tprintf(TEXT("InstanceId=%s\n"),tszTemp);
				if(SetupDiGetDeviceRegistryProperty(hdi,&devinfo,SPDRP_PHYSICAL_DEVICE_OBJECT_NAME ,0,(PBYTE)tszTemp,1024,&dwRSize))
				{
					//_tprintf(TEXT("DeviceName=%s\n"),tszTemp);
					_tcscpy_s(tszBuffer,dwSize,tszTemp);
				}
			}
		}//ERROR_NO_MORE_ITEMS
		SetupDiDestroyDeviceInfoList(hdi);
	}
}

void AcpiFilterControl::NotifyThreadProc(Object ^param)
{
	ULONG ulRetCnt=0,ulRetValue=0;
	BOOL bValid=0;
	TCHAR tszTemp[MAX_PATH]={0};
	NOTIFY_RET_DATA g_NotifyRetData={0};
	if(!param)
	{
		return;
	}
	AcpiFilterControl ^current = (AcpiFilterControl^)param;
	while(current->isNotifyMonitorRunning)
	{
		bValid=0;
		if(current->isNotifyMonitorRunning  &&  WAIT_OBJECT_0==WaitForSingleObject(current->hNotifyEvent,INFINITE))
		{
			if(current->isNotifyMonitorRunning   &&  current->hAcpiFilterDevice  &&  current->hAcpiFilterDevice!=INVALID_HANDLE_VALUE)
			{
				g_NotifyRetData.dwCount=0;
				if(current->isNotifyMonitorRunning  &&  DeviceIoControl(current->hAcpiFilterDevice, IOCTL_TDC3_GET_NOTIFY_LIST,
					&g_NotifyRetData, sizeof(NOTIFY_RET_DATA), 
					&g_NotifyRetData, sizeof(NOTIFY_RET_DATA), &ulRetCnt, 0))
				{
					DWORD dwCount=g_NotifyRetData.dwCount;
					for(int i=0;i<dwCount;i++)
					{
						/*if(g_hTargetWnd)
							PostMessage(g_hTargetWnd,WM_DEVICE_NOTIFY,(WPARAM)g_NotifyRetData.g_NotifyArray[i].pdo,g_NotifyRetData.g_NotifyArray[i].ulNotifyValue);*/
						NotifyEventArgs ^arg = gcnew NotifyEventArgs();
						int index = current->GetDeviceIndex(g_NotifyRetData.g_NotifyArray[i].pdo);
						if( index >= 0)
						{
							arg->deviceIndex = (UINT32)index;
						}
						arg->notifyInformation = g_NotifyRetData.g_NotifyArray[i].ulNotifyValue;
						current->Notify(current,arg);
					}
				}
			}
		}
	}
}

#define DATA_BUFFER_SIZE 64*1024
void AcpiFilterControl::MethodThreadProc(Object ^param)
{
	DWORD g_dwMaxLength=50;
	BYTE g_pbDataBuffer[DATA_BUFFER_SIZE]={0};
	ULONG ulRetCnt=0,ulRetValue=0;
	BOOL bValid=0;
	CString csTemp;
	if(!param)
	{
		return;
	}
	AcpiFilterControl ^current = (AcpiFilterControl^)param;
	while(current->isMethodMonitorRunning)
	{
		bValid=0;
		if(current->isMethodMonitorRunning  &&  WAIT_OBJECT_0==WaitForSingleObject(current->hMethodEvent,INFINITE))
		{
			if(current->isMethodMonitorRunning   &&  current->hAcpiFilterDevice  &&  current->hAcpiFilterDevice!=INVALID_HANDLE_VALUE)
			{
				TCHAR tszTemp[MAX_PATH]={0},tszKeyName[MAX_PATH]={0};
				DWORD dwRet=0,dwNameSize=0;
				if(current->isMethodMonitorRunning)
				{
					HKEY hReg=0;
					MethodEventArgs ^methodArg;
					if(!RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("SOFTWARE\\AcpiTool\\TraceData"),&hReg))
					{
						int i=0;
						HKEY hIrpData=0;
						while(!RegEnumKey(hReg,i,tszKeyName,MAX_PATH*sizeof(TCHAR)))
						{
							csTemp.Format(TEXT("SOFTWARE\\AcpiTool\\TraceData\\%s"),tszKeyName);
							if(!RegOpenKey(HKEY_LOCAL_MACHINE,csTemp.GetBuffer(),&hIrpData))
							{
								methodArg = gcnew MethodEventArgs();
								int j=0;
								while(!(dwRet=DATA_BUFFER_SIZE,dwNameSize=MAX_PATH,
								RegEnumValue(hIrpData,j++,tszTemp,&dwNameSize,0,REG_NONE,g_pbDataBuffer,&dwRet)))
								{
									csTemp=TEXT("");
									if(!_tcsicmp(tszTemp,TEXT("Description")))
									{
										if(dwRet)csTemp.Format(TEXT("%s"),(TCHAR*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("HwID")))
									{
										if(dwRet)csTemp.Format(TEXT("%s"),(TCHAR*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("DeviceObject")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("irp")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("iTime")))
									{
										TIME_FIELDS* pTime=(TIME_FIELDS*)g_pbDataBuffer;
										csTemp.Format(TEXT("%02d:%02d:%02d:%03d"),pTime->Hour,pTime->Minute,pTime->Second,pTime->Milliseconds);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("oTime")))
									{
										TIME_FIELDS* pTime=(TIME_FIELDS*)g_pbDataBuffer;
										csTemp.Format(TEXT("%02d:%02d:%02d:%03d"),pTime->Hour,pTime->Minute,pTime->Second,pTime->Milliseconds);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("MethodPath")))
									{
										csTemp.Format(TEXT("MethodPath: "));
										csTemp=TEXT("");
										DWORD dwCount=dwRet/sizeof(DWORD);
										CString csMethod;
										String ^temp,^path;
										path = gcnew String("");
										for(int i=0;i<dwCount;i++)
										{
											UlongToName(  ((DWORD*)g_pbDataBuffer)[i],temp);
											path += temp;
											if(i<dwCount-1)
												path +=".";
										}
										methodArg->methodInformation->Add(gcnew String(tszTemp),path);
										methodArg->methodPath = gcnew String(path);
									}
									else if(!_tcsicmp(tszTemp,TEXT("InputSize")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("ControlCode")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("Information")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("Status")))
									{
										csTemp.Format(TEXT("0x%X"),*(ULONG*)g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("InputData")))
									{
										csTemp=TEXT("");
										AnalyzeAcpiBuffer(csTemp,g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
									else if(!_tcsicmp(tszTemp,TEXT("OutputData")))
									{
										csTemp=TEXT("");
										AnalyzeAcpiBuffer(csTemp,g_pbDataBuffer);
										methodArg->methodInformation->Add(gcnew String(tszTemp),gcnew String(csTemp));
									}
								}
								RegCloseKey(hIrpData);
								current->Method(current,methodArg);
							}
							
							RegDeleteKey(hReg,tszKeyName);
							//break;
						}
						RegCloseKey(hReg);
					}		
				}
			}
		}
	}
}
void AnalyzeAcpiBuffer(CString& in_csTemp,PVOID in_pvBuffer)
{
	if(!in_pvBuffer)return;
	DWORD dwSig=*(DWORD*)in_pvBuffer;
	DWORD dwCount=0;
	TCHAR *tszData=0;
	DWORD dwSize=0;
	CString csTemp;
	CHAR tszTemp[MAX_PATH]={0};
	pin_ptr<const wchar_t> pStrArgumentType;
	switch(dwSig)
	{
	case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:
		in_csTemp += "\nSignature: ACPI_EVAL_INPUT_BUFFER_SIGNATURE";
		break;
	case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
		in_csTemp += "\nSignature: ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE";
		if(in_pvBuffer)
		{
			PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER piBuffer=(PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER)in_pvBuffer;
			pStrArgumentType = PtrToStringChars(ARGUMENT_TYPE::INT.ToString());
			csTemp.Format(TEXT("\n\t#%d Type: %s,Value: 0x%X"),dwCount,pStrArgumentType,piBuffer->IntegerArgument); 
			in_csTemp+=csTemp.GetBuffer();
		}
		break;
	case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:
		in_csTemp += "\nSignature: ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE";
		if(in_pvBuffer)
		{
			PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING piBuffer=(PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING)in_pvBuffer;
			if(sizeof(TCHAR)>1)
			{
				dwSize=MultiByteToWideChar(CP_UTF8,0,(LPCSTR)piBuffer->String,piBuffer->StringLength,tszData,dwSize);
				tszData=new(TCHAR[dwSize]);
				if(tszData)
				{
					pStrArgumentType = PtrToStringChars(ARGUMENT_TYPE::STRING.ToString());
					MultiByteToWideChar(CP_UTF8,0,(LPCSTR)piBuffer->String,piBuffer->StringLength,tszData,dwSize);
					csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: %s"),dwCount,pStrArgumentType,tszData); 
					in_csTemp+=csTemp.GetBuffer();
					delete tszData;
					tszData=0;
				}
			}else
			{
				pStrArgumentType = PtrToStringChars(ARGUMENT_TYPE::STRING.ToString());
				tszData=(TCHAR*)piBuffer->String;
				csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: %s"),dwCount,pStrArgumentType,tszData); 
				in_csTemp+=csTemp.GetBuffer();
			}
		}
		break;
	case ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE:
	case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE:
		if(dwSig == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
		{
			in_csTemp += "\nSignature: ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE";
		}
		else
		{
			in_csTemp += "\nSignature: ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE";
		}
		if(in_pvBuffer)
		{
			DWORD dwArgCount=0;
			DWORD dwPos=0;
			if(dwSig==ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
			{
				PACPI_EVAL_OUTPUT_BUFFER piBuffer=(PACPI_EVAL_OUTPUT_BUFFER)in_pvBuffer;
				dwArgCount=piBuffer->Count;
				dwPos=sizeof(ACPI_EVAL_OUTPUT_BUFFER)-sizeof(ACPI_METHOD_ARGUMENT);
			}
			if(dwSig==ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE)
			{
				PACPI_EVAL_INPUT_BUFFER_COMPLEX piBuffer=(PACPI_EVAL_INPUT_BUFFER_COMPLEX)in_pvBuffer;
				dwArgCount=piBuffer->ArgumentCount;
				dwPos=sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)-sizeof(ACPI_METHOD_ARGUMENT);
			}
			//csTemp.Format(TEXT("dwArgCount=%ld dwPos=%ld\r\n"),dwArgCount,dwPos); 
			//OutputDebugString(csTemp.GetBuffer());
			for(int i=0;i<dwArgCount;i++)
			{
				PACPI_METHOD_ARGUMENT pMethodArg=(PACPI_METHOD_ARGUMENT)&(  ((PBYTE)in_pvBuffer)[dwPos]  );
				pStrArgumentType = PtrToStringChars(((ARGUMENT_TYPE)pMethodArg->Type).ToString());
				switch(pMethodArg->Type)
				{
					case METHOD_ARGUMENT_TYPE_INTEGER:
						csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: 0x%X"),i,pStrArgumentType,pMethodArg->Argument); 
						in_csTemp+=csTemp.GetBuffer();
						break;
					case METHOD_ARGUMENT_TYPE_STRING:
						
						if(sizeof(TCHAR)>1)
						{
							dwSize=0;
							char *szTemp=new(char[pMethodArg->DataLength+1]);
							if(!szTemp)break;
							memset(szTemp,0,pMethodArg->DataLength+1);
							memcpy(szTemp,pMethodArg->Data,pMethodArg->DataLength);
							dwSize=MultiByteToWideChar(CP_UTF8,0,(LPCSTR)szTemp,pMethodArg->DataLength,tszData,dwSize);
							tszData=new(TCHAR[dwSize+1]);
							if(tszData)
							{
								memset(tszData,0,sizeof(TCHAR)*(dwSize+1));
								MultiByteToWideChar(CP_UTF8,0,(LPCSTR)szTemp,pMethodArg->DataLength,tszData,dwSize);
								csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: %s"),dwCount,pStrArgumentType,tszData); 
								in_csTemp+=csTemp.GetBuffer();
								delete tszData;tszData=0;
							}
							delete szTemp;szTemp=0;
						}else
						{
							char *szTemp=new(char[pMethodArg->DataLength+1]);
							if(!szTemp)break;
							memset(szTemp,0,pMethodArg->DataLength+1);
							memcpy(szTemp,pMethodArg->Data,pMethodArg->DataLength);
							csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: %s"),dwCount,pStrArgumentType,szTemp); 
							in_csTemp+=csTemp.GetBuffer();
							delete szTemp;szTemp=0;
						}
						break;
					case METHOD_ARGUMENT_TYPE_BUFFER:
					case METHOD_ARGUMENT_TYPE_PACKAGE:
						csTemp.Format(TEXT("\n\t#%ld Type: %s,Value: "),i,pStrArgumentType);in_csTemp+=csTemp.GetBuffer();
						for(int itemp=0;itemp<pMethodArg->DataLength;itemp++)
						{
							csTemp.Format(TEXT("0x%02X"),((PBYTE)pMethodArg->Data)[itemp] ); 
							in_csTemp+=csTemp.GetBuffer();
							if(itemp != pMethodArg->DataLength - 1)
							{
								in_csTemp+=",";
							}
						}
						break;
				}
				//OutputDebugString(csTemp.GetBuffer());
				dwPos+=pMethodArg->DataLength+sizeof(USHORT)*2;
			}
		}
		break;
	
	default:break;
	}
}