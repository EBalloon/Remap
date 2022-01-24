#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include "kdmapper/kdmapper.hpp"
#include "RawData.h"
#include "lazyimporter.h"
#include "oxorany.h"

constexpr ULONG CTL_Remap = 0x0001;
constexpr ULONG CTL_Restore = 0x0002;
constexpr ULONG CTL_IsRunning = 0x0003;

class DriverCom {

	HANDLE hDevice = INVALID_HANDLE_VALUE;

	struct info_t {
		ULONG TargetPid = 0;
		ULONG SourcePid = 0;
		ULONG Code = 0;
		ULONG RequestID = 0;
		bool IsRunning = false;
	};

public:

	bool OpenDriver(const wchar_t* DeviceName) {

		hDevice = iat(CreateFileW)(DeviceName, DWORD(0), FILE_SHARE_READ | FILE_SHARE_WRITE, LPSECURITY_ATTRIBUTES(nullptr), OPEN_EXISTING, DWORD(0), HANDLE(0));
		if (hDevice == INVALID_HANDLE_VALUE) {
			return oxorany(false);
		}

		return oxorany(true);
	}

	bool DriverIsRunning(ULONG CTL_CODE) {

		info_t Info{};
		Info.Code = CTL_CODE;
		Info.RequestID = 0xDEADFFFF;
		Info.IsRunning = false;

		IO_STATUS_BLOCK IOBlock = { 0, 0 };
		auto status = iat(NtDeviceIoControlFile)(hDevice, HANDLE(0), PIO_APC_ROUTINE(0), PVOID(0), &IOBlock, IOCTL_DISK_GET_DRIVE_GEOMETRY, &Info, sizeof(info_t), &Info, sizeof(info_t));
		return Info.IsRunning;
	}

	bool CallCode(ULONG CTL_CODE, ULONG TargetPid, ULONG SourcePid) {

		info_t Info{};
		Info.TargetPid = TargetPid;
		Info.SourcePid = SourcePid;
		Info.Code = CTL_CODE;
		Info.RequestID = oxorany(0xDEADFFFF);

		IO_STATUS_BLOCK IOBlock = { 0, 0 };
		auto status = iat(NtDeviceIoControlFile)(hDevice, HANDLE(0), PIO_APC_ROUTINE(0), PVOID(0), &IOBlock, IOCTL_DISK_GET_DRIVE_GEOMETRY, &Info, sizeof(info_t), &Info, sizeof(info_t));
		if (!NT_SUCCESS(status)) {
			return oxorany(false);
		}

		return oxorany(true);
	}

	DWORD AttachProcess(std::string processName) {

		DWORD ProcessPid = 0;
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (Process32First(snapshot, &entry) == TRUE)
		{
			while (Process32Next(snapshot, &entry) == TRUE)
			{
				if (strcmp(entry.szExeFile, processName.c_str()) == 0)
				{
					ProcessPid = entry.th32ProcessID;
					break;
				}
			}
		}

		CloseHandle(snapshot);
		return ProcessPid;
	}
};

HANDLE iqvw64e_device_handle;
LONG WINAPI SimplestCrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
		std::cout << "[!!] Crash at addr 0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress << L" by 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::endl;
	else
		std::cout << "[!!] Crash" << std::endl;

	if (iqvw64e_device_handle)
		intel_driver::Unload(iqvw64e_device_handle);

	return EXCEPTION_EXECUTE_HANDLER;
}

int GetParam(const int argc, char** argv, const char* param) {
	size_t plen = strlen(param);
	for (int i = 1; i < argc; i++) {
		if (strlen(argv[i]) == plen && strcmp(argv[i], param) == 0) { 
			return i;
		}
	}
	return -1;
}

int main(int argc, char* argv[])
{
	if (argc < 4) {
		std::cout << oxorany("Usage: FakeProc GameProc Remap/Restore") << std::endl;
		system("pause");
		return oxorany(0);
	}

	bool Isret = false;
	DriverCom Common = DriverCom();

	SetUnhandledExceptionFilter(SimplestCrashHandler);

	Isret = Common.OpenDriver(oxorany(L"\\\\.\\PEAUTH"));
	if (!Isret)
	{ 
		std::cout << oxorany("[-] Failed to Open Driver") << std::endl;
		system("pause");
		return oxorany(0);
	}

	if (!Common.DriverIsRunning(CTL_IsRunning))
	{
		std::cout << oxorany("[+] Loading Driver... ") << std::endl;

		iqvw64e_device_handle = intel_driver::Load();
		if (iqvw64e_device_handle == INVALID_HANDLE_VALUE) {
			system("pause");
			return oxorany(0);
		}
			
		NTSTATUS exitCode = 0;
		if (!kdmapper::MapDriver(iqvw64e_device_handle, rawData, sizeof(rawData), 0, 0, false, true, true, true, 0, &exitCode)) {
			std::cout << oxorany("[-] Failed to LoadDriver") << std::endl;
			intel_driver::Unload(iqvw64e_device_handle);
			system("pause");
			return oxorany(0);
		}

		if (!NT_SUCCESS(exitCode)) {
			std::cout << oxorany("[-] Failed to Open Driver") << std::endl;
			system("pause");
			return oxorany(0);
		}
	}
	
	if (Common.DriverIsRunning(CTL_IsRunning))
		std::cout << oxorany("[+] Driver is Running") << std::endl;

	auto notepad = Common.AttachProcess(argv[1]);
	auto ProcessHacker = Common.AttachProcess(argv[2]);
	if (!notepad || !ProcessHacker) {
		std::cout << oxorany("[-] Failed to find process pid") << std::endl;
		system("pause");
		return oxorany(0);
	}

	bool Remap = GetParam(argc, argv, oxorany("Remap")) > 0;
	bool Restore = GetParam(argc, argv, oxorany("Restore")) > 0;

	if (Remap)
	{
		Isret = Common.CallCode(CTL_Remap, notepad, ProcessHacker);
		if (!Isret) {
			std::cout << oxorany("[-] Failed to Call Code") << std::endl;
			system("pause");
			return oxorany(0);
		}

		std::cout << oxorany("[+] Process was Remaped") << std::endl;
	}
	else if (Restore)
	{
		Isret = Common.CallCode(CTL_Restore, 0, 0);
		if (!Isret) {
			std::cout << oxorany("[-] Failed to Call Code") << std::endl;
			system("pause");
			return oxorany(0);
		}

		std::cout << oxorany("[+] Process was Restored") << std::endl;
	}
	else
	{
		std::cout << oxorany("[-] Wrong param") << std::endl;
	}

	system("pause");
	return oxorany(0);
}
