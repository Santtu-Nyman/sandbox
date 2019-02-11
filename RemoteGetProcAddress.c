#define WIN32_LEAN_AND_MEAN
#include "RemoteGetProcAddress.h"

static DWORD ReadOnlyMapFile(const WCHAR* FileName, SIZE_T* FileSize, const void** FileData)
{
	DWORD Error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE File = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (File != INVALID_HANDLE_VALUE)
	{
		ULONGLONG RawFileSize;
		if (GetFileSizeEx(File, (LARGE_INTEGER*)&RawFileSize))
		{
			if (RawFileSize < (ULONGLONG)((SIZE_T)~0))
			{
				HANDLE Mapping = CreateFileMappingW(File, 0, PAGE_READONLY, (DWORD)(RawFileSize >> 32), (DWORD)RawFileSize, 0);
				if (Mapping)
				{
					const void* View = (const void*)MapViewOfFile(Mapping, FILE_MAP_READ, 0, 0, (SIZE_T)RawFileSize);
					if (View)
					{
						CloseHandle(Mapping);
						CloseHandle(File);
						*FileSize = (SIZE_T)RawFileSize;
						*FileData = View;
						return 0;
					}
					else
						Error = GetLastError();
					CloseHandle(Mapping);
				}
				else
					Error = GetLastError();
			}
			else
				Error = ERROR_FILE_TOO_LARGE;
		}
		else
			Error = GetLastError();
		CloseHandle(File);
	}
	else
		Error = GetLastError();
	return Error;
}

static DWORD GetProcessImageFileHeader(HANDLE hProcess, IMAGE_FILE_HEADER* pImageFileHeader)
{
	if (GetProcessId(hProcess) == GetCurrentProcessId())
	{
		const IMAGE_DOS_HEADER* CurrentModuleImageData = (const IMAGE_DOS_HEADER*)GetModuleHandleW(0);
		const IMAGE_FILE_HEADER* CurrentModuleImageFileHeader = (const IMAGE_FILE_HEADER*)((UINT_PTR)CurrentModuleImageData + (UINT_PTR)CurrentModuleImageData->e_lfanew + sizeof(DWORD));
		pImageFileHeader->Machine = CurrentModuleImageFileHeader->Machine;
		pImageFileHeader->NumberOfSections = CurrentModuleImageFileHeader->NumberOfSections;
		pImageFileHeader->TimeDateStamp = CurrentModuleImageFileHeader->TimeDateStamp;
		pImageFileHeader->PointerToSymbolTable = CurrentModuleImageFileHeader->PointerToSymbolTable;
		pImageFileHeader->NumberOfSymbols = CurrentModuleImageFileHeader->NumberOfSymbols;
		pImageFileHeader->SizeOfOptionalHeader = CurrentModuleImageFileHeader->SizeOfOptionalHeader;
		pImageFileHeader->Characteristics = CurrentModuleImageFileHeader->Characteristics;
		return 0;
	}
	HANDLE Heap = GetProcessHeap();
	if (!Heap)
		return GetLastError();
	DWORD ProcessImageNameLength = MAX_PATH + 1;
	WCHAR* ProcessImageName = (WCHAR*)HeapAlloc(Heap, 0, (SIZE_T)ProcessImageNameLength * sizeof(WCHAR));
	if (!ProcessImageName)
		return GetLastError();
	DWORD Error;
	for (BOOL GetProcessImageName = TRUE; GetProcessImageName;)
	{
		if (QueryFullProcessImageNameW(hProcess, 0, ProcessImageName, &ProcessImageNameLength))
			GetProcessImageName = FALSE;
		else
		{
			Error = GetLastError();
			if (Error == ERROR_INSUFFICIENT_BUFFER)
			{
				HeapFree(Heap, 0, ProcessImageName);
				if (ProcessImageNameLength + MAX_PATH < ProcessImageNameLength)
					return ERROR_UNIDENTIFIED_ERROR;
				ProcessImageNameLength += MAX_PATH;
				ProcessImageName = (WCHAR*)HeapAlloc(Heap, 0, (SIZE_T)ProcessImageNameLength * sizeof(WCHAR));
				if (!ProcessImageName)
					return GetLastError();
			}
			else
			{
				HeapFree(Heap, 0, ProcessImageName);
				return Error;
			}
		}
	}
	SIZE_T ProcessImageFileSize;
	const IMAGE_DOS_HEADER* ProcessImageFileData;
	Error = ReadOnlyMapFile(ProcessImageName, &ProcessImageFileSize, (const void**)&ProcessImageFileData);
	HeapFree(Heap, 0, ProcessImageName);
	if (Error)
		return Error;
	if (ProcessImageFileSize < sizeof(IMAGE_DOS_HEADER) ||
		ProcessImageFileData->e_magic != IMAGE_DOS_SIGNATURE ||
		ProcessImageFileSize < (SIZE_T)ProcessImageFileData->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) ||
		((const IMAGE_NT_HEADERS*)((UINT_PTR)ProcessImageFileData + (UINT_PTR)ProcessImageFileData->e_lfanew))->Signature != IMAGE_NT_SIGNATURE ||
		!(((const IMAGE_NT_HEADERS*)((UINT_PTR)ProcessImageFileData + (UINT_PTR)ProcessImageFileData->e_lfanew))->FileHeader.Characteristics | IMAGE_FILE_EXECUTABLE_IMAGE))
	{
		UnmapViewOfFile((LPCVOID)ProcessImageFileData);
		return ERROR_BAD_EXE_FORMAT;
	}
	const IMAGE_FILE_HEADER* MappedImageFileHeader = (const IMAGE_FILE_HEADER*)((UINT_PTR)ProcessImageFileData + (UINT_PTR)ProcessImageFileData->e_lfanew + sizeof(DWORD));
	pImageFileHeader->Machine = MappedImageFileHeader->Machine;
	pImageFileHeader->NumberOfSections = MappedImageFileHeader->NumberOfSections;
	pImageFileHeader->TimeDateStamp = MappedImageFileHeader->TimeDateStamp;
	pImageFileHeader->PointerToSymbolTable = MappedImageFileHeader->PointerToSymbolTable;
	pImageFileHeader->NumberOfSymbols = MappedImageFileHeader->NumberOfSymbols;
	pImageFileHeader->SizeOfOptionalHeader = MappedImageFileHeader->SizeOfOptionalHeader;
	pImageFileHeader->Characteristics = MappedImageFileHeader->Characteristics;
	UnmapViewOfFile((LPCVOID)ProcessImageFileData);
	return 0;
}

