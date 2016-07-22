#include "ntmdrv.h"
PDEVICE_OBJECT acpihal_pdo=0;
ULONG g_ulIndex=0;
ULONG g_ulACPIHandleOffset=0x12c;
ULONG g_ulACPICallBackOffset=0x98;

KERNEL_DEVICE_INFO *pRegHeader=0;
KSPIN_LOCK RegListLock={0};
KSPIN_LOCK NotifyLock={0};
NOTIFY_DATA NotifyBuffer[256]={0};
PKEVENT g_pApNotifyEvent=0;
PKEVENT g_pApMethodEvent=0;
ULONG g_ulNotifyCount=0;
extern HOOKED_DATA g_DataHeader;
extern KEVENT g_MtEvent;
extern KSPIN_LOCK g_LiskSpinLock;
//KSPIN_LOCK g_IrpSpinLock;
PDEVICE_EXTENSION g_pdx=0;
UCHAR g_ucCanLog=0;

PIRP pCurrentIrp = NULL;

extern "C" NTSTATUS DriverEntry (IN PDRIVER_OBJECT pDriverObject,IN PUNICODE_STRING pRegistryPath	) 
{
	NTSTATUS status;
		InitializeListHead((PLIST_ENTRY)&g_DataHeader.list);
	KeInitializeSpinLock(&RegListLock);
	KeInitializeSpinLock(&NotifyLock);
		KeInitializeEvent(&g_MtEvent,NotificationEvent,0);
		//KeInitializeSpinLock(&g_IrpSpinLock);
		KeInitializeSpinLock(&g_LiskSpinLock);
		HANDLE hThread;
		if(!PsCreateSystemThread(&hThread,0,0,0,0,(PKSTART_ROUTINE)SystemMtThread,0))
		{
			ZwClose(hThread);
		}
		g_ucCanLog= CreateTraceData();
	__try
	{
		pDriverObject->DriverUnload = ntdmDrvUnload;
		pDriverObject->MajorFunction[IRP_MJ_CREATE] = ntdmDrvDispatchRoutine;
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ntdmDrvDispatchRoutine;
		pDriverObject->MajorFunction[IRP_MJ_WRITE] = ntdmDrvDispatchRoutine;
		pDriverObject->MajorFunction[IRP_MJ_READ] = ntdmDrvDispatchRoutine;
		pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ntdmDrvDispatchRoutine;
		pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ntdmDrvDispatchRoutine;
		status = CreateDevice(pDriverObject);
		if(!status)
		{
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("EXCEPTION in DriverEntry\n"));
	}
	return status;
}
ACPI_OBJECT* hObjOld=0;
ULONG GetListItemCount(LIST_ENTRY* in_Entry)
{
	ULONG ulRet=0;
	LIST_ENTRY* pTemp=in_Entry;
	if(pTemp)
	{
		do
		{
			ulRet++;
			pTemp=pTemp->Blink;
		}
		while(pTemp!=in_Entry);
	}
	return ulRet;
}
LIST_ENTRY* GetListItemByIndex(LIST_ENTRY* in_Entry,ULONG in_ulIndex)
{
	LIST_ENTRY* pRet=in_Entry;
	LIST_ENTRY* pTemp=in_Entry;
	ULONG ulCount=0;
	if(pTemp  &&  in_ulIndex)
	{
		do
		{
			ulCount++;
			pTemp=pTemp->Blink;
		}
		while(pTemp!=in_Entry  &&  ulCount!=in_ulIndex);
		pRet=pTemp;
	}
	return pRet;
}
void UnRegForAll(PDEVICE_EXTENSION pdx)
{
	if(!acpihal_pdo  &&  !pdx->AcpiInterface.UnregisterForDeviceNotifications)return;
	ULONG ulCount =GetListItemCount((PLIST_ENTRY)pdx->pDeviceNofityInfo);
	KERNEL_DEVICE_INFO *pTemp,*pInfo=(KERNEL_DEVICE_INFO*)pdx->pDeviceNofityInfo;
	while(pInfo)
	{
		pTemp=pInfo;
		pInfo=(KERNEL_DEVICE_INFO*)pInfo->pList.Blink;
		if(pTemp->ulHooked)
		{
			if(pTemp->pCallBack)
			{
				*(PVOID*)((ULONG_PTR)pTemp->pdo->DeviceExtension+g_ulACPICallBackOffset+sizeof(PDEVICE_NOTIFY_CALLBACK))=pTemp->pParam;
				*(PDEVICE_NOTIFY_CALLBACK*)((ULONG_PTR)pTemp->pdo->DeviceExtension+g_ulACPICallBackOffset)=pTemp->pCallBack;
			}else
			{
				pdx->AcpiInterface.UnregisterForDeviceNotifications(pTemp->pdo,ASUSACPIDeviceNotifyHandler);
			}
			//if(pTemp->pCallBack)
			//{
			//	pdx->AcpiInterface.UnregisterForDeviceNotifications(pTemp->pdo,ASUSACPIDeviceNotifyHandler);
			//	pdx->AcpiInterface.RegisterForDeviceNotifications(pTemp->pdo,pTemp->pCallBack,pTemp->pParam);
			//}else
			//{
			//	pdx->AcpiInterface.UnregisterForDeviceNotifications(pTemp->pdo,ASUSACPIDeviceNotifyHandler);
			//}
		}
		ExFreePool(pTemp);
		if(pInfo==pdx->pDeviceNofityInfo)break;
	}
}
VOID ntdmDrvUnload (IN PDRIVER_OBJECT pDriverObject) 
{
	PDEVICE_OBJECT pDev;
	PDEVICE_EXTENSION pDx;
	DebugPrint(("ntdmDrvUnload\n"));
	while(!IsListEmpty((PLIST_ENTRY)&g_DataHeader))
	{
		HOOKED_DATA *temp;
		temp = (HOOKED_DATA *)ExInterlockedRemoveHeadList((PLIST_ENTRY)&g_DataHeader,&g_LiskSpinLock);
		if(temp != NULL)
		{
			ExFreePool((HOOKED_DATA*)temp);
		}
	}
	pDev = pDriverObject->DeviceObject;
	while (pDev != NULL)
	{
		pDx = (PDEVICE_EXTENSION)pDev->DeviceExtension;
		IoDeleteSymbolicLink(&(pDx->ustrSymLinkName));
		IoDeleteDevice(pDev);
	}
	//PDEVICE_OBJECT	Pdo;
	//
	//Pdo = pDriverObject->DeviceObject;
	//AcpiHalUnHook();
	//if (g_ucCanLog)
	//{
	//	DeleteTraceData();
	//}
	//if (Pdo) 
	//{
	//	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)Pdo->DeviceExtension;
	//	UnRegForAll(pdx);
	//	//if(pdx->LowerDevice)
	//	//{
	//	//	IoDetachDevice(pdx->LowerDevice);
	//	//}
	//	if(pdx->AcpiInterface.InterfaceDereference)
	//	{
	//		pdx->AcpiInterface.InterfaceDereference(pdx->AcpiInterface.Context);
	//	}
	//	if(g_pApNotifyEvent)
	//	{
	//		KeSetEvent(g_pApNotifyEvent,0,0);
	//		ObDereferenceObject(g_pApNotifyEvent);
	//		g_pApNotifyEvent=0;
	//	}
	//	if(g_pApMethodEvent)
	//	{
	//		KeSetEvent(g_pApMethodEvent,0,0);
	//		ObDereferenceObject(g_pApMethodEvent);
	//		g_pApMethodEvent=0;
	//	}
	//	if(pdx->ustrSymLinkName.Length>0)
	//		IoDeleteSymbolicLink(&pdx->ustrSymLinkName);
	//	IoDeleteDevice(Pdo);

	//}
}
void Uninitialize(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT	Pdo;
	Pdo = pDriverObject->DeviceObject;
	AcpiHalUnHook();
	if (g_ucCanLog)
	{
		DeleteTraceData();
	}
	if (Pdo) 
	{
		PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)Pdo->DeviceExtension;
		UnRegForAll(pdx);
		//if(pdx->LowerDevice)
		//{
		//	IoDetachDevice(pdx->LowerDevice);
		//}
		if(pdx->AcpiInterface.InterfaceDereference)
		{
			pdx->AcpiInterface.InterfaceDereference(pdx->AcpiInterface.Context);
		}
		if(g_pApNotifyEvent)
		{
			KeSetEvent(g_pApNotifyEvent,0,0);
			ObDereferenceObject(g_pApNotifyEvent);
			g_pApNotifyEvent=0;
		}
		if(g_pApMethodEvent)
		{
			KeSetEvent(g_pApMethodEvent,0,0);
			ObDereferenceObject(g_pApMethodEvent);
			g_pApMethodEvent=0;
		}
		if(pdx->ustrSymLinkName.Length>0)
			IoDeleteSymbolicLink(&pdx->ustrSymLinkName);
		IoDeleteDevice(Pdo);

	}
}


