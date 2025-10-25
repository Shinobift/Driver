#include <windows.h>
#include <stdio.h>
#include <iostream>

#define IOCTL_SAMPLE_GET_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SAMPLE_ECHO        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)







int main()
{
    HANDLE hDevice = CreateFileW(L"\\\\.\\hdxDevice", GENERIC_READ | GENERIC_WRITE, 0 , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDevice == INVALID_HANDLE_VALUE) 
    {
        printf("Failed to open device (error %lu)\n", GetLastError());
        return 1;
    }

    DWORD bytesReturned;
    char buffer[128] = { 0 };

    // --- 1. Get message from driver ---
    if (DeviceIoControl(hDevice, IOCTL_SAMPLE_GET_MESSAGE, NULL, 0, buffer, sizeof(buffer), &bytesReturned, NULL))
    {
        printf("Kernel says: %s\n", buffer);
    }
    else 
    {
        printf("IOCTL_SAMPLE_GET_MESSAGE failed (error %lu)\n", GetLastError());
    }

    // --- 2. Echo message to driver ---
    const char* input = "Hello from User!";
    char echoBuffer[128] = { 0 };
    strcpy_s(echoBuffer, sizeof(echoBuffer), input);

    if (DeviceIoControl(hDevice, IOCTL_SAMPLE_ECHO,
        echoBuffer, (DWORD)(strlen(echoBuffer) + 1),
        echoBuffer, sizeof(echoBuffer),
        &bytesReturned, NULL))
    {
        printf("Echo from driver: %s\n", echoBuffer);
    }
    else 
    {
        printf("IOCTL_SAMPLE_ECHO failed (error %lu)\n", GetLastError());
    }

    CloseHandle(hDevice);
    std::cin.get();
    return 0;
}

