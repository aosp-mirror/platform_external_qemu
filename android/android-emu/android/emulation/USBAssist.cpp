// Copyright (C) 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>

#include <winioctl.h>
#include <usbioctl.h>
#include <SetupAPI.h>
#include <strsafe.h>

static int GetStringDescriptor(HANDLE hub, ULONG port, UCHAR index, USHORT langID, PUCHAR buf, size_t buf_size)
{
    BOOL success = 0;
    ULONG nBytes = 0;
    ULONG nBytesReturned = 0;
    PUSB_DESCRIPTOR_REQUEST stringDescReq = NULL;
    PUSB_STRING_DESCRIPTOR  stringDesc = NULL;
    UCHAR stringDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + MAXIMUM_USB_STRING_LENGTH];

    nBytes = sizeof(stringDescReqBuf);

    stringDescReq = (PUSB_DESCRIPTOR_REQUEST)stringDescReqBuf;
    stringDesc = (PUSB_STRING_DESCRIPTOR)(stringDescReq + 1);

    memset(stringDescReq, 0, nBytes);

    stringDescReq->ConnectionIndex = port;

    stringDescReq->SetupPacket.wValue = (USB_STRING_DESCRIPTOR_TYPE << 8)
        | index;
    stringDescReq->SetupPacket.wIndex = langID;
    stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    success = DeviceIoControl(hub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
        stringDescReq, nBytes, stringDescReq, nBytes, &nBytesReturned, NULL);
    if (!success)
        return 0;
    if (nBytesReturned < 2)
        return 0;
    if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
        return 0;
    if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST))
        return 0;
    if (stringDesc->bLength % 2 != 0)
        return 0;

    memcpy(buf, stringDesc, stringDesc->bLength);

    return stringDesc->bLength;
}

static PWCHAR get_hub_handle(PWCHAR hubName, uint8_t port, HANDLE* hubHandle)
{
    PWCHAR deviceName = NULL;
    PWCHAR childHubName = NULL;
    ULONG  nBytes = 0;
    BOOL success = 0;
    HRESULT hr = S_OK;
    size_t cbHeader = 0;
    size_t cbFullHubName = 0;
    size_t cbHubName = 0;
    PUSB_NODE_CONNECTION_INFORMATION connectionInfo = NULL;
    USB_NODE_CONNECTION_NAME extHubName;
    PUSB_NODE_CONNECTION_NAME extHubNameW = NULL;


    hr = StringCbLengthW(hubName, 256, &cbHubName);
    if (FAILED(hr))
        goto out1;

    // Allocate a temp buffer for the full hub device name.
    //
    hr = StringCbLengthW(L"\\\\.\\", 200, &cbHeader);
    if (FAILED(hr))
        goto out1;
    cbFullHubName = cbHeader + cbHubName + sizeof(WCHAR);
    deviceName = (PWCHAR)malloc(cbFullHubName);
    if (deviceName == NULL)
        goto out1;
    memset(deviceName, 0, cbFullHubName);

    // Create the full hub device name
    //
    hr = StringCchCopyNW(deviceName, cbFullHubName, L"\\\\.\\", cbHeader);
    if (FAILED(hr))
        goto out1;
    hr = StringCchCatNW(deviceName, cbFullHubName, hubName, cbHubName);
    if (FAILED(hr))
        goto out1;

    *hubHandle = CreateFileW(deviceName, GENERIC_WRITE, FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, 0, NULL);
    if (*hubHandle == INVALID_HANDLE_VALUE)
        goto out1;

    nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) +
        sizeof(USB_PIPE_INFO) * 30;
    connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)malloc(nBytes);
    if (connectionInfo == NULL)
        goto out1;

    connectionInfo->ConnectionIndex = port;
    success = DeviceIoControl(*hubHandle, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
        connectionInfo, nBytes, connectionInfo, nBytes, &nBytes, NULL);
    if (!success)
        goto out1;

    if (!connectionInfo->DeviceIsHub)
        goto out1;

    extHubName.ConnectionIndex = port;
    success = DeviceIoControl(*hubHandle, IOCTL_USB_GET_NODE_CONNECTION_NAME,
        &extHubName, sizeof(extHubName), &extHubName, sizeof(extHubName), &nBytes, NULL);
    if (!success)
        goto out1;

    nBytes = extHubName.ActualLength;
    if (nBytes <= sizeof(extHubName))
        goto out1;

    extHubNameW = (PUSB_NODE_CONNECTION_NAME)malloc(nBytes);
    if (extHubNameW == NULL)
        goto out1;

    extHubNameW->ConnectionIndex = port;
    success = DeviceIoControl(*hubHandle, IOCTL_USB_GET_NODE_CONNECTION_NAME,
        extHubNameW, nBytes, extHubNameW, nBytes, &nBytes, NULL);
    if (!success)
        goto out1;

    childHubName = _wcsdup(extHubNameW->NodeName);

