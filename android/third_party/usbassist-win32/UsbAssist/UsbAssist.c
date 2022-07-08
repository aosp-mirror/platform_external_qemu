/*
 * Copyright 2021 Google LLC
 *
 * This program is ExFreePool software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <ntdef.h>
#include <ntifs.h>
#include <usb.h>
#include <usbioctl.h>
#include <initguid.h>
#include <usbiodef.h>
#include <usbspec.h>
#include <strsafe.h>
#include "UsbAssist.h"

#define USBASSIST_POOL_TAG 'ABSU'
 /* Device Name */
#define USBASSIST_DEVICE_NAME L"\\Device\\usbAssist"
#define USBASSIST_DOS_DEVICE_NAME L"\\DosDevices\\usbAssist"

#define szBusQuerryDeviceID				L"USB\\VID_18D1&PID_D400"
#define szBusQuerryHardwareIDs			L"USB\\VID_18D1&PID_D400&REV_0100\0USB\\VID_18D1&PID_D400\0\0"
#define szBusQuerryCompatibleIDs		L"USB\\Class_ff&SubClass_00&Prot_00\0USB\\Class_ff&SubClass_00\0USB\\Class_ff\0\0"
#define szDeviceTextDescription			L"Android USB Assistant"

#define USBASSIST_API_VER 1

LONG usbAssistApiVer = USBASSIST_API_VER;

LONG usbAssistOpenCnt = 0;

#define USB_HUBDRIVER_MAX	10
typedef struct __USB_HUBDRIVER_HOOK {
	PDRIVER_OBJECT drvObj;
	PFILE_OBJECT fileObj;
	PDRIVER_DISPATCH oldDispatch;
} USB_HUBDRIVER_HOOK, * PUSB_HUBDRIVER_HOOK;
USB_HUBDRIVER_HOOK usbHubDriverHooked[USB_HUBDRIVER_MAX];
int usbHubDriverHookMax = 0;
int usbHubDriverHookNoRemove = 0;
KSPIN_LOCK usbHubDriverHookLock;

typedef struct __USB_PNP_COMPLETION_CONTEXT {
	PDEVICE_OBJECT pDevObj;
	PIRP pIrp;
	IO_STACK_LOCATION  Isl;
	PIO_COMPLETION_ROUTINE OldCompletionRoutine;
	PVOID OldContext;
	UCHAR OldControl;
} USB_PNP_COMPLETION_CONTEXT, * PUSB_PNP_COMPLETION_CONTEXT;

#define USB_DEVICE_CREATED           0x1
#define USB_DEVICE_CAPTURING         0x2
#define USB_DEVICE_CAPTURED          0x3
#define USB_DEVICE_REMOVED           0x4
typedef struct __USB_DEVICE_ENTRY {
	LIST_ENTRY entry;
	PEPROCESS process;
	/* PDO representing the USB device can change and thus the pointer here
	*  may becomes invalid. 
	*  Events that trigger the change include disconnecting  and connecting,
	*  USB_CYCLE_PORT.
	*/
	PDEVICE_OBJECT pdo;
	HANDLE evt;
	USHORT vid;
	USHORT pid;
	ULONG bus;
	UCHAR port[8];
	LONG status;
	USB_TOPOLOGY_ADDRESS topo;
} USB_DEVICE_ENTRY, * PUSB_DEVICE_ENTRY;

LIST_ENTRY devListHead;
KSPIN_LOCK devListLock;

typedef struct __USB_DEVICE_PROPERTY {
	PDEVICE_OBJECT pdo;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR bClass;
	UCHAR bSubClass;
	UCHAR bProtocol;
} USB_DEVICE_PROPERTY, * PUSB_DEVICE_PROPERTY;

