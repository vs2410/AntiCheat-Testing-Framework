#include "CheatHelper.h"
#include <stdio.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <tchar.h> 
#include <iomanip>
#include "Logger.hpp"


// Check windows
#if _WIN32 || _WIN64
	#if _WIN64
		#define ENV64BIT
	#else
		#define ENV32BIT
	#endif
#endif


// WINAPI Functions

typedef LONG(NTAPI *NtSuspendProcess)(IN HANDLE ProcessHandle);
void CheatHelper::Suspend(DWORD processId)
{
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

	NtSuspendProcess pfnNtSuspendProcess = (NtSuspendProcess)GetProcAddress(GetModuleHandle("ntdll"), "NtSuspendProcess");

	pfnNtSuspendProcess(processHandle);
	CloseHandle(processHandle);
}

typedef LONG(NTAPI *NtResumeProcess)(IN HANDLE ProcessHandle);
void CheatHelper::Resume(DWORD processId)
{
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

	NtResumeProcess pfnNtResumeProcess = (NtResumeProcess)GetProcAddress(GetModuleHandle("ntdll"), "NtResumeProcess");

	pfnNtResumeProcess(processHandle);
	CloseHandle(processHandle);
}

//typedef BOOL StartServiceA(SC_HANDLE hService, DWORD dwNumServiceArgs, LPCSTR *lpServiceArgVectors);
extern "C" NTSTATUS ZwWriteVM(HANDLE hProc, PVOID pBaseAddress, PVOID pBuffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten);
extern "C" NTSTATUS ZwReadVM(HANDLE hProc, PVOID pBaseAddress, PVOID pBuffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded);


// Process Functions
DWORD CheatHelper::GetProcId(char* procName)
{
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry))
		{
			do
			{
				if (!_stricmp(procEntry.szExeFile, procName))
				{
					procId = procEntry.th32ProcessID;
					LOG_DEBUG_MSG("[+] Process Found!");
					break;
				}
			} while (Process32Next(hSnap, &procEntry));

		}
	}
	CloseHandle(hSnap);
	return procId;
}



// DEBUGING functions
void CheatHelper::ConsoleSetup(const char * title)
{
	// With this trick we'll be able to print content to the console, and if we have luck we could get information printed by the game.
	AllocConsole();
	SetConsoleTitle(title);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);
}

void CheatHelper::PrintBytes(PVOID buffer, SIZE_T  nSize)
{
	/*
	for (int i = 0; i < sizeof(buffer); i++) {
		std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)((char*)buffer)[i] << " ";
	}
	std::cout << std::endl;
	*/
	/*
	printf("[ ");
	for (size_t i = 0; i < sizeof(buffer); i++)
	{
		printf("%02x ", ((char*)buffer)[i]);
	}
	printf("]\n");
	*/
	UCHAR * uBuf = (UCHAR*)buffer;
	for (uint32_t i = 0; i != nSize; i++)
	{
		std::cout <<
			std::hex <<           // output in hex
			std::setw(2) <<       // each byte prints as two characters
			static_cast<unsigned int>(uBuf[i]) << " ";
			std::setfill('0'); // fill with 0 if not enough characters
	}
	std::cout << std::endl;
}

// Memory functions
// It will do RPM and print the memory obtained. It will return the buffer value too on the parameter buffer
int CheatHelper::RPM(HANDLE  hProcess, LPCVOID lpBaseAddress, LPVOID  lpBuffer, SIZE_T  nSize, SIZE_T  *lpNumberOfBytesRead)
{
	int status = ReadProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
	if (status == 0)
	{
		LOG_ERROR_MSG("ReadProcessMemory failed");
		return 1;
	}
	//std::cout << "[+] ReadProcessMemory: " << lpBuffer << std::endl;
	LOG_DEBUG_MSG("[+] ReadProcessMemory: ");
	CheatHelper::PrintBytes((PVOID)lpBuffer, nSize);
	return 0;
}

int CheatHelper::WPM(HANDLE  hProcess, LPVOID  lpBaseAddress, LPCVOID lpBuffer, SIZE_T  nSize, SIZE_T  *lpNumberOfBytesWritten)
{
	int status = WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
	if (status == 0)
	{
		LOG_ERROR_MSG("WriteProcessMemory failed");
		return 1;
	}
	//std::cout << "[+] WriteProcessMemory: " << lpBuffer << std::endl;
	LOG_DEBUG_MSG("[+] WriteProcessMemory: ");
	CheatHelper::PrintBytes((PVOID)lpBuffer, nSize);
	return 0;
}

