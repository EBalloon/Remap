#include <ntifs.h>

#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19042
#define WINDOWS_21H1 19043
#define WINDOWS_21H2 19044

#define print(fmt, ...) DbgPrintEx(0, 0, fmt, ##__VA_ARGS__)

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef NTSTATUS(*HookControl_t)(PDEVICE_OBJECT device, PIRP Irp);
HookControl_t OriginalPtr;

constexpr ULONG CTL_Remap = 0x0001;
constexpr ULONG CTL_Restore = 0x0002;
constexpr ULONG CTL_IsRunning = 0x0003;

extern "C" POBJECT_TYPE * IoDriverObjectType;

extern "C"
{
	NTSTATUS NTAPI PsSuspendProcess(IN PEPROCESS Process);
	NTSTATUS NTAPI PsResumeProcess(IN PEPROCESS Process);

	NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(PUNICODE_STRING ObjectPath, ULONG Attributes, PACCESS_STATE PassedAccessState, 
		ACCESS_MASK DesiredAccess, POBJECT_TYPE ObjectType, KPROCESSOR_MODE AccessMode, PVOID ParseContext, PVOID* ObjectPtr);
	NTSTATUS IoCreateDriver(
		IN  PUNICODE_STRING DriverName    OPTIONAL,
		IN  PDRIVER_INITIALIZE InitializationFunction
	);
}

struct info_t {
	ULONG TargetPid = 0;
	ULONG SourcePid = 0;
	ULONG Code = 0;
	ULONG RequestID = 0;
	bool IsRunning = false;
};

typedef struct _Remap_struct
{
	//Process
	PEPROCESS FakeProcess;
	PEPROCESS GameProcess;

	//Offset
	ULONG DirectoryTableBase;
	ULONG UserDirectoryTableBase;
	ULONG VadRoot;
	ULONG VadHint;

	ULONG64 OldDirectoryTableBase;
	ULONG64 OldUserDirectoryTableBase;
	ULONG64 OldVadRoot;
	ULONG64 OldVadHint;

} Remap_struct, * PRemap_struct;
PRemap_struct pRemap;

const NTSTATUS GetOffsets()
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	RTL_OSVERSIONINFOW ver = { 0 };
	RtlGetVersion(&ver);
	switch (ver.dwBuildNumber)
	{
	case WINDOWS_1803:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x0278;
		pRemap->VadRoot = 0x628;
		pRemap->VadHint = 0x630;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_1809:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x0278;
		pRemap->VadRoot = 0x628;
		pRemap->VadHint = 0x630;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_1903:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x280;
		pRemap->VadRoot = 0x658;
		pRemap->VadHint = 0x660;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_1909:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x0278;
		pRemap->VadRoot = 0x628;
		pRemap->VadHint = 0x630;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_2004:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x388;
		pRemap->VadRoot = 0x7d8;
		pRemap->VadHint = 0x7e0;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_20H2:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x388;
		pRemap->VadRoot = 0x7d8;
		pRemap->VadHint = 0x7e0;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_21H1:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x388;
		pRemap->VadRoot = 0x7d8;
		pRemap->VadHint = 0x7e0;
		Status = STATUS_SUCCESS;
		break;
	case WINDOWS_21H2:
		pRemap->DirectoryTableBase = 0x28;
		pRemap->UserDirectoryTableBase = 0x388;
		pRemap->VadRoot = 0x7d8;
		pRemap->VadHint = 0x7e0;
		Status = STATUS_SUCCESS;
		break;
	}

	return Status;
}