void BuildDeviceLinkList(PDEVICE_EXTENSION pdx)
{
	KERNEL_DEVICE_INFO *pHookInfo=0,*pHookHead=0;
	UCHAR ucFirst=1;
	ULONG ulRet=0,status=0;
	if(acpihal_pdo)
	{
		PDEVICE_OBJECT pdo=acpihal_pdo->DriverObject->DeviceObject;
		while(pdo)
		{
			if(pdo->Flags  &  DO_BUS_ENUMERATED_DEVICE )
			{
				if(pdo->DeviceExtension != NULL)
				{
					pHookInfo=(KERNEL_DEVICE_INFO*)ExAllocatePool(NonPagedPool,sizeof(KERNEL_DEVICE_INFO));
					if(pHookInfo)
					{
						KdPrint(("ItemAddress=0x%X\n",pHookInfo));
						memset(pHookInfo,0,sizeof(KERNEL_DEVICE_INFO));
						InitializeListHead((PLIST_ENTRY)pHookInfo);
						if(ucFirst)
						{
							ucFirst=0;
							pdx->pDeviceNofityInfo=pHookInfo;
						}
						
						pHookInfo->pdo=pdo;
						//DebugPrint(("ACPI: pdo %08lx\n",pdo));
						pHookInfo->pCallBack=*(PDEVICE_NOTIFY_CALLBACK*)((ULONG_PTR)pdo->DeviceExtension+g_ulACPICallBackOffset);
						pHookInfo->pParam=*(PVOID*)((ULONG_PTR)pdo->DeviceExtension+g_ulACPICallBackOffset+sizeof(PDEVICE_NOTIFY_CALLBACK));
						status=IoGetDeviceProperty(pdo,
							//DevicePropertyFriendlyName
							//DevicePropertyPhysicalDeviceObjectName
							//DevicePropertyHardwareID
							DevicePropertyDeviceDescription
							,260*sizeof(WCHAR),pHookInfo->wszNameBuffer,&ulRet);
						if(!ucFirst)
						{
							ExInterlockedInsertHeadList((PLIST_ENTRY)pdx->pDeviceNofityInfo,(PLIST_ENTRY)pHookInfo,&RegListLock);
						}
					}
				}
			}
			pdo=pdo->NextDevice;
		}
	}
}