static NTSTATUS SyncCompletionRoutine(PDEVICE_OBJECT pDevObj, PIRP irp, PVOID context)
{
	UNREFERENCED_PARAMETER(pDevObj);
	PKEVENT kevent = (PKEVENT)context;

	if (irp->PendingReturned == TRUE)
		KeSetEvent(kevent, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS usbAssistCyclePort(PDEVICE_OBJECT pDevObj)
{
	NTSTATUS Status;
	KEVENT kEvent;
	PIO_STACK_LOCATION nextStack;

	if (!pDevObj)
		return STATUS_SUCCESS;

	PIRP irp = IoAllocateIrp(pDevObj->StackSize, FALSE);
	if (!irp)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	nextStack = IoGetNextIrpStackLocation(irp);

	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->MinorFunction = 0;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_CYCLE_PORT;

	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	IoSetCompletionRoutine(irp, SyncCompletionRoutine, (PVOID)&kEvent, TRUE, TRUE, TRUE);
	Status = IoCallDriver(pDevObj, irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	Status = irp->IoStatus.Status;

	IoFreeIrp(irp);
	return Status;
}

static NTSTATUS usbAssistGetTopology(PDEVICE_OBJECT pDevObj, PUSB_TOPOLOGY_ADDRESS pTopo)
{
	NTSTATUS Status;
	KEVENT kEvent;
	PIO_STACK_LOCATION nextStack;

	PIRP irp = IoAllocateIrp(pDevObj->StackSize, FALSE);
	if (!irp)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	nextStack = IoGetNextIrpStackLocation(irp);

	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->MinorFunction = 0;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_GET_TOPOLOGY_ADDRESS;
	nextStack->Parameters.Others.Argument1 = pTopo;

	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	IoSetCompletionRoutine(irp, SyncCompletionRoutine, (PVOID)&kEvent, TRUE, TRUE, TRUE);
	Status = IoCallDriver(pDevObj, irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	Status = irp->IoStatus.Status;

	IoFreeIrp(irp);
	return Status;
}


static NTSTATUS usbAssistSubmitUrbSync(PDEVICE_OBJECT pDevObj, PURB pURB)
{
	NTSTATUS Status;
	KEVENT kEvent;
	PIO_STACK_LOCATION nextStack;

	PIRP irp = IoAllocateIrp(pDevObj->StackSize, FALSE);
	if (!irp)
		return STATUS_INSUFFICIENT_RESOURCES;

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	nextStack = IoGetNextIrpStackLocation(irp);

	nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	nextStack->MinorFunction = 0;
	nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
	nextStack->Parameters.Others.Argument1 = pURB;

	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	IoSetCompletionRoutine(irp, SyncCompletionRoutine, (PVOID)&kEvent, TRUE, TRUE, TRUE);
	Status = IoCallDriver(pDevObj, irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&kEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);
	}

	Status = irp->IoStatus.Status;

	IoFreeIrp(irp);
	return Status;
}

static NTSTATUS usbAssistGetDescriptor(PDEVICE_OBJECT pDevObj, void* buf, UCHAR size, UCHAR type, UCHAR index, USHORT langID, ULONG dwTimeoutMs)
{
	UNREFERENCED_PARAMETER(dwTimeoutMs);
	NTSTATUS Status;

	PURB pURB = ExAllocatePoolZero(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USBASSIST_POOL_TAG);
	if (!pURB)
		return STATUS_INSUFFICIENT_RESOURCES;

	PUSB_COMMON_DESCRIPTOR pCommDesc = (PUSB_COMMON_DESCRIPTOR)buf;
	pCommDesc->bLength = size;
	pCommDesc->bDescriptorType = type;

	pURB->UrbHeader.Length = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	pURB->UrbHeader.Function = URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
	pURB->UrbControlDescriptorRequest.TransferBuffer = buf;
	pURB->UrbControlDescriptorRequest.TransferBufferLength = size;
	pURB->UrbControlDescriptorRequest.Index = index;
	pURB->UrbControlDescriptorRequest.DescriptorType = type;
	pURB->UrbControlDescriptorRequest.LanguageId = langID;;

	Status = usbAssistSubmitUrbSync(pDevObj, pURB);

	ExFreePool(pURB);

	return Status;
}

static BOOLEAN usbAssistMatchDevice(PDEVICE_OBJECT devObj)
{
	USB_TOPOLOGY_ADDRESS topo;
	NTSTATUS status = STATUS_SUCCESS;
	PLIST_ENTRY entry;
	PUSB_DEVICE_ENTRY udev;
	LONG i = 0;
	USB_DEVICE_DESCRIPTOR desc = { 0 };

	for (entry = devListHead.Flink; entry != &devListHead; entry = entry->Flink) {
		udev = CONTAINING_RECORD(entry, USB_DEVICE_ENTRY, entry);

		// Compare VID and PID
		status = usbAssistGetDescriptor(devObj, &desc, sizeof(desc),
			USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, 10000);
		if (!NT_SUCCESS(status))
			continue;
		if (desc.idProduct != udev->pid)
			continue;
		if (desc.idVendor != udev->vid)
			continue;

		// Compare topo numbers
		status = usbAssistGetTopology(devObj, &topo);
		if (!NT_SUCCESS(status))
			continue;
		if (topo.PciBusNumber != udev->topo.PciBusNumber)
			continue;
		if (topo.PciDeviceNumber != udev->topo.PciDeviceNumber)
			continue;
		if (topo.PciFunctionNumber != udev->topo.PciFunctionNumber)
			continue;
		if (topo.RootHubPortNumber != udev->topo.RootHubPortNumber)
			continue;
		for (i = 0; i < 5; i++)
			if (topo.HubPortNumber[i] != udev->topo.HubPortNumber[i])
				break;
		if (i != 5)
			continue;

		// Match
		udev->pdo = devObj;
		udev->status = USB_DEVICE_CAPTURED;
		ZwSetEvent(udev->evt, NULL);
		return TRUE;
	}
	return FALSE;
}

static int isUsbHubDriverHooked(PDRIVER_OBJECT pDrvObj)
{
	int index = 0;

	for (index = 0; index < usbHubDriverHookMax; index++) {
		if (usbHubDriverHooked[index].drvObj == pDrvObj)
			return 1;
	}
	return 0;
}

NTSTATUS usbAssistPnpHook(PDEVICE_OBJECT pDevObj, PIRP pIrp);

static NTSTATUS usbAssistInstallHook(PDRIVER_OBJECT drvObj, PFILE_OBJECT fileObj)
{
	KIRQL irql;

	KeAcquireSpinLock(&usbHubDriverHookLock, &irql);
	if (usbHubDriverHookMax == USB_HUBDRIVER_MAX) {
		KeReleaseSpinLock(&usbHubDriverHookLock, irql);
		return STATUS_INSUFFICIENT_RESOURCES;;
	}

	if (isUsbHubDriverHooked(drvObj)) {
		KeReleaseSpinLock(&usbHubDriverHookLock, irql);
		return STATUS_SUCCESS;
	}

	ObReferenceObject(fileObj);
	usbHubDriverHooked[usbHubDriverHookMax].drvObj = drvObj;
	usbHubDriverHooked[usbHubDriverHookMax].fileObj = fileObj;
	usbHubDriverHooked[usbHubDriverHookMax].oldDispatch =
		(PDRIVER_DISPATCH)InterlockedExchangePointer((PVOID*)&drvObj->MajorFunction[IRP_MJ_PNP], (PVOID)usbAssistPnpHook);
	usbHubDriverHookMax++;
	KeReleaseSpinLock(&usbHubDriverHookLock, irql);
	return STATUS_SUCCESS;
}

static PDRIVER_DISPATCH usbHubDriverOldDispatch(PDRIVER_OBJECT pDrvObj)
{
	int index = 0;

	for (index = 0; index < USB_HUBDRIVER_MAX; index++) {
		if (usbHubDriverHooked[index].drvObj == pDrvObj)
			return usbHubDriverHooked[index].oldDispatch;
	}
	return NULL;
}

static void usbAssistPdoRemoval(PDEVICE_OBJECT pDevObj)
{
	PLIST_ENTRY entry;
	PUSB_DEVICE_ENTRY udev;

	for (entry = devListHead.Flink; entry != &devListHead; entry = entry->Flink) {
		udev = CONTAINING_RECORD(entry, USB_DEVICE_ENTRY, entry);
		if (udev->pdo == pDevObj) {
			udev->pdo = NULL;
			udev->status = USB_DEVICE_REMOVED;
			return;
		}
	}

	return;
}

NTSTATUS usbAssistPnpCompletion(PDEVICE_OBJECT pDevObj, PIRP pIrp, void* pContext)
{
	PUSB_PNP_COMPLETION_CONTEXT pUsbPnpContext = pContext;
	PDEVICE_OBJECT pDevObjOrig = pUsbPnpContext->pDevObj;
	PIRP pIrpOrig = pUsbPnpContext->pIrp;
	PIO_STACK_LOCATION pIslOrig = &pUsbPnpContext->Isl;
	PIO_COMPLETION_ROUTINE OldCompletionRoutine = pUsbPnpContext->OldCompletionRoutine;
	PVOID OldContext = pUsbPnpContext->OldContext;
	UCHAR OldControl = pUsbPnpContext->OldControl;
	NTSTATUS status = STATUS_SUCCESS;

	if (pIslOrig->MinorFunction == IRP_MN_QUERY_ID) {
		if (pIrpOrig->IoStatus.Status == STATUS_SUCCESS) {
			WCHAR* pID = (WCHAR*)pIrpOrig->IoStatus.Information;
			if (pID) {
				switch (pIslOrig->Parameters.QueryId.IdType) {

				case BusQueryDeviceID:
					if (!usbAssistMatchDevice(pDevObjOrig))
						break;
					pID = ExAllocatePoolZero(PagedPool, sizeof(szBusQuerryDeviceID), USBASSIST_POOL_TAG);
					if (!pID)
						break;
					ExFreePool((PVOID)pIrpOrig->IoStatus.Information);
					RtlCopyBytes(pID, szBusQuerryDeviceID, sizeof(szBusQuerryDeviceID));
					pIrpOrig->IoStatus.Information = (ULONG_PTR)pID;
					break;

				case BusQueryHardwareIDs:
					if (!usbAssistMatchDevice(pDevObjOrig))
						break;
					pID = ExAllocatePoolZero(PagedPool, sizeof(szBusQuerryHardwareIDs), USBASSIST_POOL_TAG);
					if (!pID)
						break;
					ExFreePool((PVOID)pIrpOrig->IoStatus.Information);
					RtlCopyBytes(pID, szBusQuerryHardwareIDs, sizeof(szBusQuerryHardwareIDs));
					pIrpOrig->IoStatus.Information = (ULONG_PTR)pID;
					break;

				case BusQueryCompatibleIDs:
					if (!usbAssistMatchDevice(pDevObjOrig))
						break;
					pID = ExAllocatePoolZero(PagedPool, sizeof(szBusQuerryCompatibleIDs), USBASSIST_POOL_TAG);
					if (!pID)
						break;
					ExFreePool((PVOID)pIrpOrig->IoStatus.Information);
					RtlCopyBytes(pID, szBusQuerryCompatibleIDs, sizeof(szBusQuerryCompatibleIDs));
					pIrpOrig->IoStatus.Information = (ULONG_PTR)pID;
					break;
				}
			}
		}
	}

	if (pIslOrig->MinorFunction == IRP_MN_QUERY_DEVICE_TEXT) {
		if (pIrpOrig->IoStatus.Status == STATUS_SUCCESS) {
			WCHAR* pText = (WCHAR*)pIrpOrig->IoStatus.Information;
			if (pText) {
				switch (pIslOrig->Parameters.QueryDeviceText.DeviceTextType) {

				case DeviceTextDescription:
					if (!usbAssistMatchDevice(pDevObjOrig))
						break;
					pText = ExAllocatePoolZero(PagedPool, sizeof(szBusQuerryDeviceID), USBASSIST_POOL_TAG);
					if (!pText)
						break;
					ExFreePool((PVOID)pIrpOrig->IoStatus.Information);
					RtlCopyBytes(pText, szDeviceTextDescription, sizeof(szDeviceTextDescription));
					pIrpOrig->IoStatus.Information = (ULONG_PTR)pText;
					break;

				default:
					break;
				}
			}
		}
	}

	if (pIslOrig->MinorFunction == IRP_MN_SURPRISE_REMOVAL ||
	    pIslOrig->MinorFunction == IRP_MN_REMOVE_DEVICE) {
		if (pIrpOrig->IoStatus.Status == STATUS_SUCCESS) {
			usbAssistPdoRemoval(pDevObjOrig);
		}
	}

	if (OldCompletionRoutine && OldControl)
		status = OldCompletionRoutine(pDevObj, pIrp, OldContext);

	ExFreePool((PVOID)pContext);

	return status;
}

NTSTATUS usbAssistSetCompletionRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp, PIO_STACK_LOCATION pIsl)
{
	PUSB_PNP_COMPLETION_CONTEXT pUsbCompletionContext;

	pUsbCompletionContext = ExAllocatePoolZero(NonPagedPool, sizeof(USB_PNP_COMPLETION_CONTEXT), USBASSIST_POOL_TAG);
	if (!pUsbCompletionContext)
		return STATUS_INSUFFICIENT_RESOURCES;
	pUsbCompletionContext->Isl = *pIsl;
	pUsbCompletionContext->pDevObj = pDevObj;
	pUsbCompletionContext->pIrp = pIrp;
	pUsbCompletionContext->OldCompletionRoutine = pIsl->CompletionRoutine;
	pUsbCompletionContext->OldContext = pIsl->Context;
	pUsbCompletionContext->OldControl = pIsl->Control;

	pIsl->CompletionRoutine = usbAssistPnpCompletion;
	pIsl->Context = pUsbCompletionContext;
	pIsl->Control = SL_INVOKE_ON_CANCEL | SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR;

	return STATUS_SUCCESS;
}

NTSTATUS usbAssistPnpHook(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PDRIVER_DISPATCH oldPnpDispatch = usbHubDriverOldDispatch(pDevObj->DriverObject);
	PIO_STACK_LOCATION pIsl;

	if (!oldPnpDispatch)
		KeBugCheck(0x20001);

	pIsl = IoGetCurrentIrpStackLocation(pIrp);
	if (pIsl->MinorFunction == IRP_MN_QUERY_ID || pIsl->MinorFunction == IRP_MN_QUERY_DEVICE_TEXT ||
	    pIsl->MinorFunction == IRP_MN_SURPRISE_REMOVAL || pIsl->MinorFunction == IRP_MN_REMOVE_DEVICE) {
		usbAssistSetCompletionRoutine(pDevObj, pIrp, pIsl);
	}

	return oldPnpDispatch(pDevObj, pIrp);
}

static void usbAssistUninstallHooks()
{
	int i = 0;
	PUSB_HUBDRIVER_HOOK hook = NULL;

	if (usbHubDriverHookNoRemove)
		return;

	for (i = usbHubDriverHookMax - 1; i >= 0; i--) {
		hook = &usbHubDriverHooked[i];
		if (InterlockedCompareExchangePointer((PVOID*)&hook->drvObj->MajorFunction[IRP_MJ_PNP],
			(PVOID)hook->oldDispatch, (PVOID)usbAssistPnpHook) != (PVOID)usbAssistPnpHook) {
			// Someone else changed the hook function (possibly another driver?)
			// To be safe, we do not uninstall the hook and leave everything be
			// A system reboot should reset everything to default
			usbHubDriverHookNoRemove = 1;
			break;
		}
		ObDereferenceObject(hook->fileObj);
		RtlZeroBytes(hook, sizeof(*hook));
		usbHubDriverHookMax--;
	}
}

static NTSTATUS usbAssistInstallHooks()
{
	NTSTATUS status;
	PWSTR hubList;
	UNICODE_STRING DeviceName;
	PDEVICE_OBJECT hubDevObj;
	PFILE_OBJECT hubFileObj;
	PWSTR hubDeviceName = NULL;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	status = IoGetDeviceInterfaces(&GUID_DEVINTERFACE_USB_HUB, NULL, 0, &hubList);
	if (status != STATUS_SUCCESS)
		return status;
	if (!hubList)
		return STATUS_NO_SUCH_DEVICE;

	for (hubDeviceName = hubList; *hubDeviceName != UNICODE_NULL; hubDeviceName += wcslen(hubDeviceName) + 1) {
		RtlInitUnicodeString(&DeviceName, hubDeviceName);
		status = IoGetDeviceObjectPointer(&DeviceName, FILE_READ_DATA, &hubFileObj, &hubDevObj);
		if (status != STATUS_SUCCESS)
			break;
		status = usbAssistInstallHook(hubDevObj->DriverObject, hubFileObj);
		ObDereferenceObject(hubFileObj);
		if (status != STATUS_SUCCESS)
			break;
	}

	ExFreePool(hubList);
	if (status != STATUS_SUCCESS)
		usbAssistUninstallHooks();
	return status;
}

static NTSTATUS usbAssistQueryBusRelations(PDEVICE_OBJECT pDevObj, PFILE_OBJECT pFileObj, PDEVICE_RELATIONS *pDevRelations)
{
	KEVENT event;
	PIRP irp;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS Status;
	PIO_STACK_LOCATION pSl;

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(IRP_MJ_PNP, pDevObj, NULL, 0, NULL, 0, FALSE, &event, &ioStatus);
	if (!irp)
		return STATUS_INSUFFICIENT_RESOURCES;
	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	pSl = IoGetNextIrpStackLocation(irp);
	pSl->MajorFunction = IRP_MJ_PNP;
	pSl->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
	pSl->Parameters.QueryDeviceRelations.Type = BusRelations;
	pSl->FileObject = pFileObj;

	Status = IoCallDriver(pDevObj, irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		Status = ioStatus.Status;
	}

	if (Status == STATUS_SUCCESS)
	{
		PDEVICE_RELATIONS pRel = (PDEVICE_RELATIONS)ioStatus.Information;
		*pDevRelations = pRel;
	}

	return Status;
}

static NTSTATUS usbAssistGetNextHubName(HANDLE hub, UCHAR port, PWCHAR *nextHubName)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	HANDLE event;
	IO_STATUS_BLOCK ioStatusBlock;
	PUSB_NODE_CONNECTION_INFORMATION connectionInfo = NULL;
	ULONG nBytes = 0;
	USB_NODE_CONNECTION_NAME extHubName;
	PUSB_NODE_CONNECTION_NAME extHubNameW = NULL;
	PWCHAR ret = NULL;
	size_t cbHubName = 0;
	HRESULT hr = S_OK;

	do {
		nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
			sizeof(USB_PIPE_INFO) * 30;
		connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ExAllocatePoolZero(NonPagedPool, nBytes, USBASSIST_POOL_TAG);
		if (connectionInfo == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		connectionInfo->ConnectionIndex = port;

		status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
		if (!NT_SUCCESS(status))
			break;
		status = ZwDeviceIoControlFile(hub, event, NULL, NULL, &ioStatusBlock,
			IOCTL_USB_GET_NODE_CONNECTION_INFORMATION, connectionInfo, nBytes,
			connectionInfo, nBytes);
		if (STATUS_PENDING == status) {
			status = ZwWaitForSingleObject(event, TRUE, 0);
			if (NT_SUCCESS(status))
				status = ioStatusBlock.Status;
		}
		ZwClose(event);
		if (!NT_SUCCESS(status))
			break;

		// If the device connected to the port is an external hub, get the
		// name of the external hub and recursively enumerate it.
		//
		if (connectionInfo->DeviceIsHub) {
			// Get the length of the name of the external hub attached to the
			// specified port.
			//
			extHubName.ConnectionIndex = port;

			status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
			if (!NT_SUCCESS(status))
				break;
			status = ZwDeviceIoControlFile(hub, event, NULL, NULL, &ioStatusBlock,
				IOCTL_USB_GET_NODE_CONNECTION_NAME, &extHubName, sizeof(extHubName),
				&extHubName, sizeof(extHubName));
			if (STATUS_PENDING == status) {
				status = ZwWaitForSingleObject(event, TRUE, 0);
				if (NT_SUCCESS(status)) {
					status = ioStatusBlock.Status;
				}
			}
			ZwClose(event);
			if (!NT_SUCCESS(status))
				break;

			// Allocate space to hold the external hub name
			//
			nBytes = extHubName.ActualLength;
			if (nBytes <= sizeof(extHubName)) {
				status = STATUS_DATA_ERROR;
				break;
			}

			extHubNameW = ExAllocatePoolZero(NonPagedPool, nBytes, USBASSIST_POOL_TAG);
			if (extHubNameW == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			// Get the name of the external hub attached to the specified port
			//
			extHubNameW->ConnectionIndex = port;

			status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
			if (!NT_SUCCESS(status))
				break;
			status = ZwDeviceIoControlFile(hub, event, NULL, NULL, &ioStatusBlock,
				IOCTL_USB_GET_NODE_CONNECTION_NAME, extHubNameW, nBytes, extHubNameW, nBytes);
			if (STATUS_PENDING == status) {
				status = ZwWaitForSingleObject(event, TRUE, 0);
				ZwClose(event);
				if (NT_SUCCESS(status)) {
					status = ioStatusBlock.Status;
				}
			}
			if (!NT_SUCCESS(status)) {
				break;
			}

			hr = StringCbLengthW(extHubNameW->NodeName, 256, &cbHubName);
			if (FAILED(hr)) {
				status = STATUS_DATA_ERROR;
				break;
			}
			ret = ExAllocatePoolZero(NonPagedPool, cbHubName + sizeof(WCHAR), USBASSIST_POOL_TAG);
			if (!ret) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			StringCbCopyW(ret, cbHubName + 2, extHubNameW->NodeName);

			*nextHubName = ret;
			status = STATUS_SUCCESS;

		}
	} while (0);

	if (extHubNameW != NULL) {
		ExFreePool(extHubNameW);
		extHubNameW = NULL;
	}

	if (connectionInfo)
		ExFreePool(connectionInfo);
	return status;
}

// Given Parent HubName and Port Number, return HubHandle and child HubName
static NTSTATUS usbAssistGetHubHandleAndName(PWCHAR HubName, UCHAR port, HANDLE* hubHandle, PWCHAR* nextHubName)
{
	PWCHAR deviceName = NULL;
	UNICODE_STRING deviceNameU;
	USB_NODE_INFORMATION hubInfo;
	HANDLE event = NULL;
	OBJECT_ATTRIBUTES objAttr;
	IO_STATUS_BLOCK ioStatusBlock;
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	HRESULT hr = S_OK;
	size_t cchHeader = 0;
	size_t cchFullHubName = 0;
	size_t cbHubName = 0;

	hr = StringCbLengthW(HubName, 256, &cbHubName);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out2;
	}
	hr = StringCbLengthW(L"\\??\\", 200, &cchHeader);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out2;
	}
	cchFullHubName = cchHeader + cbHubName + sizeof(WCHAR);
	deviceName = (PWCHAR)ExAllocatePoolZero(NonPagedPool, cchFullHubName, USBASSIST_POOL_TAG);
	if (deviceName == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Out2;
	}

	// Create the full hub device name
	//
	hr = StringCchCopyNW(deviceName, cchFullHubName, L"\\??\\", cchHeader);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out2;
	}
	hr = StringCchCatNW(deviceName, cchFullHubName, HubName, cbHubName);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out2;
	}

	// Try to hub the open device
	//
	RtlInitUnicodeString(&deviceNameU, deviceName);
	InitializeObjectAttributes(&objAttr, &deviceNameU, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(hubHandle, GENERIC_WRITE,
		&objAttr, &ioStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
		FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
		goto Out2;
	if (*hubHandle == NULL) {
		status = STATUS_NO_SUCH_DEVICE;
		goto Out2;
	}

	status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
	if (!NT_SUCCESS(status))
		goto Out2;
	status = ZwDeviceIoControlFile(*hubHandle, event, NULL, NULL, &ioStatusBlock,
		IOCTL_USB_GET_NODE_INFORMATION, NULL, 0, &hubInfo, sizeof(hubInfo));
	if (STATUS_PENDING == status) {
		status = ZwWaitForSingleObject(event, TRUE, 0);
		if (NT_SUCCESS(status)) {
			status = ioStatusBlock.Status;
		}
	}
	ZwClose(event);
	if (!NT_SUCCESS(status))
		goto Out2;;

	if (port > hubInfo.u.HubInformation.HubDescriptor.bNumberOfPorts) {
		status = STATUS_NO_SUCH_DEVICE;
		goto Out2;
	}

	status = usbAssistGetNextHubName(*hubHandle, port, nextHubName);

Out2:
	if (deviceName)
		ExFreePool(deviceName);

	return status;
}


static NTSTATUS usbAssistGetRootHubName(ULONG bus, PWCHAR* name)
{
	ULONG nBytes = 0;
	USB_ROOT_HUB_NAME rootHubName = { 0 };
	PUSB_ROOT_HUB_NAME rootHubNameW = NULL;
	HANDLE event;
	PWCHAR ret = NULL;
	size_t cbRootHubName;
	HRESULT hr = S_OK;
	NTSTATUS status = STATUS_NO_SUCH_DEVICE;
	PWSTR hcdList = NULL;
	UNICODE_STRING deviceName;
	PWSTR hcdDeviceName = NULL;
	HANDLE hcdHandle = NULL;
	OBJECT_ATTRIBUTES objAttr;
	IO_STATUS_BLOCK ioStatusBlock;
	UINT32 i = 0;

	// Find the name of Roothub from bus number
	status = IoGetDeviceInterfaces(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, NULL, 0, &hcdList);
	if (status != STATUS_SUCCESS)
		return status;
	if (!hcdList)
		return STATUS_NO_SUCH_DEVICE;

	for (hcdDeviceName = hcdList, i = 0; *hcdDeviceName != UNICODE_NULL; hcdDeviceName += wcslen(hcdDeviceName) + 1, i++)
		if (i == bus - 1)
			break;
	if (hcdDeviceName == UNICODE_NULL) {
		status = STATUS_NO_SUCH_DEVICE;
		goto GetRootHubNameOut;
	}

	RtlInitUnicodeString(&deviceName, hcdDeviceName);
	InitializeObjectAttributes(&objAttr, &deviceName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	status = ZwCreateFile(&hcdHandle, GENERIC_WRITE,
		&objAttr, &ioStatusBlock, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE,
		FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
		goto GetRootHubNameOut;

	// Get the length of the name of the Root Hub attached to the
	// Host Controller
	//
	status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
	if (!NT_SUCCESS(status))
		goto GetRootHubNameOut;
	status = ZwDeviceIoControlFile(hcdHandle, event, NULL, NULL, &ioStatusBlock,
		IOCTL_USB_GET_ROOT_HUB_NAME, NULL, 0, &rootHubName, sizeof(rootHubName));
	if (STATUS_PENDING == status) {
		status = ZwWaitForSingleObject(event, TRUE, 0);
		if (NT_SUCCESS(status)) {
			status = ioStatusBlock.Status;
		}
	}
	ZwClose(event);
	if (!NT_SUCCESS(status))
		goto GetRootHubNameOut;

	// Allocate space to hold the Root Hub name
	//
	nBytes = rootHubName.ActualLength;
	rootHubNameW = ExAllocatePoolZero(NonPagedPool, nBytes, USBASSIST_POOL_TAG);
	if (rootHubNameW == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto GetRootHubNameOut;
	}
	status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
	if (!NT_SUCCESS(status))
		goto GetRootHubNameOut;
	status = ZwDeviceIoControlFile(hcdHandle, event, NULL, NULL, &ioStatusBlock,
		IOCTL_USB_GET_ROOT_HUB_NAME, NULL, 0, rootHubNameW, nBytes);
	if (STATUS_PENDING == status) {
		status = ZwWaitForSingleObject(event, TRUE, 0);
		ZwClose(event);
		if (NT_SUCCESS(status)) {
			status = ioStatusBlock.Status;
		}
	}
	if (!NT_SUCCESS(status))
		goto GetRootHubNameOut;

	hr = StringCbLengthW(rootHubNameW->RootHubName, 256, &cbRootHubName);
	if (FAILED(hr))
	{
		return STATUS_DATA_ERROR;
	}
	ret = ExAllocatePoolZero(NonPagedPool, cbRootHubName + 2, USBASSIST_POOL_TAG);
	if (!ret)
		return STATUS_INSUFFICIENT_RESOURCES;
	StringCbCopyW(ret, cbRootHubName + 2, rootHubNameW->RootHubName);
	*name = ret;

	status = STATUS_SUCCESS;

GetRootHubNameOut:
	if (rootHubNameW != NULL) {
		ExFreePool(rootHubNameW);
		rootHubNameW = NULL;
	}

	if (hcdHandle)
		ZwClose(hcdHandle);

	if (hcdList) {
		ExFreePool(hcdList);
		hcdList = NULL;
	}

	return status;
}

static NTSTATUS usbAssistCaptureDev(PUSB_DEVICE_ENTRY udev)
{
	NTSTATUS status = STATUS_NO_SUCH_DEVICE;
	HANDLE hubHandle[7] = { 0 };
	PWCHAR hubName[7] = { 0 };
	HANDLE targetHubHandle = 0;
	USHORT targetHubPort = 0;
	PDEVICE_OBJECT targetHubDevObj = NULL;
	PFILE_OBJECT targetHubFileObj = NULL;
	UNICODE_STRING targetHubNameU;
	PWCHAR deviceName = NULL;
	size_t cchHeader = 0;
	size_t cchFullHubName = 0;
	size_t cbHubName = 0;
	PDEVICE_OBJECT targetPDO = NULL;
	PDEVICE_RELATIONS devRel = NULL;
	UINT32 i = 0, j = 0;
	HRESULT hr = S_OK;
	USB_TOPOLOGY_ADDRESS topo = { 0 };
	HANDLE event = NULL;
	IO_STATUS_BLOCK ioStatusBlock;
	PUSB_NODE_CONNECTION_INFORMATION connectionInfo = NULL;
	ULONG nBytes = 0;

	// Get roothub name
	status = usbAssistGetRootHubName(udev->bus, &hubName[0]);
	if (!NT_SUCCESS(status) || hubName[0] == NULL)
		return STATUS_NO_SUCH_DEVICE;

	for (i = 0; udev->port[i]; i++) {
		status = usbAssistGetHubHandleAndName(hubName[i], (UCHAR)udev->port[i], &hubHandle[i], &hubName[i + 1]);
		if (!NT_SUCCESS(status))
			goto Out;
		if (hubHandle[i] == NULL || hubName[i + 1] == NULL)
			break;
	}

	targetHubHandle = hubHandle[i];
	targetHubPort = udev->port[i];
	if (!targetHubHandle || !targetHubPort) {
		status = STATUS_NO_SUCH_DEVICE;
		goto Out;
	}

	hr = StringCbLengthW(hubName[i], 256, &cbHubName);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out;
	}
	hr = StringCbLengthW(L"\\??\\", 200, &cchHeader);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out;
	}
	cchFullHubName = cchHeader + cbHubName + sizeof(WCHAR);
	deviceName = (PWCHAR)ExAllocatePoolZero(NonPagedPool, cchFullHubName, USBASSIST_POOL_TAG);
	if (deviceName == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Out;
	}

	hr = StringCchCopyNW(deviceName, cchFullHubName, L"\\??\\", cchHeader);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out;
	}
	hr = StringCchCatNW(deviceName, cchFullHubName, hubName[i], cbHubName);
	if (FAILED(hr)) {
		status = STATUS_DATA_ERROR;
		goto Out;
	}
	RtlInitUnicodeString(&targetHubNameU, deviceName);
	if (!NT_SUCCESS(status))
		goto Out;
	status = IoGetDeviceObjectPointer(&targetHubNameU, FILE_READ_DATA, &targetHubFileObj, &targetHubDevObj);
	if (!NT_SUCCESS(status))
		goto Out;

	status = usbAssistQueryBusRelations(targetHubDevObj, targetHubFileObj, &devRel);
	if (!NT_SUCCESS(status))
		goto Out;

	for (j = 0; j < devRel->Count; j++) {
		status = usbAssistGetTopology(devRel->Objects[j], &topo);
		if (!NT_SUCCESS(status))
			goto Out;
		if ((i == 0 && topo.RootHubPortNumber == targetHubPort) ||
			(i > 0 && topo.HubPortNumber[i - 1] == targetHubPort)) {
			nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
				sizeof(USB_PIPE_INFO) * 30;
			connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)ExAllocatePoolZero(NonPagedPool, nBytes, USBASSIST_POOL_TAG);
			if (connectionInfo == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto Out;
			}
			connectionInfo->ConnectionIndex = targetHubPort;

			status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
			if (!NT_SUCCESS(status))
				goto Out;
			status = ZwDeviceIoControlFile(targetHubHandle, event, NULL, NULL, &ioStatusBlock,
				IOCTL_USB_GET_NODE_CONNECTION_INFORMATION, connectionInfo, nBytes,
				connectionInfo, nBytes);
			if (STATUS_PENDING == status) {
				status = ZwWaitForSingleObject(event, TRUE, 0);
				if (NT_SUCCESS(status))
					status = ioStatusBlock.Status;
			}
			ZwClose(event);
			if (!NT_SUCCESS(status))
				goto Out;
			if (connectionInfo->DeviceDescriptor.idVendor == udev->vid &&
				connectionInfo->DeviceDescriptor.idProduct == udev->pid) {
				targetPDO = devRel->Objects[j];
				break;
			}
		}
	}
	if (j == devRel->Count) {
		status = STATUS_NO_SUCH_DEVICE;
		goto Out;
	}

	udev->topo = topo;

	status = usbAssistCyclePort(targetPDO);
	if (!NT_SUCCESS(status))
		goto Out;

	status = STATUS_SUCCESS;
	
