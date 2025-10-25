#include <ntddk.h>

#define DEVICE_NAME  L"\\Device\\hdxDevice"
#define SYMLINK_NAME L"\\DosDevices\\hdxDevice"

// IOCTL codes
#define IOCTL_SAMPLE_GET_MESSAGE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SAMPLE_ECHO        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Forward declarations
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
NTSTATUS SampleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);
NTSTATUS SampleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp);

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);


    NTSTATUS status;
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLINK_NAME);

    status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

    if (!NT_SUCCESS(status)) 
    {
        KdPrint(("Failed to create device (0x%08X)\n", status));
        return status;
    }

    status = IoCreateSymbolicLink(&symLink, &devName);
    if (!NT_SUCCESS(status)) 
    {
        IoDeleteDevice(DeviceObject);
        KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = SampleCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SampleCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SampleDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    KdPrint(("Driver loaded successfully\n"));
    return STATUS_SUCCESS;
}

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMLINK_NAME);
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(DriverObject->DeviceObject);
    KdPrint(("Driver unloaded\n"));
}

NTSTATUS SampleCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS SampleDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR info = 0;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

    switch (stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_SAMPLE_GET_MESSAGE:
    {
        const char* message = "Hello from Kernel!";
        size_t msgLen = strlen(message) + 1;

        if (stack->Parameters.DeviceIoControl.OutputBufferLength >= msgLen) 
        {
            RtlCopyMemory(buffer, message, msgLen);
            info = msgLen;
            status = STATUS_SUCCESS;
        }
        else 
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;
    }

    case IOCTL_SAMPLE_ECHO:
    {
        ULONG inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
        ULONG outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;

        if (inputLen == 0 || outputLen == 0) 
        {
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        ULONG bytesToCopy = min(inputLen, outputLen);
        info = bytesToCopy;
        status = STATUS_SUCCESS;

        KdPrint(("Echoed message from user: %s\n", (char*)buffer));
        break;
    }

    default:
        KdPrint(("Unknown IOCTL: 0x%X\n", stack->Parameters.DeviceIoControl.IoControlCode));
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}