NTSTATUS ntdmDrvDispatchRoutine(IN PDEVICE_OBJECT pDevObj,IN PIRP pIrp) 
{
	NTSTATUS status = 0;
	__try
	{		
		PIO_STACK_LOCATION IrpStack;
		IrpStack = IoGetCurrentIrpStackLocation(pIrp);
		ULONG dwIoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
		ULONG* pInputs=(ULONG*)pIrp->AssociatedIrp.SystemBuffer;
		UI_METHOD_INPUT *pMethodInputs=(UI_METHOD_INPUT*)pInputs;
		//ULONG *pOutputs=(ULONG*)pIrp->UserBuffer;
		ULONG *pOutputs=(ULONG*)pIrp->AssociatedIrp.SystemBuffer;
		LONG ulInputBufferSize=IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		ULONG ulOutputBufferSize=IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
		PDEVICE_EXTENSION pdx=(PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		pIrp->IoStatus.Information=0;
		pIrp->IoStatus.Status =status;
		ULONG_PTR ulRet=0;
		switch(IrpStack->MajorFunction)
		{//STATUS_INVALID_PARAMETER
		case IRP_MJ_CLOSE:
			Uninitialize(pDevObj->DriverObject);
			break;
			case IRP_MJ_DEVICE_CONTROL:
				pIrp->IoStatus.Status =status=STATUS_UNSUCCESSFUL;
				pIrp->IoStatus.Information=0;
				switch(dwIoControlCode)
				{
				case IOCTL_TDC3_CALL_METHOD:
					
					DebugPrint(("ACPI: IOCTL_TDC3_CALL_METHOD\n"));
					if(pInputs  &&  acpihal_pdo)
					{
						//hObjOld=ReplaceObject(   ((ACPI_OBJECT*)pMethodInputs->PDO));
						hObjOld=(ACPI_OBJECT *)ReplaceACPIHandle(acpihal_pdo,pMethodInputs->PDO);
						if(hObjOld)
						{
							status=EvaluateACPIMethod(acpihal_pdo,pMethodInputs->ulMethod,pMethodInputs->ulArgumentCount,pMethodInputs->Argument,
								pMethodInputs->ulSize,pMethodInputs,ulOutputBufferSize,&ulRet);
							if(!status)
							{
								pIrp->IoStatus.Information=ulRet;
								pIrp->IoStatus.Status =status;
							}
							else
							{
								DebugPrint(("ACPI: status %08lx",status));
							}
							ReplaceACPIHandle(acpihal_pdo,hObjOld);
							//ReplaceObject(hObjOld);
						}
					}
					break;
				case IOCTL_TDC3_QUERY_ACPI_OBJECT:
					if(pOutputs && ulOutputBufferSize>=sizeof(ULONG) &&  acpihal_pdo)
					{
						//ulRet=GetAcpiObjectCount(GetAcpiObjectRoot( (ACPI_OBJECT*)(*(ULONG*)((ULONG_PTR)(acpihal_pdo->DeviceExtension)+0x12c))),0,0);
						ulRet=GetAcpiObjectCount(GetAcpiObjectRoot( (ACPI_OBJECT *)GetAcpiHandle(acpihal_pdo)),0,0);
						ulRet++;
						if(ulOutputBufferSize<ulRet*sizeof(ACPI_OBJECT_AP))
						{
							pIrp->IoStatus.Information=sizeof(ULONG);
							DebugPrint(("ACPI: pOutputs %08lx\n",pOutputs));
							DebugPrint(("ACPI: pOutputs[0] %08lx\n",pOutputs[0]));
							DebugPrint(("ACPI: ulRet %08lx\n",ulRet));
							DebugPrint(("ACPI: ACPI_OBJECT_AP %08lx\n",sizeof(ACPI_OBJECT_AP)));
							pOutputs[0]=(ULONG)(ulRet*sizeof(ACPI_OBJECT_AP));
							pIrp->IoStatus.Status =status=0;
						}
						else
						{
							g_ulIndex=0;
							//GetAcpiObjectCount(GetAcpiObjectRoot( (ACPI_OBJECT*)(*(ULONG*)((ULONG)(acpihal_pdo->DeviceExtension)+0x12c))),(ACPI_OBJECT_AP *)pOutputs,ulOutputBufferSize);
							GetAcpiObjectCount(GetAcpiObjectRoot( (ACPI_OBJECT *)GetAcpiHandle(acpihal_pdo)),(ACPI_OBJECT_AP *)pOutputs,ulOutputBufferSize);
							pIrp->IoStatus.Information=ulRet*sizeof(ACPI_OBJECT_AP);
							pIrp->IoStatus.Status =status=0;
						}
					}
					break;
				case IOCTL_TDC3_INITIALIZE:
					if(pInputs)
					{
						//DebugPrint(("ACPI: 1\n"));
						if(!acpihal_pdo)
						{
						//DebugPrint(("ACPI: 2\n"));
							RTL_OSVERSIONINFOW osvx={0};
							osvx.dwOSVersionInfoSize=sizeof(RTL_OSVERSIONINFOW);
							RtlGetVersion(&osvx);
							ULONG ulSupporttedVersion=1;

							//DebugPrint(("ACPI: dwMajorVersion == %d\n",osvx.dwMajorVersion));
							//DebugPrint(("ACPI: dwMinorVersion == %d\n",osvx.dwMinorVersion));

							if(osvx.dwMajorVersion==5){g_ulACPIHandleOffset=0x12c;g_ulACPICallBackOffset=0x98;}
							else if(osvx.dwMajorVersion==6)
							{
						//DebugPrint(("ACPI: 3\n"));
								if(osvx.dwMinorVersion==0)
								{
						//DebugPrint(("ACPI: 4\n"));
#ifdef _X86_
						//DebugPrint(("ACPI: 4.1\n"));
									g_ulACPIHandleOffset=0x168
										/*win7X64 0x240*//*win8X64 0x2a0*//*win8X86 0x1ac*//*win7X86 0x168*/;
									g_ulACPICallBackOffset=0x138 + 0x10;
										/*win7X86 0xc4 + 0x08;*//*win8X64: 0x138 + 0x10*//*win7X64 0x138 + 0x10*//*win8X860xc4+0x08*/
#else
						//DebugPrint(("ACPI: 4.2\n"));
									ulSupporttedVersion = 0;
#endif
								}
								else if(osvx.dwMinorVersion==1)
								{
									
						//DebugPrint(("ACPI: 5\n"));
#ifdef _X86_
						//DebugPrint(("ACPI: 5.1\n"));
									g_ulACPIHandleOffset=0x168;
									g_ulACPICallBackOffset=0xc4 + 0x08;
#else
	#ifdef _AMD64_
						//DebugPrint(("ACPI: 5.2\n"));
									g_ulACPIHandleOffset=0x240;
									g_ulACPICallBackOffset=0x138 + 0x10;
	#else
						//DebugPrint(("ACPI: 5.3\n"));
									ulSupporttedVersion = 0;
	#endif
#endif
								}
								else if(osvx.dwMinorVersion == 2)
								{
						//DebugPrint(("ACPI: 6\n"));
#ifdef _X86_
						//DebugPrint(("ACPI: win8 X86\n"));
									g_ulACPIHandleOffset=0x1ac;
									g_ulACPICallBackOffset=0xc4 + 0x08;
#else
	#ifdef _AMD64_
						//DebugPrint(("ACPI: win8 X64\n"));
									g_ulACPIHandleOffset=0x2a0;
									g_ulACPICallBackOffset=0x138 + 0x10;
	#else
		#ifdef ARM 
						//DebugPrint(("ACPI: win8 ARM\n"));
									g_ulACPIHandleOffset=0x1ac;
									g_ulACPICallBackOffset=0xc4 + 0x08;
		#else
						//DebugPrint(("ACPI: win8 UNKNOWN\n"));
						ulSupporttedVersion = 0;
		#endif
	#endif
#endif
								}
								else if(osvx.dwMinorVersion == 3)
								{
#ifdef _X86_
						//DebugPrint(("ACPI: win8.1 X86\n"));
									g_ulACPIHandleOffset=0x1ac;
									g_ulACPICallBackOffset=0xc8 + 0x08;
#else
	#ifdef _AMD64_
						//DebugPrint(("ACPI: win8.1 X64\n"));
									g_ulACPIHandleOffset=0x2b0;
									g_ulACPICallBackOffset=0x140 + 0x10;
	#else
		#ifdef ARM 
						//DebugPrint(("ACPI: win8.1 ARM\n"));
									g_ulACPIHandleOffset=0x1ac;
									g_ulACPICallBackOffset=0xc8 + 0x08;
		#else
						//DebugPrint(("ACPI: win8.1 UNKNOWN\n"));
						ulSupporttedVersion = 0;
		#endif
	#endif
#endif
								}
								else
								{
									ulSupporttedVersion = 0;
								}
							}
							else if(osvx.dwMajorVersion == 10)
							{
								//DebugPrint(("ACPI: win10\n"));

								if(osvx.dwMinorVersion == 0)
								{
#ifdef _X86_
						DebugPrint(("ACPI: win10 X86 not support\n"));
									ulSupporttedVersion = 0;
#else
	#ifdef _AMD64_
						DebugPrint(("ACPI: win10 X64\n"));
									g_ulACPIHandleOffset=0x2c0;
									g_ulACPICallBackOffset=0x140 + 0x10;
	#else
						DebugPrint(("ACPI: win10 UNKNOWN\n"));
						ulSupporttedVersion = 0;
	#endif
#endif
								}
							}
							else{ulSupporttedVersion=0;}
							///////////////////////////////////////////////////
							if(ulSupporttedVersion)
							{
						//DebugPrint(("ACPI: 7\n"));
								UNICODE_STRING usDeviceName;
								PFILE_OBJECT pfo=0;
								PDEVICE_OBJECT pdoTemp=0;
								RtlInitUnicodeString(&usDeviceName,/*L"\\Device\\00000016"*/(PCWSTR)pInputs);
								PSTR p = (PSTR)pInputs;
								DebugPrint(("ACPI: name length %d",wcslen((PCWSTR)pInputs)));
								DebugPrint(("ACPI: %c%c%c%c%c %c%c%c%c%c %c%c%c%c%c %c\n",p[0],p[2],p[4],p[6],p[8],p[10],p[12],p[14],p[16],p[18],p[20],p[22],p[24],p[26],p[28],p[30]));
								status = IoGetDeviceObjectPointer(&usDeviceName,FILE_READ_DATA,&pfo,&pdoTemp);
								if(status)
								{
						//DebugPrint(("ACPI: 8\n"));
						DebugPrint(("ACPI: %08lx",status));
									pIrp->IoStatus.Status =status=STATUS_UNSUCCESSFUL;
									pIrp->IoStatus.Information=0;
								}
								else
								{	
									acpihal_pdo=pdoTemp;
									DebugPrint(("ACPI: acpihal_pdo %08lx\n",acpihal_pdo));
									DebugPrint(("ACPI: pdx %08lx\n",pdx));
									DebugPrint(("ACPI: DeviceObject %08lx\n",acpihal_pdo->DriverObject->DeviceObject));
#ifdef _AMD64_
									DebugPrint(("ACPI: Extension %I64x\n",acpihal_pdo->DriverObject->DeviceObject->DeviceExtension));
#else
									DebugPrint(("ACPI: Extension %08lx\n",acpihal_pdo->DriverObject->DeviceObject->DeviceExtension));
#endif
									
									BuildDeviceLinkList(pdx);
									if(!GetAcpiInterfaces(acpihal_pdo->DriverObject->DeviceObject,&pdx->AcpiInterface))
									{
										if(pdx->AcpiInterface.Size)
										{
											pdx->ucGetAcpiInterfaceOk=1;
										}
									}
									ObDereferenceObject(pfo);
									pIrp->IoStatus.Information=0;
									pIrp->IoStatus.Status =status=0;
								}
							}else
							{
								pIrp->IoStatus.Information=0;
								pIrp->IoStatus.Status =status=STATUS_UNSUCCESSFUL;
							}
						}else
						{
							pIrp->IoStatus.Information=0;
							pIrp->IoStatus.Status =status=0;
						}
						
					}
					break;
				case IOCTL_TDC3_QUERY_DEVICE_OBJECT:
					if(pOutputs  &&  ulOutputBufferSize>=sizeof(ULONG))
					{
						ULONG ulCount=GetListItemCount((PLIST_ENTRY)pdx->pDeviceNofityInfo);
						ULONG ulAPDataSize=sizeof(long)+sizeof(AP_DEVICE_INFO)*ulCount;
						AP_DEVICE_LIST *outList = (AP_DEVICE_LIST*)pOutputs;
						if(ulOutputBufferSize>=ulAPDataSize)
						{
							outList->dwCount = ulCount;
							AP_DEVICE_INFO *pApList=outList->list;
							for(UINT32 i=0;i<ulCount;i++)
							{
								KERNEL_DEVICE_INFO *pKnlDeviceInfo=(KERNEL_DEVICE_INFO*)GetListItemByIndex((PLIST_ENTRY)pdx->pDeviceNofityInfo,i);
								if(!pKnlDeviceInfo)
									break;
								{
									pApList[i].pdo=pKnlDeviceInfo->pdo;
									memcpy(pApList[i].wszDeviceName,pKnlDeviceInfo->wszNameBuffer,sizeof(WCHAR)*260);
								}
							}
							pIrp->IoStatus.Information=ulAPDataSize;
							pIrp->IoStatus.Status =status=STATUS_SUCCESS;
						}else
						{
							outList->dwCount=sizeof(long)+sizeof(AP_DEVICE_INFO)*GetListItemCount((PLIST_ENTRY)pdx->pDeviceNofityInfo);
							pIrp->IoStatus.Information=4;
							pIrp->IoStatus.Status =status=STATUS_SUCCESS;
						}
					}
					break;
				case IOCTL_TDC3_UPDATE_NOTIFY_LIST:
					DebugPrint(("ACPI: IOCTL_TDC3_UPDATE_NOTIFY_LIST\n"));
					if(pInputs  &&  ulInputBufferSize>=sizeof(UPDATE_NOTIFY_INFO)  &&  acpihal_pdo
						&&  pdx->AcpiInterface.RegisterForDeviceNotifications  &&  pdx->AcpiInterface.UnregisterForDeviceNotifications)
					{
						UPDATE_NOTIFY_INFO *pInfo=(UPDATE_NOTIFY_INFO*)pInputs;
						ULONG ulCount=GetListItemCount((PLIST_ENTRY)pdx->pDeviceNofityInfo);
						for(UINT32 i=0;i<ulCount;i++)
						{
							KERNEL_DEVICE_INFO *pKnlDeviceInfo=(KERNEL_DEVICE_INFO*)GetListItemByIndex((PLIST_ENTRY)pdx->pDeviceNofityInfo,i);
							if(pInfo->PDO==pKnlDeviceInfo->pdo)
							{
								if(pInfo->ulAction)
								{
									if(!pKnlDeviceInfo->ulHooked)
									{
										if(status=pdx->AcpiInterface.RegisterForDeviceNotifications(pInfo->PDO,ASUSACPIDeviceNotifyHandler,pInfo->PDO))
										{
											status=0;
											pKnlDeviceInfo->ulHooked=1;
											DebugPrint(("ACPI: RegisterForDeviceNotifications For Device %S Failed\n",pKnlDeviceInfo->wszNameBuffer));
											*(PDEVICE_NOTIFY_CALLBACK*)((ULONG_PTR)pInfo->PDO->DeviceExtension+g_ulACPICallBackOffset)=ASUSACPIDeviceNotifyHandler;
											*(PVOID*)((ULONG_PTR)pInfo->PDO->DeviceExtension+g_ulACPICallBackOffset+sizeof(PDEVICE_NOTIFY_CALLBACK))=pInfo->PDO;

										}else
										{
											DebugPrint(("ACPI: RegisterForDeviceNotifications For Device %S Succeed\n",pKnlDeviceInfo->wszNameBuffer));
											status=0;
											pKnlDeviceInfo->ulHooked=1;
										}
									}else
									{
										status=0;
									}
								}else
								{
									status=0;
									KERNEL_DEVICE_INFO *pTemp,*pInfoTemp=(KERNEL_DEVICE_INFO*)pdx->pDeviceNofityInfo;
									while(pInfo)
									{
										pTemp=pInfoTemp;
										pInfoTemp=(KERNEL_DEVICE_INFO*)pInfoTemp->pList.Blink;
										if(pTemp->pdo==pInfo->PDO)
										{
											DebugPrint(("ACPI: oldCallBack %08lx",pTemp->pCallBack));
											if(pTemp->pCallBack)
											{
												*(PVOID*)((ULONG_PTR)pTemp->pdo->DeviceExtension+g_ulACPICallBackOffset+sizeof(PDEVICE_NOTIFY_CALLBACK))=pTemp->pParam;
												*(PDEVICE_NOTIFY_CALLBACK*)((ULONG_PTR)pTemp->pdo->DeviceExtension+g_ulACPICallBackOffset)=pTemp->pCallBack;												
											}else
											{
												pdx->AcpiInterface.UnregisterForDeviceNotifications(pTemp->pdo,ASUSACPIDeviceNotifyHandler);
											}
											break;
										}
										if(pInfoTemp==g_pdx->pDeviceNofityInfo)break;
									}
									pKnlDeviceInfo->ulHooked=0;
									//pdx->AcpiInterface.UnregisterForDeviceNotifications(pInfo->PDO,ASUSACPIDeviceNotifyHandler);
								}
							}
						}
					}
					break;
				case IOCTL_TDC3_INITIALIZE_NOTIFY:
					g_ulNotifyCount=0;
					if(pOutputs &&  pInputs  && ulInputBufferSize>=sizeof(HANDLE))
					{
						if(ulInputBufferSize>=sizeof(HANDLE))
						{ 
							HANDLE hTemp=*(HANDLE*)pInputs;
							if(ObReferenceObjectByHandle(hTemp,SYNCHRONIZE,*ExEventObjectType,KernelMode,(PVOID*)&g_pApNotifyEvent,0))
							{
								KdPrint(("ObReferenceObjectByHandle Failed\n"));
							}else
							{
								pIrp->IoStatus.Status=status=STATUS_SUCCESS;
								pIrp->IoStatus.Information=0;
							}
						}
					}
					break;
				case IOCTL_TDC3_UNINITIALIZE_NOTIFY:
					if(g_pApNotifyEvent)
					{
						ObDereferenceObject(g_pApNotifyEvent);
						g_pApNotifyEvent=0;
					}
					pIrp->IoStatus.Status=status=STATUS_SUCCESS;
					pIrp->IoStatus.Information=0;
				case IOCTL_TDC3_GET_NOTIFY_LIST:
					DebugPrint(("ACPI: IOCTL_TDC3_GET_NOTIFY_LIST\n"));
					KIRQL irql;
					KeAcquireSpinLock(&NotifyLock,&irql);
					if(g_ulNotifyCount)
					{
						if(pOutputs &&  pInputs  && ulInputBufferSize>=sizeof(HANDLE)  &&  ulOutputBufferSize>=g_ulNotifyCount*sizeof(NOTIFY_DATA)+sizeof(long))
						{
							NOTIFY_DATA_LIST *outList = (NOTIFY_DATA_LIST*)pOutputs;
							outList->dwCount = g_ulNotifyCount;
							memcpy(outList->g_NotifyArray,NotifyBuffer,sizeof(NOTIFY_DATA)*g_ulNotifyCount);
							pIrp->IoStatus.Status=status=STATUS_SUCCESS;
							pIrp->IoStatus.Information=g_ulNotifyCount*sizeof(NOTIFY_DATA)+sizeof(long);
							g_ulNotifyCount=0;
						}else
						{
							pIrp->IoStatus.Status=status=STATUS_UNSUCCESSFUL;
							pIrp->IoStatus.Information=0;
						}
					}else
					{
						pIrp->IoStatus.Status=status=STATUS_SUCCESS;
						pIrp->IoStatus.Information=0;
					}
					KeReleaseSpinLock(&NotifyLock,irql);
					break;
				case IOCTL_TDC3_UNINITIALIZE_METHOD_EVENT:
					if(g_pApMethodEvent)
					{
						ObDereferenceObject(g_pApMethodEvent);
						g_pApMethodEvent=0;
					}
					pIrp->IoStatus.Status=status=STATUS_SUCCESS;
					pIrp->IoStatus.Information=0;
					AcpiHalUnHook();
					break;
				case IOCTL_TDC3_INITIALIZE_METHOD_EVENT:
					DebugPrint(("ACPI: IOCTL_TDC3_INITIALIZE_METHOD_EVENT\n"));
					if(!g_pApMethodEvent  &&  acpihal_pdo  &&  pOutputs &&  pInputs  && ulInputBufferSize>=sizeof(HANDLE))
					{
						if(ulInputBufferSize>=sizeof(HANDLE))
						{ 
							HANDLE hTemp=*(HANDLE*)pInputs;
							PKEVENT pEvet=0;
							if(ObReferenceObjectByHandle(hTemp,SYNCHRONIZE,*ExEventObjectType,KernelMode,(PVOID*)&pEvet,0))
							{
								KdPrint(("ObReferenceObjectByHandle Failed\n"));
							}else
							{
								g_pApMethodEvent=pEvet;
								pIrp->IoStatus.Status=status=STATUS_SUCCESS;
								pIrp->IoStatus.Information=0;
								AcpiHalHook();
							}
						}
					}
					break;
				default:break;
				}
				break;		
			default:break;
		}
		pIrp->IoStatus.Status = status;	
		DebugPrint(("ACPI: Iostatus %08lx\n",pIrp->IoStatus.Status));
		//if(status!=STATUS_PENDING)
			IoCompleteRequest( pIrp, IO_NO_INCREMENT );
	}__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DebugPrint(("ACPI: exception\n"));
		pIrp->IoStatus.Status = DBG_EXCEPTION_HANDLED;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest( pIrp, IO_NO_INCREMENT );
		KdPrint(("EXCEPTION in ntdmDrvDispatchRoutine\n"));
	}
	return status;
}