DWORD WINAPI RemoteGetModuleHandle(HANDLE hProcess, const CHAR* pModuleName, HMODULE* phModule)
{
	HANDLE Heap = GetProcessHeap();
	if (!Heap)
		return GetLastError();
	SIZE_T ProcessModuleCount = 128;
	HMODULE* ProcessModules = (HMODULE*)HeapAlloc(Heap, 0, ProcessModuleCount * sizeof(HMODULE));
	if (!ProcessModules)
		return GetLastError();
	for (BOOL (WINAPI* EnumProcessModules)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded) = (BOOL (WINAPI*)(HANDLE, HMODULE*, DWORD, LPDWORD))GetProcAddress(GetModuleHandleA("Kernel32.dll"), "K32EnumProcessModules"); EnumProcessModules;)
	{
		DWORD ModuleArraySize;
		if (EnumProcessModules(hProcess, ProcessModules, (DWORD)(ProcessModuleCount * sizeof(HMODULE)), &ModuleArraySize))
		{
			SIZE_T ModuleArrayElementCount = (SIZE_T)ModuleArraySize / sizeof(HMODULE);
			if (!(ProcessModuleCount < ModuleArrayElementCount))
			{
				ProcessModuleCount = ModuleArrayElementCount;
				EnumProcessModules = 0;
			}
			else
			{
				ProcessModuleCount = ModuleArrayElementCount;
				HeapFree(Heap, 0, ProcessModules);
				ProcessModules = (HMODULE*)HeapAlloc(Heap, 0, ProcessModuleCount * sizeof(HMODULE));
				if (!ProcessModules)
					return GetLastError();
			}
		}
		else
			return GetLastError();
	}
	SIZE_T ModuleRegionBufferSize = 0;
	LPVOID ModuleRegionBuffer;
	SIZE_T NameBufferSize = 0;
	CHAR* Name;
	SIZE_T MemoryRead;
	DWORD Error = 0;
	for (SIZE_T ModuleIndex = 0; !Error && ModuleIndex != ProcessModuleCount; ++ModuleIndex)
	{
		UINT_PTR Module = (UINT_PTR)ProcessModules[ModuleIndex];
		MEMORY_BASIC_INFORMATION ModuleRegion;
		if (VirtualQueryEx(hProcess, (LPCVOID)Module, &ModuleRegion, sizeof(MEMORY_BASIC_INFORMATION)))
		{
			UINT_PTR ModuleRegionImageBase = Module - (UINT_PTR)ModuleRegion.BaseAddress;
			if (ModuleRegion.Type == MEM_IMAGE && (ModuleRegion.Protect == PAGE_EXECUTE_READ || ModuleRegion.Protect == PAGE_EXECUTE_READWRITE || ModuleRegion.Protect == PAGE_READONLY || ModuleRegion.Protect == PAGE_READWRITE))
			{
				if (ModuleRegionBufferSize < ModuleRegion.RegionSize)
				{
					if (ModuleRegionBufferSize)
						HeapFree(Heap, 0, ModuleRegionBuffer);
					ModuleRegionBuffer = HeapAlloc(Heap, 0, ModuleRegion.RegionSize);
					if (ModuleRegionBuffer)
					{
						ModuleRegionBufferSize = HeapSize(Heap, 0, ModuleRegionBuffer);
						if (ModuleRegionBufferSize == (SIZE_T)-1)
							ModuleRegionBufferSize = ModuleRegion.RegionSize;
					}
					else
						Error = GetLastError();
				}
				if (!Error)
				{
					if (ReadProcessMemory(hProcess, ModuleRegion.BaseAddress, ModuleRegionBuffer, ModuleRegion.RegionSize, &MemoryRead) && MemoryRead == ModuleRegion.RegionSize)
					{
						if (ModuleRegion.RegionSize >= ModuleRegionImageBase + sizeof(IMAGE_DOS_HEADER))
						{
							const IMAGE_DOS_HEADER* DOSHeader = (const IMAGE_DOS_HEADER*)((UINT_PTR)ModuleRegionBuffer + ModuleRegionImageBase);
							if (DOSHeader->e_magic == IMAGE_DOS_SIGNATURE)
							{
								if (ModuleRegion.RegionSize >= ModuleRegionImageBase + (SIZE_T)DOSHeader->e_lfanew + (sizeof(IMAGE_NT_HEADERS) - ((IMAGE_NUMBEROF_DIRECTORY_ENTRIES - (IMAGE_DIRECTORY_ENTRY_EXPORT + 1)) * sizeof(IMAGE_DATA_DIRECTORY))))
								{
									const IMAGE_NT_HEADERS* NTHeader = (const IMAGE_NT_HEADERS*)((UINT_PTR)DOSHeader + (UINT_PTR)DOSHeader->e_lfanew);
									if (NTHeader->Signature == IMAGE_NT_SIGNATURE && (NTHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) && NTHeader->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress && NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress >= sizeof(IMAGE_EXPORT_DIRECTORY))
									{
										MEMORY_BASIC_INFORMATION ExportDirectoryRegion;
										if (VirtualQueryEx(hProcess, (LPCVOID)(Module + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), &ExportDirectoryRegion, sizeof(MEMORY_BASIC_INFORMATION)))
										{
											if (ExportDirectoryRegion.Type == MEM_IMAGE && (ExportDirectoryRegion.Protect == PAGE_EXECUTE_READ || ExportDirectoryRegion.Protect == PAGE_EXECUTE_READWRITE || ExportDirectoryRegion.Protect == PAGE_READONLY || ExportDirectoryRegion.Protect == PAGE_READWRITE) && (Module + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + sizeof(IMAGE_EXPORT_DIRECTORY)) <= ((UINT_PTR)ExportDirectoryRegion.BaseAddress + (UINT_PTR)ExportDirectoryRegion.RegionSize))
											{
												IMAGE_EXPORT_DIRECTORY ExportDirectory;
												if (ReadProcessMemory(hProcess, (LPCVOID)(Module + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), &ExportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY), &MemoryRead) && MemoryRead == sizeof(IMAGE_EXPORT_DIRECTORY))
												{
													MEMORY_BASIC_INFORMATION ExportNameRegion;
													if (VirtualQueryEx(hProcess, (LPCVOID)(Module + (UINT_PTR)ExportDirectory.Name), &ExportNameRegion, sizeof(MEMORY_BASIC_INFORMATION)))
													{
														if (ExportNameRegion.Type == MEM_IMAGE && (ExportNameRegion.Protect == PAGE_EXECUTE_READ || ExportNameRegion.Protect == PAGE_EXECUTE_READWRITE || ExportNameRegion.Protect == PAGE_READONLY || ExportNameRegion.Protect == PAGE_READWRITE))
														{
															SIZE_T MaximumNameLength = (SIZE_T)(((UINT_PTR)ExportNameRegion.BaseAddress + (UINT_PTR)ExportNameRegion.RegionSize) - (Module + (UINT_PTR)ExportDirectory.Name));
															if (MaximumNameLength > NameBufferSize)
															{
																if (NameBufferSize)
																	HeapFree(Heap, 0, Name);
																Name = (CHAR*)HeapAlloc(Heap, 0, MaximumNameLength);
																if (Name)
																{
																	NameBufferSize = HeapSize(Heap, 0, Name);
																	if (NameBufferSize == (SIZE_T)-1)
																		NameBufferSize = MaximumNameLength;
																}
																else
																	Error = GetLastError();
															}
															if (!Error)
															{
																if (ReadProcessMemory(hProcess, (LPVOID)(Module + (UINT_PTR)ExportDirectory.Name), Name, (SIZE_T)MaximumNameLength, &MemoryRead) && MemoryRead == MaximumNameLength)
																{
																	UINT_PTR NameLength = 0;
																	while (NameLength != MaximumNameLength && Name[NameLength])
																		++NameLength;
																	if (!Name[NameLength] && !lstrcmpiA(Name, pModuleName))
																	{
																		*phModule = (HMODULE)Module;
																		HeapFree(Heap, 0, Name);
																		HeapFree(Heap, 0, ModuleRegionBuffer);
																		HeapFree(Heap, 0, ProcessModules);
																		return 0;
																	}
																}
																else
																	Error = GetLastError();
															}
														}
													}
													else
														Error = GetLastError();
												}
												else
													Error = GetLastError();
											}
										}
										else
											Error = GetLastError();
									}
								}
							}
						}
					}
					else
						Error = GetLastError();
				}
			}
		}
		else
			Error = GetLastError();
	}
	if (!Error)
		Error = ERROR_NOT_FOUND;
	if (NameBufferSize)
		HeapFree(Heap, 0, Name);
	if (ModuleRegionBufferSize)
		HeapFree(Heap, 0, ModuleRegionBuffer);
	HeapFree(Heap, 0, ProcessModules);
	return Error;
}