Out:
	if (connectionInfo) {
		ExFreePool(connectionInfo);
		connectionInfo = NULL;
	}

	if (devRel) {
		ExFreePool(devRel);
		devRel = NULL;
	}

	if (targetHubFileObj){
		ObDereferenceObject(targetHubFileObj);
		targetHubFileObj = NULL;
		targetHubDevObj = NULL;
	}

	if (deviceName) {
		ExFreePool(deviceName);
		deviceName = NULL;
	}

	for (i = 0; i < 7; i++) {
		if (hubHandle[i])
			ZwClose(hubHandle[i]);
		if (hubName[i])
			ExFreePool(hubName[i]);
	}

	return status;
}

static NTSTATUS usbAssistCreateDev(PUSB_PASSTHROUGH_TARGET targetDev, PUSB_DEVICE_ENTRY *udev)
{
	KIRQL irql;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	HANDLE event = NULL;
	PUSB_DEVICE_ENTRY devEntry = NULL;

	devEntry = ExAllocatePoolZero(NonPagedPool, sizeof(USB_DEVICE_ENTRY), USBASSIST_POOL_TAG);
	if (!devEntry)
		goto Out;

	status = ZwCreateEvent(&event, GENERIC_ALL, 0, NotificationEvent, FALSE);
	if (!NT_SUCCESS(status))
		goto Out;

	InitializeListHead(&devEntry->entry);
	devEntry->bus = targetDev->bus;
	memcpy_s(devEntry->port, 8, targetDev->port, 8);
	devEntry->vid = targetDev->vid;
	devEntry->pid = targetDev->pid;
	devEntry->process = IoGetCurrentProcess();
	devEntry->status = USB_DEVICE_CREATED;
	devEntry->evt = event;

	KeAcquireSpinLock(&devListLock, &irql);
	InsertTailList(&devListHead, &devEntry->entry);
	KeReleaseSpinLock(&devListLock, irql);

	*udev = devEntry;
	return STATUS_SUCCESS;

Out:
    if (devEntry)
        ExFreePool(devEntry);
	return status;
}