int CheatHelper::NtRVM(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded)
{
	TNtReadVirtualMemory pfnNtReadVirtualMemory = (TNtReadVirtualMemory)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtReadVirtualMemory");

	auto status = pfnNtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesReaded);
	if (status != 0)
	{
		LOG_ERROR_MSG("NtReadVirtualMemory failed");
		return 1;
	}
	//std::cout << "[+] NtReadVirtualMemory: " << &Buffer << std::endl;
	LOG_DEBUG_MSG("[+] NtReadVirtualMemory: ");
	CheatHelper::PrintBytes((PVOID)Buffer, NumberOfBytesToRead);
	return 0;

}

int CheatHelper::NtWVM(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToWrite, PULONG  NumberOfBytesWritten)
{
	TNtWriteVirtualMemory pfnNtWriteVirtualMemory = (TNtWriteVirtualMemory)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtWriteVirtualMemory");
	SIZE_T stWrite = 0;

	int status = pfnNtWriteVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten);
	if (status != 0)
	{
		LOG_ERROR_MSG("NtWriteVirtualMemory Failed");
		return 1;
	}
	//std::cout << "[+] NtWriteVirtualMemory: " << &Buffer << std::endl;
	LOG_DEBUG_MSG("[+] NtWriteVirtualMemory: ");
	CheatHelper::PrintBytes((PVOID)Buffer, NumberOfBytesToWrite);
	return 0;
}


int CheatHelper::ZwRVM(HANDLE hProc, PVOID pBaseAddress, PVOID pBuffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded = NULL)
{
	auto status = ZwReadVM(hProc, pBaseAddress, pBuffer, NumberOfBytesToRead, NumberOfBytesReaded);
	if (status != 0)
	{
		LOG_ERROR_MSG("ZwReadVirtualMemory failed");
		return 1;
	}
	//std::cout << "[+] NtReadVirtualMemory: " << &Buffer << std::endl;
	LOG_DEBUG_MSG("[+] ZwReadVirtualMemory: ");
	CheatHelper::PrintBytes((PVOID)pBuffer, NumberOfBytesToRead);
	return 0;

}

int CheatHelper::ZwWVM(HANDLE hProc, PVOID pBaseAddress, PVOID pBuffer, ULONG NumberOfBytesToWrite, PULONG NumberOfBytesWritten = NULL)
{
	//SIZE_T stWrite = 0;

	int status = ZwWriteVM(hProc, pBaseAddress, pBuffer, NumberOfBytesToWrite, NumberOfBytesWritten);
	if (status != 0)
	{
		LOG_ERROR_MSG("ZwWriteVirtualMemory Failed");
		return 1;
	}
	//std::cout << "[+] NtWriteVirtualMemory: " << &Buffer << std::endl;
	LOG_DEBUG_MSG("[+] ZwWriteVirtualMemory: ");
	CheatHelper::PrintBytes((PVOID)pBuffer, NumberOfBytesToWrite);
	return 0;
}



// NamedPipe functions



// FileMapping


bool CheatHelper::checkSpinLockByte(LPVOID pFileMapMem, byte value)
{
	//Read last byte to validate if the pivot connected to the shared memory
	//We will use the last byte of the FILEMAP (FILEMAPSIZE-1)
	int n;
	BYTE init = value;
	void * dest = (void *)((intptr_t)pFileMapMem + FILEMAPSIZE - 1);
	LOG_DEBUG_MSG("[+] Waiting for pivot.");
	while (1)
	{
		n = memcmp(dest, &init, sizeof(BYTE));
		if (n == 0)
		{
			LOG_DEBUG_MSG("[+] Pivot Ready.");
			break;
		}
		else
		{
			Sleep(500);
			continue;
		}
	}
	return 0;
}

bool CheatHelper::setSpinLockByte(LPVOID pFileMapMem, byte value)
{
	BYTE init = value;
	void * dest = (void *)((intptr_t)pFileMapMem + FILEMAPSIZE - 1);
	CopyMemory(dest, &init, sizeof(BYTE));
	LOG_DEBUG_MSG("[+] Ready.");
	return 1;
}