DWORD WINAPI RemoteGetProcAddress(HANDLE hProcess, HMODULE hModule, LPCSTR lpProcName, FARPROC* pProc)
{
	HANDLE Heap = GetProcessHeap();
	if (!Heap)
		return GetLastError();
	DWORD Error = 0;
	SIZE_T MemoryRead;
	MEMORY_BASIC_INFORMATION ModuleRegion;
	if (VirtualQueryEx(hProcess, (LPCVOID)hModule, &ModuleRegion, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		UINT_PTR ModuleRegionImageBase = (UINT_PTR)hModule - (UINT_PTR)ModuleRegion.BaseAddress;
		if (ModuleRegion.Type == MEM_IMAGE && (ModuleRegion.Protect == PAGE_EXECUTE_READ || ModuleRegion.Protect == PAGE_EXECUTE_READWRITE || ModuleRegion.Protect == PAGE_READONLY || ModuleRegion.Protect == PAGE_READWRITE))
		{
			LPVOID ModuleRegionBuffer = HeapAlloc(Heap, 0, ModuleRegion.RegionSize);
			if (ModuleRegionBuffer)
			{
				if (ReadProcessMemory(hProcess, ModuleRegion.BaseAddress, ModuleRegionBuffer, ModuleRegion.RegionSize, &MemoryRead) && MemoryRead == ModuleRegion.RegionSize)
				{
					if (ModuleRegion.RegionSize >= ModuleRegionImageBase + sizeof(IMAGE_DOS_HEADER))
					{
						const IMAGE_DOS_HEADER* DOSHeader = (const IMAGE_DOS_HEADER*)((UINT_PTR)ModuleRegionBuffer + ModuleRegionImageBase);
						if (DOSHeader->e_magic == IMAGE_DOS_SIGNATURE && ModuleRegion.RegionSize >= ModuleRegionImageBase + (SIZE_T)DOSHeader->e_lfanew + (sizeof(IMAGE_NT_HEADERS) - (IMAGE_NUMBEROF_DIRECTORY_ENTRIES * sizeof(IMAGE_DATA_DIRECTORY))))
						{
							const IMAGE_NT_HEADERS* NTHeader = (const IMAGE_NT_HEADERS*)((UINT_PTR)DOSHeader + (UINT_PTR)DOSHeader->e_lfanew);
							if (NTHeader->Signature == IMAGE_NT_SIGNATURE && (NTHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
							{
								if (ModuleRegion.RegionSize >= ModuleRegionImageBase + (SIZE_T)DOSHeader->e_lfanew + (sizeof(IMAGE_NT_HEADERS) - ((IMAGE_NUMBEROF_DIRECTORY_ENTRIES - (IMAGE_DIRECTORY_ENTRY_EXPORT + 1)) * sizeof(IMAGE_DATA_DIRECTORY))) && NTHeader->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXPORT && NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress && NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress >= sizeof(IMAGE_EXPORT_DIRECTORY))
								{
									MEMORY_BASIC_INFORMATION ExportDirectoryRegion;
									if (VirtualQueryEx(hProcess, (LPCVOID)((UINT_PTR)hModule + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), &ExportDirectoryRegion, sizeof(MEMORY_BASIC_INFORMATION)))
									{
										if (ExportDirectoryRegion.Type == MEM_IMAGE && (ExportDirectoryRegion.Protect == PAGE_EXECUTE_READ || ExportDirectoryRegion.Protect == PAGE_EXECUTE_READWRITE || ExportDirectoryRegion.Protect == PAGE_READONLY || ExportDirectoryRegion.Protect == PAGE_READWRITE) && ((UINT_PTR)hModule + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + sizeof(IMAGE_EXPORT_DIRECTORY)) <= ((UINT_PTR)ExportDirectoryRegion.BaseAddress + (UINT_PTR)ExportDirectoryRegion.RegionSize))
										{
											IMAGE_EXPORT_DIRECTORY ExportDirectory;
											if (ReadProcessMemory(hProcess, (LPCVOID)((UINT_PTR)hModule + (UINT_PTR)NTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), &ExportDirectory, sizeof(IMAGE_EXPORT_DIRECTORY), &MemoryRead) && MemoryRead == sizeof(IMAGE_EXPORT_DIRECTORY))
											{
												/*
												The pointer to a null-terminated ASCII string in the export section.
												This string must be within the range that is given by the export table data directory entry.
												See Optional Header Data Directories (Image Only).
												This string gives the DLL name and the name of the export (for example, "MYDLL.expfunc") or the DLL name and the ordinal number of the export (for example, "MYDLL.#27"). 
												*/

												LPVOID ExportTableBuffer = HeapAlloc(Heap, 0, ((SIZE_T)ExportDirectory.NumberOfNames * sizeof(DWORD)) + ((SIZE_T)ExportDirectory.NumberOfFunctions * (sizeof(DWORD) + sizeof(WORD))));
												if (ExportTableBuffer)
												{
													DWORD* ProcedureNames = (DWORD*)ExportTableBuffer;
													MEMORY_BASIC_INFORMATION ProcedureNameRegion;
													if (VirtualQueryEx(hProcess, (LPCVOID)((UINT_PTR)hModule + ExportDirectory.AddressOfNames), &ProcedureNameRegion, sizeof(MEMORY_BASIC_INFORMATION)))
													{
														

														//((UINT_PTR)ProcedureNameRegion.BaseAddress + (UINT_PTR)ProcedureNameRegion.RegionSize);

														DWORD* ProcedureAddresses = (DWORD*)((UINT_PTR)ExportTableBuffer + ((SIZE_T)ExportDirectory.NumberOfNames * sizeof(DWORD)));
														WORD* ProcedureOrdinals = (WORD*)((UINT_PTR)ExportTableBuffer + ((SIZE_T)ExportDirectory.NumberOfNames * sizeof(DWORD)) + ((SIZE_T)ExportDirectory.NumberOfFunctions * sizeof(DWORD)));


													}
													else
														Error = GetLastError();

													HeapFree(Heap, 0, ExportTableBuffer);
												}
												else
													Error = GetLastError();

												DWORD* ProcedureNames = (DWORD*)((UINT_PTR)hModule + ExportDirectory.AddressOfNames);
												DWORD* ProcedureAddresses = (DWORD*)((UINT_PTR)hModule + ExportDirectory.AddressOfFunctions);
												WORD* ProcedureOrdinals = (WORD*)((UINT_PTR)hModule + ExportDirectory.AddressOfNameOrdinals);

												WORD OrdinalBase = ExportDirectory.Base;
												for (SIZE_T e = (SIZE_T)ExportDirectory.NumberOfNames, i = 0; i != e; ++i)
												{
													CHAR* ProcedureName = (CHAR*)((UINT_PTR)hModule + ProcedureNames[i]);
													WORD Ordinal = ProcedureOrdinals[i];
													FARPROC ProcedureAddress = (FARPROC)((UINT_PTR)hModule + (UINT_PTR)ProcedureAddresses[Ordinal - OrdinalBase]);

												}
											}
											else
												Error = GetLastError();
										}
										else
											Error = ERROR_NOT_FOUND;
									}
									else
										Error = GetLastError();
								}
								else
									Error = ERROR_NOT_FOUND;
							}
							else
								Error = ERROR_BAD_EXE_FORMAT;
						}
						else
							Error = ERROR_BAD_EXE_FORMAT;
					}
					else
						Error = ERROR_INVALID_PARAMETER;
				}
				else
					Error = GetLastError();
				HeapFree(Heap, 0, ModuleRegionBuffer);
			}
			else
				Error = GetLastError();
		}
		else
			Error = ERROR_INVALID_PARAMETER;
	}
	else
		Error = GetLastError();
	return Error;


	/*
	
	const IMAGE_EXPORT_DIRECTORY* export_directory = (const IMAGE_EXPORT_DIRECTORY*)((UINT_PTR)hModule + (UINT_PTR)nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	WORD ordinal_base = export_directory->Base;
	DWORD* addresses = (DWORD*)((UINT_PTR)hModule + export_directory->AddressOfFunctions);
	DWORD* names = (DWORD*)((UINT_PTR)hModule + export_directory->AddressOfNames);
	WORD* ordinals = (WORD*)((UINT_PTR)hModule + export_directory->AddressOfNameOrdinals);
	for (SIZE_T e = (SIZE_T)export_directory->NumberOfNames, i = 0; i != e; ++i)
	{
		CHAR* name = (CHAR*)((UINT_PTR)hModule + names[i]);
		WORD ordinal = ordinals[i];
		FARPROC address = (FARPROC)((UINT_PTR)hModule + (UINT_PTR)addresses[ordinal - ordinal_base]);

	}
	
	*/

	return Error;
}