static NTSTATUS __usbAssistRemoveDev(PUSB_DEVICE_ENTRY dev)
{
	KIRQL irql;
	NTSTATUS status = STATUS_SUCCESS;

	KeAcquireSpinLock(&devListLock, &irql);
	RemoveEntryList(&dev->entry);
	KeReleaseSpinLock(&devListLock, irql);

	if (dev->status == USB_DEVICE_CAPTURED) {
		status = usbAssistCyclePort(dev->pdo);
	}

	if (dev->evt)
		ZwClose(dev->evt);
	ExFreePool(dev);
	return status;
}

static NTSTATUS usbAssistRemoveDevs(PEPROCESS process)
{
	PLIST_ENTRY entry;
	PUSB_DEVICE_ENTRY udev;

	for (entry = devListHead.Flink; entry != &devListHead; entry = entry->Flink) {
		udev = CONTAINING_RECORD(entry, USB_DEVICE_ENTRY, entry);
		if (udev->process != process)
			continue;
		// Clean up Device Here
		__usbAssistRemoveDev(udev);
	}

	return STATUS_SUCCESS;
}

VOID usbAssistDriverUnload(PDRIVER_OBJECT pDrvObj)
{
	PDEVICE_OBJECT pDevObj = pDrvObj->DeviceObject;
	UNICODE_STRING DosDeviceName;

	RtlInitUnicodeString(&DosDeviceName, USBASSIST_DOS_DEVICE_NAME);
	IoDeleteSymbolicLink(&DosDeviceName);
	IoDeleteDevice(pDevObj);
}