NTSTATUS
SendDownStreamIrp(
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            Ioctl,
    IN PVOID            InputBuffer,
    IN ULONG            InputSize,
    IN PVOID            OutputBuffer,
    IN ULONG            OutputSize
)

/*
Routine Description:
    General-purpose function called to send a request to the Pdo.
    The Ioctl argument accepts the control method being passed down
    by the calling function.

	This subroutine is only valid for the IOCTLS other than ASYNC EVAL.

Arguments:
    Pdo             - the request is sent to this device object
    Ioctl           - the request - specified by the calling function
    InputBuffer     - incoming request
    InputSize       - size of the incoming request
    OutputBuffer    - the answer
    OutputSize      - size of the answer buffer

Return Value:
    NT Status of the operation
*/
{
    IO_STATUS_BLOCK     ioBlock;
    KEVENT              myIoctlEvent;
    NTSTATUS            status;
    PIRP                irp;

    // Initialize an event to wait on
    KeInitializeEvent(&myIoctlEvent, SynchronizationEvent, FALSE);

    // Build the request
    irp = IoBuildDeviceIoControlRequest(
        Ioctl,
        Pdo,
        InputBuffer,
        InputSize,
        OutputBuffer,
        OutputSize,
        FALSE,
        &myIoctlEvent,
        &ioBlock);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	IoGetNextIrpStackLocation(irp)->FileObject = (PFILE_OBJECT)0x866108f0;//0x8559a020;
	IoGetCurrentIrpStackLocation(irp)->FileObject = (PFILE_OBJECT)0x866108f0;//0x8559a020;
	if(Pdo == NULL)
	{
		DebugPrint(("ACPI: Pdo == NULL"));
        return STATUS_INSUFFICIENT_RESOURCES;
	}
    // Pass request to Pdo, always wait for completion routine
    status = IoCallDriver(Pdo, irp);

    if (status == STATUS_PENDING) {
        // Wait for the IRP to be completed, then grab the real status code
        KeWaitForSingleObject(
            &myIoctlEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        status = ioBlock.Status;
    }

    return status;
}
