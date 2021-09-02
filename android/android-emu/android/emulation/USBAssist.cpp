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
#include "USBAssistInterface.h"

// A clash with a windows type that is #defined causing breakage.
#undef LogSeverity
#include "android/utils/debug.h"

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
            derror("failed to get device descriptor");
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

static int findUSBDevice(uint16_t *vid, uint16_t *pid,
        uint32_t *bus, uint8_t *port, uint8_t *numPorts)
{
    libusb_device **devs = NULL;
    struct libusb_device_descriptor ddesc;
    uint8_t path[8];
    int path_depth, r, i, j, n,found=0;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    n = libusb_get_device_list(NULL, &devs);
    for (i = 0; i < n; i++) {
        path_depth = libusb_get_port_numbers(devs[i], path, sizeof(path));
        if (libusb_get_device_descriptor(devs[i], &ddesc) != 0)
            continue;
        if (ddesc.bDeviceClass == LIBUSB_CLASS_HUB)
            continue;

        if (*bus > 0 && *bus != libusb_get_bus_number(devs[i]))
            continue;
        if (*numPorts > 0) {
            if (*numPorts != path_depth)
                continue;
            for (j = 0; j < path_depth; j++)
                if (path[j] != port[j])
                    continue;
        }
        if (*vid > 0 &&
            *vid != ddesc.idVendor) {
            continue;
        }
        if (*pid > 0 &&
            *pid != ddesc.idProduct) {
            continue;
        }
        // match
        found = 1;
        if (*bus == 0)
            *bus = libusb_get_bus_number(devs[i]);
        if (*numPorts == 0) {
            *numPorts = path_depth;
            for (j = 0; j < path_depth; j++)
                port[j] = path[j];
        }
        if (*vid == 0)
            *vid = ddesc.idVendor;
        if (*pid == 0)
            *pid = ddesc.idProduct;
	break;
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);

    return found - 1;
}

static int parseUSBOptions(char *options, uint16_t *vid, uint16_t *pid,
        uint32_t *bus, uint8_t *port, uint8_t *numPorts)
{
    char *params = options;
    char *params1 = NULL;
    char *param = NULL, *context = NULL;
    char *param1 = NULL, *context1 = NULL;
    unsigned long temp = 0;
    uint8_t i = 0;

    for (param = strtok_s(params, ",", &context);  param != NULL;
            param = strtok_s(NULL, ",", &context)) {
        if (!strncmp(param, "vendorid=", 9)) {
            temp = strtol(param + 9, NULL, 16);
            if (temp > USHRT_MAX) {
                derror("Vendorid exceeds 16 bits");
                goto error_out;
            }
            *vid = (unsigned short)temp;
            continue;
        }
        if (!strncmp(param, "productid=", 10)) {
            temp = strtol(param + 10, NULL, 16);
            if (temp > USHRT_MAX) {
                derror("productid exceeds 16 bits");
                goto error_out;
            }
            *pid = (unsigned short)temp;
            continue;
        }
        if (!strncmp(param, "hostbus=", 8)) {
            *bus = strtol(param + 8, NULL, 10);
            continue;
        }
        if (!strncmp(param, "hostport=", 9)) {
            params1 = param + 9;
            for (param1 = strtok_s(params1, ".", &context1);
                    param1 != NULL && i < 6;
                    param1 = strtok_s(NULL, ",", &context1), i++) {
                temp = strtol(param1, NULL, 10);
                if (temp > UCHAR_MAX) {
                    fprintf(stderr, "Error: port number exceeds 8 bits\n");
                    goto error_out;
                }
                *(port + i) = (unsigned char)temp;
            }
            if (param1 != NULL) {
                fprintf(stderr, "Error: number of ports exceeds 6.\n");
                goto error_out;
            }
            continue;
        }
        derror("invalid parametes for USB Passthrough!!"
            "Expected Format:  vendorid=0x..., productid=0x..."
            "[,hostbus=...,hostport=...]");
        goto error_out;
    }
    *numPorts = i;
    return 0;

error_out:
    return -1;
}

static HANDLE usbAssistHandle = INVALID_HANDLE_VALUE;

int usbassist_winusb_load(char *dev_opt)
{
    uint16_t vid = 0, pid = 0;
    uint32_t bus = 0;
    uint8_t port[6] = { 0 }, num_ports = 0;
    unsigned long i = 0, nbytes = 0;
    USB_PASSTHROUGH_TARGET target = { 0 };
    BOOL success;

    if (parseUSBOptions(dev_opt, &vid, &pid, &bus, (uint8_t *)port, &num_ports))
        return -1;

    if (findUSBDevice(&vid, &pid, &bus, (uint8_t *)port, &num_ports))
        return -1;

    if (usbAssistHandle == INVALID_HANDLE_VALUE &&
        (usbAssistHandle = CreateFileW(L"\\\\.\\UsbAssist",
        GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        return -1;
    }

    target.bus = bus;
    target.vid = vid;
    target.pid = pid;
    for (i = 0; i < num_ports; i++)
        target.port[i] = port[i];
    target.flags = USB_PASSTHROUGH_ONESHOT;
    target.ops = USB_PASSTHROUGH_DEV_ADD;

    success = DeviceIoControl(usbAssistHandle,
        (DWORD)USBASSIST_IOCTL_DEVICE_OPS,
        &target, sizeof(target), NULL, 0, &nbytes, NULL);

    return success - 1;
}

#endif // _WIN32