NTSTATUS usbAssistDeviceClose(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevObj);

	usbAssistRemoveDevs(IoGetCurrentProcess());
	if (!InterlockedDecrement(&usbAssistOpenCnt))
		usbAssistUninstallHooks();

	// Completing the device control
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

static NTSTATUS usbAssistDeviceCreate(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DbgPrint("USBASSIST device open\n");
	UNREFERENCED_PARAMETER(pDevObj);
	NTSTATUS rc = STATUS_SUCCESS;

	if (usbAssistOpenCnt == 0) {
		rc = usbAssistInstallHooks();
		if (!NT_SUCCESS(rc))
			return rc;
	}
	InterlockedIncrement(&usbAssistOpenCnt);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

#define DEVICE_CAPTURE_TIMEOUT_VALUE -100*1000*10
#define DEVICE_CAPTURE_WAIT_MAX 20

static NTSTATUS usbAssistDeviceOps(PUSB_PASSTHROUGH_TARGET target)
{
	NTSTATUS rc = STATUS_SUCCESS; 
	PLIST_ENTRY entry;
	PUSB_DEVICE_ENTRY udev = NULL;
	LONG i = 0;
	LARGE_INTEGER timeout = { .QuadPart = DEVICE_CAPTURE_TIMEOUT_VALUE };

	if (target->flags != USB_PASSTHROUGH_ONESHOT)
		return STATUS_INVALID_PARAMETER;

	if (target->ops != USB_PASSTHROUGH_DEV_ADD &&
		target->ops != USB_PASSTHROUGH_DEV_DEL)
		return STATUS_INVALID_PARAMETER;

	// Bus Number starts with 1 and there is at least one port number
	// According to USB spec, there are at most 5 hubs (root hub excluded) from HCD to a device.
	if (target->bus == 0 || target->port[0] == 0 || target->port[7])
		return STATUS_INVALID_PARAMETER;

	switch (target->ops) {
	case USB_PASSTHROUGH_DEV_ADD:
		rc = usbAssistCreateDev(target, &udev);
		if (!NT_SUCCESS(rc))
			break;

		udev->status = USB_DEVICE_CAPTURING;
		rc = usbAssistCaptureDev(udev);
		if (!NT_SUCCESS(rc)) {
			__usbAssistRemoveDev(udev);
			break;
		}

		for (i = 0; udev->status != USB_DEVICE_CAPTURED && i < DEVICE_CAPTURE_WAIT_MAX; i++) {
			rc = ZwWaitForSingleObject(udev->evt, TRUE, &timeout);
			if (rc == STATUS_SUCCESS)
				break;
		}
		if (udev->status != USB_DEVICE_CAPTURED) {
			__usbAssistRemoveDev(udev);
			rc = STATUS_TIMEOUT;
		}
		rc = STATUS_SUCCESS;
		break;
	case USB_PASSTHROUGH_DEV_DEL:
		for (entry = devListHead.Flink; entry != NULL && entry != &devListHead; entry = entry->Flink) {
			udev = CONTAINING_RECORD(entry, USB_DEVICE_ENTRY, entry);
			if (udev->pid != target->pid)
				continue;
			if (udev->vid != target->vid)
				continue;
			if (udev->bus != target->bus)
				continue;
			if (udev->topo.RootHubPortNumber != target->port[0])
				continue;
			for (i = 1; i < 6; i++)
				if (udev->topo.HubPortNumber[i - 1] != target->port[i])
					break;
			if (i != 6)
				continue;
			__usbAssistRemoveDev(udev);
			break;
		}
		break;
	default:
		rc = STATUS_INVALID_PARAMETER;
	}

	return rc;
}

NTSTATUS usbAssistDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevObj);
	NTSTATUS rc = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION pIoStackLocation;
	ULONG ioctl;
	USB_PASSTHROUGH_TARGET* input, target;

	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	NT_ASSERT(pIoStackLocation != NULL);

	ioctl = pIoStackLocation->Parameters.DeviceIoControl.IoControlCode;

	switch (ioctl) {
	case USBASSIST_IOCTL_API_VERSION:
		if (pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LONG)) {
			rc = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		RtlCopyBytes(pIrp->AssociatedIrp.SystemBuffer, &usbAssistApiVer, sizeof(LONG));
		pIrp->IoStatus.Information = sizeof(LONG);
		rc = STATUS_SUCCESS;
		break;
	case USBASSIST_IOCTL_DEVICE_OPS:
		if (pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(target)) {
			rc = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		input = pIrp->AssociatedIrp.SystemBuffer;
		target = *input;

		rc = usbAssistDeviceOps(&target);
		break;
	default:
		break;
	}

	// Completing the device control
	pIrp->IoStatus.Status = rc;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return rc;
}

