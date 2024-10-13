/*
	GetSystemIdForPublisher procedure from Windows ABI Windows.System.Profile.SystemIdentification class example by Santtu S. Nyman.

	License:

		This is free and unencumbered software released into the public domain.

		Anyone is free to copy, modify, publish, use, compile, sell, or
		distribute this software, either in source code form or as a compiled
		binary, for any purpose, commercial or non-commercial, and by any
		means.

		In jurisdictions that recognize copyright laws, the author or authors
		of this software dedicate any and all copyright interest in the
		software to the public domain. We make this dedication for the benefit
		of the public at large and to the detriment of our heirs and
		successors. We intend this dedication to be an overt act of
		relinquishment in perpetuity of all present and future rights to this
		software under copyright law.

		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
		IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
		OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
		ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
		OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CINTERFACE
#define CINTERFACE
#endif // CINTERFACE
#include "GetSystemIdForPublisher.h"
#include <Windows.h>
#include <roapi.h>
#include <unknwn.h>
#include <inspectable.h>

#undef INTERFACE
#define INTERFACE _IBufferByteAccess
DECLARE_INTERFACE_IID_(_IBufferByteAccess, IUnknown, "905A0FEF-BC53-11DF-8C49-001E4FC686DA")
{
	BEGIN_INTERFACE
	STDMETHOD(QueryInterface)(THIS_ const IID * riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(Buffer)(THIS_ BYTE * *value) PURE;
	END_INTERFACE
};

#undef INTERFACE
#define INTERFACE _IBuffer
DECLARE_INTERFACE_IID_(_IBuffer, IInspectable, "905A0FE0-BC53-11DF-8C49-001E4FC686DA")
{
	BEGIN_INTERFACE
	STDMETHOD(QueryInterface)(THIS_ const IID * riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(GetIids)(THIS_ ULONG * iidCount, IID * *iids) PURE;
	STDMETHOD(GetRuntimeClassName)(THIS_ HSTRING * className) PURE;
	STDMETHOD(GetTrustLevel)(THIS_ TrustLevel * trustLevel) PURE;
	STDMETHOD(get_Capacity)(THIS_ UINT32 * value) PURE;
	STDMETHOD(get_Length)(THIS_ UINT32 * value) PURE;
	STDMETHOD(put_Length)(THIS_ UINT32 value) PURE;
	END_INTERFACE
};

typedef enum _SystemIdentificationSource _SystemIdentificationSource;
enum _SystemIdentificationSource
{
	_SystemIdentificationSource_None = 0,
	_SystemIdentificationSource_Tpm = 1,
	_SystemIdentificationSource_Uefi = 2,
	_SystemIdentificationSource_Registry = 3,
};

#undef INTERFACE
#define INTERFACE _ISystemIdentificationInfo
DECLARE_INTERFACE_IID_(_ISystemIdentificationInfo, IInspectable, "0C659E7D-C3C2-4D33-A2DF-21BC41916EB3")
{
	BEGIN_INTERFACE
	STDMETHOD(QueryInterface)(THIS_ const IID * riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(GetIids)(THIS_ ULONG * iidCount, IID * *iids) PURE;
	STDMETHOD(GetRuntimeClassName)(THIS_ HSTRING * className) PURE;
	STDMETHOD(GetTrustLevel)(THIS_ TrustLevel * trustLevel) PURE;
	STDMETHOD(get_Id)(THIS_ _IBuffer ** value) PURE;
	STDMETHOD(get_Source)(THIS_ _SystemIdentificationSource ** value) PURE;
	END_INTERFACE
};

#undef INTERFACE
#define INTERFACE _ISystemIdentificationStatics
DECLARE_INTERFACE_IID_(_ISystemIdentificationStatics, IInspectable, "5581F42A-D3DF-4D93-A37D-C41A616C6D01")
{
	BEGIN_INTERFACE
	STDMETHOD(QueryInterface)(THIS_ const IID * riid, void** ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;
	STDMETHOD(GetIids)(THIS_ ULONG * iidCount, IID * *iids) PURE;
	STDMETHOD(GetRuntimeClassName)(THIS_ HSTRING * className) PURE;
	STDMETHOD(GetTrustLevel)(THIS_ TrustLevel * trustLevel) PURE;
	STDMETHOD(GetSystemIdForPublisher)(THIS_ _ISystemIdentificationInfo * *result) PURE;
	STDMETHOD(GetSystemIdForUser)(THIS_ void* user, _ISystemIdentificationInfo * *result) PURE;
	END_INTERFACE
};

HRESULT GetSystemIdForPublisher(size_t IdBufferSize, size_t* IdSizeAddress, void* IdBuffer)
{
	const IID ISystemIdentificationStaticsIID = { 0x5581F42A, 0xD3DF, 0x4D93, 0xA3, 0x7D, 0xC4, 0x1A, 0x61, 0x6C, 0x6D, 0x01 };
	const IID IBufferByteAccessIID = { 0x905A0FEF, 0xBC53, 0x11DF, 0x8C, 0x49, 0x00, 0x1E, 0x4F, 0xC6, 0x86, 0xDA };

	HRESULT Result = S_FALSE;
	HMODULE CombaseModuleHandle = 0;
	HRESULT(WINAPI * RoInitializeProcedure)(RO_INIT_TYPE initType) = 0;
	void (WINAPI * RoUninitializeProcedure)(void) = 0;
	HRESULT(WINAPI * RoGetActivationFactoryProcedure)(HSTRING activatableClassId, const IID * iid, void** factory) = 0;
	HRESULT(WINAPI * WindowsCreateStringReferenceProcedure)(PCWSTR sourceString, UINT32 length, HSTRING_HEADER * hstringHeader, HSTRING * string) = 0;
	HRESULT RoInitializeResult = S_FALSE;
	HSTRING_HEADER ActivatableClassIdHeader;
	HSTRING ActivatableClassId = 0;
	_ISystemIdentificationStatics* SystemIdentificationStatics = 0;
	_ISystemIdentificationInfo* SystemIdentificationInfo = 0;
	_IBuffer* SystemIdBuffer = 0;
	_IBufferByteAccess* SystemIdBufferAccess = 0;
	UINT32 SystemIdLength = 0;
	BYTE* SystemId = 0;

	CombaseModuleHandle = LoadLibraryW(L"Combase.dll");
	if (!CombaseModuleHandle)
	{
		Result = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}
	RoInitializeProcedure = (HRESULT(WINAPI*)(RO_INIT_TYPE))GetProcAddress(CombaseModuleHandle, "RoInitialize");
	if (!RoInitializeProcedure)
	{
		Result = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}
	RoUninitializeProcedure = (void (WINAPI*)(void))GetProcAddress(CombaseModuleHandle, "RoUninitialize");
	if (!RoUninitializeProcedure)
	{
		Result = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}
	RoGetActivationFactoryProcedure = (HRESULT(WINAPI*)(HSTRING, const IID*, void**))GetProcAddress(CombaseModuleHandle, "RoGetActivationFactory");
	if (!RoGetActivationFactoryProcedure)
	{
		Result = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}
	WindowsCreateStringReferenceProcedure = (HRESULT(WINAPI*)(PCWSTR, UINT32, HSTRING_HEADER*, HSTRING*))GetProcAddress(CombaseModuleHandle, "WindowsCreateStringReference");
	if (!WindowsCreateStringReferenceProcedure)
	{
		Result = HRESULT_FROM_WIN32(GetLastError());
		goto Cleanup;
	}
	RoInitializeResult = RoInitializeProcedure(RO_INIT_MULTITHREADED);
	if (RoInitializeResult != S_OK && RoInitializeResult != S_FALSE && RoInitializeResult != RPC_E_CHANGED_MODE)
	{
		Result = RoInitializeResult;
		goto Cleanup;
	}
	memset(&ActivatableClassIdHeader, 0, sizeof(HSTRING_HEADER));
	Result = WindowsCreateStringReferenceProcedure(L"Windows.System.Profile.SystemIdentification", 43, &ActivatableClassIdHeader, &ActivatableClassId);
	if (Result != S_OK)
	{
		goto Cleanup;
	}
	Result = RoGetActivationFactoryProcedure(ActivatableClassId, &ISystemIdentificationStaticsIID, (void**)&SystemIdentificationStatics);
	if (Result != S_OK)
	{
		SystemIdentificationStatics = 0;
		goto Cleanup;
	}
	Result = SystemIdentificationStatics->lpVtbl->GetSystemIdForPublisher(SystemIdentificationStatics, &SystemIdentificationInfo);
	if (Result != S_OK)
	{
		SystemIdentificationInfo = 0;
		goto Cleanup;
	}
	Result = SystemIdentificationInfo->lpVtbl->get_Id(SystemIdentificationInfo, &SystemIdBuffer);
	if (Result != S_OK)
	{
		SystemIdBuffer = 0;
		goto Cleanup;
	}
	Result = SystemIdBuffer->lpVtbl->get_Length(SystemIdBuffer, &SystemIdLength);
	if (Result != S_OK)
	{
		SystemIdLength = 0;
		goto Cleanup;
	}
	if (SystemIdLength)
	{
		Result = SystemIdBuffer->lpVtbl->QueryInterface(SystemIdBuffer, &IBufferByteAccessIID, (void**)&SystemIdBufferAccess);
		if (Result != S_OK)
		{
			SystemIdBufferAccess = 0;
			goto Cleanup;
		}
		Result = SystemIdBufferAccess->lpVtbl->Buffer(SystemIdBufferAccess, &SystemId);
		if (Result != S_OK)
		{
			SystemId = 0;
			goto Cleanup;
		}
	}
	if ((size_t)SystemIdLength <= IdBufferSize)
	{
		Result = S_OK;
		for (size_t i = 0; i < (size_t)SystemIdLength; i++)
		{
			((BYTE*)IdBuffer)[i] = SystemId[i];
		}
	}
	else
	{
		Result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}

Cleanup:
	if (SystemIdBufferAccess)
	{
		SystemIdBufferAccess->lpVtbl->Release(SystemIdBufferAccess);
	}
	if (SystemIdBuffer)
	{
		SystemIdBuffer->lpVtbl->Release(SystemIdBuffer);
	}
	if (SystemIdentificationInfo)
	{
		SystemIdentificationInfo->lpVtbl->Release(SystemIdentificationInfo);
	}
	if (SystemIdentificationStatics)
	{
		SystemIdentificationStatics->lpVtbl->Release(SystemIdentificationStatics);
	}
	if (RoInitializeProcedure && RoUninitializeProcedure && (RoInitializeResult == S_OK || RoInitializeResult == S_FALSE))
	{
		RoUninitializeProcedure();
	}
	if (CombaseModuleHandle)
	{
		FreeLibrary(CombaseModuleHandle);
	}
	*IdSizeAddress = (size_t)SystemIdLength;
	return Result;
}