NTSTATUS Remappingprocess(ULONG TargetPid, ULONG SourcePid)
{
	pRemap = PRemap_struct(ExAllocatePool(NonPagedPool, sizeof(Remap_struct)));
	if (!pRemap) goto _Exit;

	if (!NT_SUCCESS(GetOffsets()))
		goto _Exit;

	PsLookupProcessByProcessId(HANDLE(TargetPid), &pRemap->FakeProcess);
	PsLookupProcessByProcessId(HANDLE(SourcePid), &pRemap->GameProcess);

	if (!pRemap->FakeProcess || !pRemap->GameProcess)
		goto _Exit;
	
	PsSuspendProcess(pRemap->FakeProcess);

	pRemap->OldDirectoryTableBase = *((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->DirectoryTableBase));
	pRemap->OldUserDirectoryTableBase = *((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->UserDirectoryTableBase));
	pRemap->OldVadRoot = *((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadRoot));
	pRemap->OldVadHint = *((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadHint));

	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->DirectoryTableBase)) = *((PULONG64)(uintptr_t(pRemap->GameProcess) + pRemap->DirectoryTableBase));
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->UserDirectoryTableBase)) = *((PULONG64)(uintptr_t(pRemap->GameProcess) + pRemap->UserDirectoryTableBase));
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadRoot)) = *((PULONG64)(uintptr_t(pRemap->GameProcess) + pRemap->VadRoot));
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadHint)) = *((PULONG64)(uintptr_t(pRemap->GameProcess) + pRemap->VadHint));

	return STATUS_SUCCESS;

_Exit:
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS RestoreFakeProc()
{
	if (!pRemap)
		return STATUS_UNSUCCESSFUL;

	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->DirectoryTableBase)) = pRemap->OldDirectoryTableBase;
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->UserDirectoryTableBase)) = pRemap->OldUserDirectoryTableBase;
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadRoot)) = pRemap->OldVadRoot;
	*((PULONG64)(uintptr_t(pRemap->FakeProcess) + pRemap->VadHint)) = pRemap->OldVadHint;

	PsResumeProcess(pRemap->FakeProcess);

	if (pRemap)
		ExFreePool(pRemap);

	return STATUS_SUCCESS;
}

NTSTATUS DeviceControl(PDEVICE_OBJECT device, PIRP Irp) {

	ULONG SizeIO = 0;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;

	Irp->IoStatus.Information = sizeof(info_t);
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	if (stack) {
		const auto ControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
		if (ControlCode == IOCTL_DISK_GET_DRIVE_GEOMETRY)
		{
			auto* buffer = (info_t*)Irp->AssociatedIrp.SystemBuffer;
			if (buffer && sizeof(*buffer) >= sizeof(info_t))
			{
				if (buffer->RequestID == 0xDEADFFFF)
				{
					if (buffer->Code == CTL_IsRunning) {

						buffer->IsRunning = true;
						Status = STATUS_SUCCESS;
						SizeIO = sizeof(info_t);
					}
					if (buffer->Code == CTL_Remap) {
						Status = Remappingprocess(buffer->TargetPid, buffer->SourcePid);
						SizeIO = sizeof(info_t);
					}
					if (buffer->Code == CTL_Restore) {
						Status = RestoreFakeProc();
						SizeIO = sizeof(info_t);
					}

					Irp->IoStatus.Status = Status;
					Irp->IoStatus.Information = SizeIO;
					IoCompleteRequest(Irp, IO_NO_INCREMENT);
					return Status;
				}
			}		
		}
	}

	return OriginalPtr(device, Irp);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegisteryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	UNREFERENCED_PARAMETER(pDriverObject);
	UNREFERENCED_PARAMETER(pRegisteryPath);

	PDRIVER_OBJECT DriverObject = nullptr;
	UNICODE_STRING DriverObjectName = RTL_CONSTANT_STRING(L"\\Driver\\PEAUTH");
	ObReferenceObjectByName(&DriverObjectName, (ULONG)OBJ_CASE_INSENSITIVE, (PACCESS_STATE)0, (ACCESS_MASK)0, *IoDriverObjectType, KernelMode, (PVOID)0, (PVOID*)&DriverObject);
	if (DriverObject) {
		*(PVOID*)&OriginalPtr = InterlockedExchangePointer((void**)&DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL], DeviceControl);

		return STATUS_SUCCESS;
	}

	return status;
}