NTSTATUS usbAssistInternalDeviceControl(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDevObj);
	NTSTATUS rc = STATUS_INVALID_PARAMETER;
	PIO_STACK_LOCATION pIoStackLocation;
	ULONG ioctl;
	size_t arg;

	pIoStackLocation = IoGetCurrentIrpStackLocation(pIrp);
	NT_ASSERT(pIoStackLocation != NULL);

	ioctl = pIoStackLocation->Parameters.DeviceIoControl.IoControlCode;
	arg = (size_t)pIrp->AssociatedIrp.SystemBuffer;

	// Completing the device control
	pIrp->IoStatus.Status = rc;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return rc;
}

NTSTATUS _stdcall DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);
	UNICODE_STRING DeviceName;
	UNICODE_STRING DosDeviceName;
	PDEVICE_OBJECT pDevObj = NULL;
	NTSTATUS rc;

	KeInitializeSpinLock(&devListLock);
	InitializeListHead(&devListHead);

	RtlInitUnicodeString(&DeviceName, USBASSIST_DEVICE_NAME);

	rc = IoCreateDevice(pDrvObj,
		0x100,
		&DeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&pDevObj);

	if (!NT_SUCCESS(rc))
		goto out_ExFreePool1;

	pDrvObj->DriverUnload = usbAssistDriverUnload;
	pDrvObj->MajorFunction[IRP_MJ_CREATE] = usbAssistDeviceCreate;
	pDrvObj->MajorFunction[IRP_MJ_CLOSE] = usbAssistDeviceClose;
	pDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = usbAssistDeviceControl;
	pDrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = usbAssistInternalDeviceControl;

	RtlInitUnicodeString(&DosDeviceName, USBASSIST_DOS_DEVICE_NAME);

	rc = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
	if (!NT_SUCCESS(rc))
		goto out_ExFreePool2;

	return STATUS_SUCCESS;

out_ExFreePool2:
	IoDeleteDevice(pDevObj);
out_ExFreePool1:
	return rc;
}