void CheatHelper::prepareRequest(PipeMessageRequest &PMRequest)
{
	switch (PMRequest.action) {
	case 0: //Ping
	{
		LOG_DEBUG_MSG('0');
		break;
	}
	case 1: //RPM
	{
		LOG_DEBUG_MSG("[+] RPM");
		PMRequest.address = CheatHelper::RPMAddress;
		SecureZeroMemory(PMRequest.buffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::RPMBufferSize;
		break;
	}
	case 2: //WPM
	{
		LOG_DEBUG_MSG("[+] WPM");
		PMRequest.address = CheatHelper::WPMAddress;
		strncpy_s(PMRequest.buffer, CheatHelper::WPMBuffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::WPMBufferSize;
		break;
	}
	case 3: //CreatRemoteThread
	{
		LOG_DEBUG_MSG("[!] CRThread unavailable.");
		break;
	}
	case 4: //NtReadVirtualMemory
	{
		LOG_DEBUG_MSG("[+] NtReadVirtualMemory");
		PMRequest.address = CheatHelper::ntRVMAddress;
		SecureZeroMemory(PMRequest.buffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::ntRVMBufferSize;
		break;
	}
	case 5: //NtWriteVirtualMemory
	{
		LOG_DEBUG_MSG("[+] NtWriteVirtualMemory");
		PMRequest.address = CheatHelper::ntWVMAddress;
		strncpy_s(PMRequest.buffer, CheatHelper::ntWVMBuffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::ntWVMBufferSize;
		break;
	}
	case 6: //ZwReadVirtualMemory
	{
		LOG_DEBUG_MSG("[+] ZwReadVirtualMemory");
		PMRequest.address = CheatHelper::ZwRVMAddress;
		SecureZeroMemory(PMRequest.buffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::ZwRVMBufferSize;
		break;
	}
	case 7: //ZwWriteVirtualMemory
	{
		LOG_DEBUG_MSG("[+] ZwWriteVirtualMemory");
		PMRequest.address = CheatHelper::ZwWVMAddress;
		strncpy_s(PMRequest.buffer, CheatHelper::ZwWVMBuffer, BUFSIZE);
		PMRequest.size = (int)CheatHelper::ZwWVMBufferSize;
		break;
	}
	}
}


//States
bool CheatHelper::bDelayExecution;

//Addresses
intptr_t CheatHelper::RPMAddressHigh;
intptr_t CheatHelper::RPMAddressLow;
intptr_t CheatHelper::RPMAddress;
intptr_t CheatHelper::WPMAddressHigh;
intptr_t CheatHelper::WPMAddressLow;
intptr_t CheatHelper::WPMAddress;
intptr_t CheatHelper::ntRVMAddressHigh;
intptr_t CheatHelper::ntRVMAddressLow;
intptr_t CheatHelper::ntRVMAddress;
intptr_t CheatHelper::ntWVMAddressHigh;
intptr_t CheatHelper::ntWVMAddressLow;
intptr_t CheatHelper::ntWVMAddress;
intptr_t CheatHelper::ZwRVMAddressHigh;
intptr_t CheatHelper::ZwRVMAddressLow;
intptr_t CheatHelper::ZwRVMAddress;
intptr_t CheatHelper::ZwWVMAddressHigh;
intptr_t CheatHelper::ZwWVMAddressLow;
intptr_t CheatHelper::ZwWVMAddress;

DWORDLONG CheatHelper::startAddressPhyHigh;
DWORDLONG CheatHelper::startAddressPhyLow;
DWORDLONG CheatHelper::startAddressPhy;

//Handles
HANDLE CheatHelper::requestHandleNP = NULL;
HANDLE CheatHelper::requestHandleFM = NULL;
HANDLE CheatHelper::requestHandleDrv = NULL;



//Buffers
char CheatHelper::RPMBuffer[BUFSIZE];
char CheatHelper::WPMBuffer[BUFSIZE];
char CheatHelper::ntRVMBuffer[BUFSIZE];
char CheatHelper::ntWVMBuffer[BUFSIZE];
char CheatHelper::ZwRVMBuffer[BUFSIZE];
char CheatHelper::ZwWVMBuffer[BUFSIZE];
SIZE_T CheatHelper::RPMBufferSize;
SIZE_T CheatHelper::WPMBufferSize;
SIZE_T CheatHelper::ntRVMBufferSize;
SIZE_T CheatHelper::ntWVMBufferSize;
SIZE_T CheatHelper::ZwRVMBufferSize;
SIZE_T CheatHelper::ZwWVMBufferSize;

//Shared Memory
//LPTSTR CheatHelper::sPipeName;

//Strings
char CheatHelper::targetProc[];
char CheatHelper::privotProc[];
char CheatHelper::namedPipeName[];
char CheatHelper::fileMapName[];
char CheatHelper::driverName[];

// Configuration file (INI)
bool CheatHelper::loadConfig()
{
	//LPCTSTR configFile = _T(".\\..\\..\\config.ini");
	LPCTSTR configFile = _T("F:\\Recon2019\\AntiCheat-Testing-Framework\\config.ini");

	//States
	CheatHelper::bDelayExecution = (bool)GetPrivateProfileInt("Addresses", "bDelayExecution", 0, configFile);
	std::cout << "[.] bDelayExecution " << std::hex << CheatHelper::bDelayExecution << std::endl;

	//Addresses
	#if defined(ENV64BIT)
		// GetPrivateProfileInt does not allow to obtain int64 values, we need this for x64 processes
		CheatHelper::RPMAddressHigh = GetPrivateProfileInt("Addresses", "RPMAddressHigh", 0x0, configFile);
		CheatHelper::RPMAddressLow = GetPrivateProfileInt("Addresses", "RPMAddressLow", 0x0, configFile);
		CheatHelper::WPMAddressHigh = GetPrivateProfileInt("Addresses", "WPMAddressHigh", 0x0, configFile);
		CheatHelper::WPMAddressLow = GetPrivateProfileInt("Addresses", "WPMAddressLow", 0x0, configFile);
		CheatHelper::ntRVMAddressHigh = GetPrivateProfileInt("Addresses", "ntRVMAddressHigh", 0x0, configFile);
		CheatHelper::ntRVMAddressLow = GetPrivateProfileInt("Addresses", "ntRVMAddressLow", 0x0, configFile);
		CheatHelper::ntWVMAddressHigh = GetPrivateProfileInt("Addresses", "ntWVMAddressHigh", 0x0, configFile);
		CheatHelper::ntWVMAddressLow = GetPrivateProfileInt("Addresses", "ntWVMAddressLow", 0x0, configFile);
		CheatHelper::ZwRVMAddressHigh = GetPrivateProfileInt("Addresses", "ZwRVMAddressHigh", 0x0, configFile);
		CheatHelper::ZwRVMAddressLow = GetPrivateProfileInt("Addresses", "ZwRVMAddressLow", 0x0, configFile);
		CheatHelper::ZwWVMAddressHigh = GetPrivateProfileInt("Addresses", "ZwWVMAddressHigh", 0x0, configFile);
		CheatHelper::ZwWVMAddressLow = GetPrivateProfileInt("Addresses", "ZwWVMAddressLow", 0x0, configFile);
		CheatHelper::startAddressPhyHigh = GetPrivateProfileInt("Addresses", "startAddressPhyHigh", 0x0, configFile);
		CheatHelper::startAddressPhyLow = GetPrivateProfileInt("Addresses", "startAddressPhyLow", 0x0, configFile);

		CheatHelper::RPMAddress = CheatHelper::RPMAddressHigh << 32 | CheatHelper::RPMAddressLow;
		CheatHelper::WPMAddress = CheatHelper::WPMAddressHigh << 32 | CheatHelper::WPMAddressLow;
		CheatHelper::ntRVMAddress = CheatHelper::ntRVMAddressHigh << 32 | CheatHelper::ntRVMAddressLow;
		CheatHelper::ntWVMAddress = CheatHelper::ntWVMAddressHigh << 32 | CheatHelper::ntWVMAddressLow;
		CheatHelper::ZwRVMAddress = CheatHelper::ZwRVMAddressHigh << 32 | CheatHelper::ZwRVMAddressLow;
		CheatHelper::ZwWVMAddress = CheatHelper::ZwWVMAddressHigh << 32 | CheatHelper::ZwWVMAddressLow;
		CheatHelper::startAddressPhy = CheatHelper::startAddressPhyHigh << 32 | CheatHelper::startAddressPhyLow;


	#elif defined (ENV32BIT)
		CheatHelper::RPMAddress = GetPrivateProfileStruct("Addresses", "RPMAddress", (LPVOID)CheatHelper::RPMAddress, 0x8, configFile);
		CheatHelper::WPMAddress = GetPrivateProfileInt("Addresses", "WPMAddress", 0x0, configFile);
		CheatHelper::ntRVMAddress = GetPrivateProfileInt("Addresses", "ntRVMAddress", 0x0, configFile);
		CheatHelper::ntWVMAddress = GetPrivateProfileInt("Addresses", "ntWVMAddress", 0x0, configFile);
		CheatHelper::ZwRVMAddress = GetPrivateProfileInt("Addresses", "ZwRVMAddress", 0x0, configFile);
		CheatHelper::ZwWVMAddress = GetPrivateProfileInt("Addresses", "ZwWVMAddress", 0x0, configFile);
		CheatHelper::startAddressPhy = GetPrivateProfileInt("Addresses", "startAddressPhy", 0x0, configFile);
#endif


	std::cout << "[.] RPMAddress 0x" << std::hex << CheatHelper::RPMAddress << std::endl;
	std::cout << "[.] WPMAddress 0x" << std::hex << CheatHelper::WPMAddress << std::endl;
	std::cout << "[.] ntRVMAddress 0x" << std::hex << CheatHelper::ntRVMAddress << std::endl;
	std::cout << "[.] ntWVMAddress 0x" << std::hex << CheatHelper::ntWVMAddress << std::endl;
	std::cout << "[.] ZwRVMAddress 0x" << std::hex << CheatHelper::ZwRVMAddress << std::endl;
	std::cout << "[.] ZwWVMAddress 0x" << std::hex << CheatHelper::ZwWVMAddress << std::endl;
	std::cout << "[.] startAddressPhy 0x" << std::hex << CheatHelper::startAddressPhy << std::endl;


	//Handles
	CheatHelper::requestHandleNP = (HANDLE)GetPrivateProfileInt("Handles", "requestHandleNP", 0x0, configFile);
	CheatHelper::requestHandleFM = (HANDLE)GetPrivateProfileInt("Handles", "requestHandleFM", 0x0, configFile);
	CheatHelper::requestHandleDrv = (HANDLE)GetPrivateProfileInt("Handles", "requestHandleDrv", 0x0, configFile);
	std::cout << "[.] requestHandleNP 0x" << std::hex << CheatHelper::requestHandleNP << std::endl;
	std::cout << "[.] requestHandleFM 0x" << std::hex << CheatHelper::requestHandleFM << std::endl;
	std::cout << "[.] requestHandleDrv 0x" << std::hex << CheatHelper::requestHandleDrv << std::endl;


	//Buffers
	
	CheatHelper::RPMBufferSize = GetPrivateProfileInt("Buffers", "RPMBufferSize", BUFSIZE, configFile);
	CheatHelper::WPMBufferSize = GetPrivateProfileInt("Buffers", "WPMBufferSize", BUFSIZE, configFile);
	CheatHelper::ntRVMBufferSize = GetPrivateProfileInt("Buffers", "ntRVMBufferSize", BUFSIZE, configFile);
	CheatHelper::ntWVMBufferSize = GetPrivateProfileInt("Buffers", "ntWVMBufferSize", BUFSIZE, configFile);
	CheatHelper::ZwRVMBufferSize = GetPrivateProfileInt("Buffers", "ZwRVMBufferSize", BUFSIZE, configFile);
	CheatHelper::ZwWVMBufferSize = GetPrivateProfileInt("Buffers", "ZwWVMBufferSize", BUFSIZE, configFile);

	std::cout << "[.] RPMBufferSize 0x" << std::hex << CheatHelper::RPMBufferSize << std::endl;
	std::cout << "[.] WPMBufferSize 0x" << std::hex << CheatHelper::WPMBufferSize << std::endl;
	std::cout << "[.] ntRVMBufferSize 0x" << std::hex << CheatHelper::ntRVMBufferSize << std::endl;
	std::cout << "[.] ntWVMBufferSize 0x" << std::hex << CheatHelper::ntWVMBufferSize << std::endl;
	std::cout << "[.] ZwRVMBufferSize 0x" << std::hex << CheatHelper::ZwRVMBufferSize << std::endl;
	std::cout << "[.] ZwWVMBufferSize 0x" << std::hex << CheatHelper::ZwWVMBufferSize << std::endl;

	SecureZeroMemory(CheatHelper::RPMBuffer, BUFSIZE);
	SecureZeroMemory(CheatHelper::WPMBuffer, BUFSIZE);
	SecureZeroMemory(CheatHelper::ntRVMBuffer, BUFSIZE);
	SecureZeroMemory(CheatHelper::ntWVMBuffer, BUFSIZE);
	SecureZeroMemory(CheatHelper::ZwRVMBuffer, BUFSIZE);
	SecureZeroMemory(CheatHelper::ZwWVMBuffer, BUFSIZE);


	GetPrivateProfileString("Buffers", "RPMBuffer", "calc.exe", CheatHelper::RPMBuffer, (DWORD)CheatHelper::RPMBufferSize, configFile);
	GetPrivateProfileString("Buffers", "WPMBuffer", "calc.exe", CheatHelper::WPMBuffer, (DWORD)CheatHelper::WPMBufferSize, configFile);
	GetPrivateProfileString("Buffers", "ntRVMBuffer", "calc.exe", CheatHelper::ntRVMBuffer, (DWORD)CheatHelper::ntRVMBufferSize, configFile);
	GetPrivateProfileString("Buffers", "ntWVMBuffer", "calc.exe", CheatHelper::ntWVMBuffer, (DWORD)CheatHelper::ntWVMBufferSize, configFile);
	GetPrivateProfileString("Buffers", "ZwRVMBuffer", "calc.exe", CheatHelper::ZwRVMBuffer, (DWORD)CheatHelper::ZwRVMBufferSize, configFile);
	GetPrivateProfileString("Buffers", "ZwWVMBuffer", "calc.exe", CheatHelper::ZwWVMBuffer, (DWORD)CheatHelper::ZwWVMBufferSize, configFile);
	
	std::cout << "[.] RPMBuffer " << CheatHelper::RPMBuffer << std::endl;
	std::cout << "[.] WPMBuffer " << CheatHelper::WPMBuffer << std::endl;
	std::cout << "[.] ntWPMBuffer " << CheatHelper::ntRVMBuffer << std::endl;
	std::cout << "[.] ntWPMBuffer " << CheatHelper::ntWVMBuffer << std::endl;
	std::cout << "[.] ZwRPMBuffer " << CheatHelper::ZwRVMBuffer << std::endl;
	std::cout << "[.] ZwWPMBuffer " << CheatHelper::ZwWVMBuffer << std::endl;

	//Shared Memory
	//GetPrivateProfileString("SharedMemory", "sPipeName", "calc.exe", CheatHelper::sPipeName, BUFSIZE, configFile);
//	std::cout << "[.] sPipeName " << CheatHelper::sPipeName << std::endl;

	//Strings
	GetPrivateProfileString("Strings", "targetProc", "calc2.exe", CheatHelper::targetProc, BUFSIZE, configFile);
	GetPrivateProfileString("Strings", "privotProc", "calc2.exe", CheatHelper::privotProc, BUFSIZE, configFile);
	GetPrivateProfileString("Strings", "namedPipeName", "\\.\\pipe\\driverbypass", CheatHelper::namedPipeName, BUFSIZE, configFile);
	GetPrivateProfileString("Strings", "fileMapName", "Global\StealthHijacking", CheatHelper::fileMapName, BUFSIZE, configFile);
	GetPrivateProfileString("Strings", "driverName", "\\.\\GIO", CheatHelper::driverName, BUFSIZE, configFile);

	std::cout << "[.] targetProc " << CheatHelper::targetProc << std::endl;
	std::cout << "[.] privotProc " << CheatHelper::privotProc << std::endl;
	std::cout << "[.] namedPipeName " << CheatHelper::namedPipeName << std::endl;
	std::cout << "[.] fileMapName " << CheatHelper::fileMapName << std::endl;
	std::cout << "[.] driverName " << CheatHelper::driverName << std::endl;

	return 0;
}