out1:
    if (extHubNameW)
        free(extHubNameW);

    if (connectionInfo)
        free(connectionInfo);

    if (deviceName)
        free(deviceName);

    return childHubName;
}

static PWCHAR get_roothub_name(int bus)
{
    HANDLE hcd = INVALID_HANDLE_VALUE;
    HDEVINFO  hcdInfo = NULL;
    SP_DEVINFO_DATA hcdInfoData;
    SP_DEVICE_INTERFACE_DATA hcdInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W hcdDetailData = NULL;
    ULONG nbytes = 0;
    ULONG index = 0;
    BOOL success;
    PWCHAR ret = NULL;
    USB_ROOT_HUB_NAME rootHubName;
    PUSB_ROOT_HUB_NAME rootHubNameW = NULL;

    hcdInfo = SetupDiGetClassDevsW((LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
        NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    hcdInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (index = 0; SetupDiEnumDeviceInfo(hcdInfo, index, &hcdInfoData); index++)
        if (index == bus - 1)
            break;

    do {
        hcdInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        success = SetupDiEnumDeviceInterfaces(hcdInfo,
            0, (LPGUID)&GUID_CLASS_USB_HOST_CONTROLLER,
            index, &hcdInterfaceData);
        if (!success)
            break;

        success = SetupDiGetDeviceInterfaceDetailW(hcdInfo,
            &hcdInterfaceData, NULL, 0, &nbytes, NULL);

        if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            break;

        hcdDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(nbytes);
        if (hcdDetailData == NULL)
            break;
        hcdDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        success = SetupDiGetDeviceInterfaceDetailW(hcdInfo,
            &hcdInterfaceData, hcdDetailData, nbytes,
            &nbytes, NULL);
        if (!success)
            break;

        hcd = CreateFileW(hcdDetailData->DevicePath,
            GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hcd == INVALID_HANDLE_VALUE)
            break;

        success = DeviceIoControl(hcd, IOCTL_USB_GET_ROOT_HUB_NAME,
            0, 0, &rootHubName, sizeof(rootHubName), &nbytes, NULL);
        if (!success)
            break;

        nbytes = rootHubName.ActualLength;
        rootHubNameW = (PUSB_ROOT_HUB_NAME)malloc(nbytes);
        if (rootHubNameW == NULL)
            break;

        success = DeviceIoControl(hcd, IOCTL_USB_GET_ROOT_HUB_NAME,
            NULL, 0, rootHubNameW, nbytes, &nbytes, NULL);
        if (!success)
            break;

        ret = _wcsdup(rootHubNameW->RootHubName);
    } while (0);

    if (hcd != INVALID_HANDLE_VALUE)
        CloseHandle(hcd);

    if (hcdDetailData)
        free(hcdDetailData);

    if (rootHubNameW)
        free(rootHubNameW);

    SetupDiDestroyDeviceInfoList(hcdInfo);

    return ret;
}

static int get_string_descriptor_by_hub(int bus, uint8_t* path, int path_depth, uint8_t index, PWCHAR buf, int len)
{
    int i;
    PWCHAR hubName;
    HANDLE targetHubHandle = INVALID_HANDLE_VALUE;
    UCHAR targetHubPort = 0;
    USHORT targetLangID = 0;
    UCHAR stringBuf[MAXIMUM_USB_STRING_LENGTH] = { 0 };
    int r = 0;

    HANDLE* handle_table = (HANDLE*)malloc(path_depth * sizeof(HANDLE));
    if (handle_table == NULL)
        return 0;

    for (i = 0; i < path_depth; i++)
        *(handle_table + i) = INVALID_HANDLE_VALUE;

    hubName = get_roothub_name(bus);
    for (i = 0; i < path_depth; i++) {
        if (hubName == NULL)
            goto out;
        hubName = get_hub_handle(hubName, path[i], handle_table + i);
    }

    targetHubHandle = *(handle_table + path_depth - 1);
    targetHubPort = path[path_depth - 1];
    if (targetHubHandle == INVALID_HANDLE_VALUE)
        goto out;
    r = GetStringDescriptor(targetHubHandle, targetHubPort, 0, 0, stringBuf, sizeof(stringBuf));
    if (r < 2)
        return r;
    targetLangID = *(USHORT*)(stringBuf + 2);

    r = GetStringDescriptor(targetHubHandle, targetHubPort, index, targetLangID, stringBuf, sizeof(stringBuf));
    if (r < 2)
        return r;
    memcpy(buf, stringBuf + 2, r - 2);
out:
    if (handle_table) {
        for (i = 0; i < path_depth; i++)
            if (*(handle_table + i) != INVALID_HANDLE_VALUE)
            CloseHandle(*(handle_table + i));
        free(handle_table);
    }

    return r;
}

static void print_devs(libusb_device** devs)
{
    libusb_device* dev;
    libusb_device_handle* handle;
    int i = 0, j = 0;
    uint8_t path[8];
    int path_depth;
    WCHAR string[MAXIMUM_USB_STRING_LENGTH];
    uint8_t string_index[3];    // indexes of the string descriptors

    while ((dev = devs[i++]) != NULL) {
        /* Do not list HCD */
        if (!libusb_get_port_number(dev))
            continue;
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return;
        }
        /* Do not list hubs */
        if (desc.bDeviceClass == 0x9)
            continue;

        printf("VID:PID %04x:%04x (Bus %d, Port ",
            desc.idVendor, desc.idProduct,
            libusb_get_bus_number(dev));

        path_depth = libusb_get_port_numbers(dev, path, sizeof(path));
        if (path_depth > 0) {
            printf("%d", path[0]);
            for (j = 1; j < path_depth; j++)
                printf(".%d", path[j]);
            printf(")");
        }
        // Copy the string descriptors for easier parsing
        string_index[0] = desc.iManufacturer;
        string_index[1] = desc.iProduct;
        string_index[2] = desc.iSerialNumber;
        r = libusb_open(dev, &handle);
        for (j = 0; j < 3; j++) {
            wprintf(L"\n\t%s:\t", (j == 0) ? L"Manufacturer" : ((j == 1) ? L"Product" : L"SerialNumber"));
            memset(string, 0, sizeof(string));
            if (string_index[j] == 0) {
                wprintf(L"NA");
                continue;
            }
            if (r > 0 && libusb_get_string_descriptor_ascii(handle, string_index[j], (unsigned char*)string, sizeof(string)) > 0)
                printf("%s", (char*)string);
            else if (get_string_descriptor_by_hub(libusb_get_bus_number(dev), path, path_depth, string_index[j], string, sizeof(string)))
                wprintf(L"%s", string);
            else
                printf("Could not fetch description from device");
        }
        printf("\n\n");
    }
}

int listUSBDevices(void)
{
    libusb_device** devs;
    int r;
    ssize_t cnt;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0) {
        libusb_exit(NULL);
        return (int)cnt;
    }

    print_devs(devs);
    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);
    return 0;
}

#endif // _WIN32
