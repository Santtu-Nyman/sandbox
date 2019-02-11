#ifndef WS2_32_EXPORTS_H
#define WS2_32_EXPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <Ws2ipdef.h>
#include <Ws2tcpip.h>

typedef struct Ws2_32_dll_exports_t
{
	SOCKET(WSAAPI* accept)(SOCKET s, sockaddr* addr, int* addrlen);
	int (WSAAPI* bind)(SOCKET s, const struct sockaddr FAR* name, int namelen);
	int (WSAAPI* closesocket)(SOCKET s);
	int (WSAAPI* connect)(SOCKET s, const sockaddr* name, int namelen);
	VOID(WSAAPI* freeaddrinfo)(PADDRINFOA pAddrInfo);
	void (WSAAPI* _FreeAddrInfoEx)(PADDRINFOEXA pAddrInfoEx);// The function name does not have _ at the beginning. It was added because FreeAddrInfoEx is macro.
	void (WSAAPI* FreeAddrInfoExW)(PADDRINFOEXW pAddrInfoEx);
	VOID(WSAAPI* FreeAddrInfoW)(PADDRINFOW pAddrInfo);
	INT(WSAAPI* getaddrinfo)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult);
	INT(WSAAPI* GetAddrInfoExA)(PCSTR pName, PCSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXA* hints, PADDRINFOEXA* ppResult, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle);
	INT(WSAAPI* GetAddrInfoExCancel)(LPHANDLE lpHandle);
	INT(WSAAPI* GetAddrInfoExOverlappedResult)(LPOVERLAPPED lpOverlapped);
	INT(WSAAPI* GetAddrInfoExW)(PCWSTR pName, PCWSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXW* hints, PADDRINFOEXW* ppResult, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpHandle);
	hostent* (WSAAPI* gethostbyaddr)(const char* addr, int len, int type);
	hostent* (WSAAPI* gethostbyname)(const char *name);
	int (WSAAPI* gethostname)(char* name, int namelen);
	int (WSAAPI* GetHostNameW)(PWSTR name, int namelen);
	INT(WSAAPI* getnameinfo)(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PCHAR pNodeBuffer, DWORD NodeBufferSize, PCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags);
	INT(WSAAPI* GetNameInfoW)(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PWCHAR pNodeBuffer, DWORD NodeBufferSize, PWCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags);
	int (WSAAPI* getpeername)(SOCKET s, sockaddr* name, int* namelen);
	protoent* (WSAAPI* getprotobyname)(const char* name);
	protoent* (WSAAPI* getprotobynumber)(int number);
	servent* (WSAAPI* getservbyname)(const char* name, const char* proto);
	servent* (WSAAPI* getservbyport)(int port, const char* proto);
	int (WSAAPI* getsockname)(SOCKET s, sockaddr* name, int* namelen);
	int (WSAAPI* getsockopt)(SOCKET s, int level, int optname, char* optval, int* optlen);
	u_long(WSAAPI* htonl)(u_long hostlong);
	u_short(WSAAPI* htons)(u_short hostshort);
	unsigned long (WSAAPI* inet_addr)(const char* cp);
	char* (WSAAPI* inet_ntoa)(in_addr in);
	PCSTR(WSAAPI* inet_ntop)(INT Family, const VOID* pAddr, PSTR pStringBuf, size_t StringBufSize);
	INT(WSAAPI* inet_pton)(INT Family, PCSTR pszAddrString, PVOID pAddrBuf);
	PCWSTR(WSAAPI* InetNtopW)(INT Family, const VOID* pAddr, PWSTR pStringBuf, size_t StringBufSize);
	INT(WSAAPI* InetPtonW)(INT Family, PCWSTR pszAddrString, PVOID pAddrBuf);
	int (WSAAPI* ioctlsocket)(SOCKET s, long cmd, u_long* argp);
	int (WSAAPI* listen)(SOCKET s, int backlog);
	u_long(WSAAPI* ntohl)(u_long netlong);
	u_short(WSAAPI* ntohs)(u_short netshort);
	int (WSAAPI* recv)(SOCKET s, char* buf, int len, int flags);
	int (WSAAPI* recvfrom)(SOCKET s, char FAR* buf, int len, int flags, struct sockaddr FAR* from, int FAR* fromlen);
	int (WSAAPI* select)(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timeval* timeout);
	int (WSAAPI* send)(SOCKET s, const char* buf, int len, int flags);
	int (WSAAPI* sendto)(SOCKET s, const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen);
	INT(WSAAPI* SetAddrInfoExA)(PCSTR pName, PCSTR pServiceName, SOCKET_ADDRESS* pAddresses, DWORD dwAddressCount, LPBLOB lpBlob, DWORD dwFlags, DWORD dwNameSpace, LPGUID lpNspId, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle);
	INT(WSAAPI* SetAddrInfoExW)(PCWSTR pName, PCWSTR pServiceName, SOCKET_ADDRESS* pAddresses, DWORD dwAddressCount, LPBLOB lpBlob, DWORD dwFlags, DWORD dwNameSpace, LPGUID lpNspId, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle);
	int (WSAAPI* setsockopt)(SOCKET s, int level, int optname, const char FAR* optval, int optlen);
	int (WSAAPI* shutdown)(SOCKET s, int how);
	SOCKET(WSAAPI* socket)(int af, int type, int protocol);
	int (WSAAPI* WSAGetLastError)(void);
	int (WSAAPI* WSACleanup)(void);
	int (WSAAPI* WSAStartup)(WORD wVersionRequested, LPWSADATA lpWSAData);
	int (WSAAPI* __WSAFDIsSet)(SOCKET fd, fd_set* arg2);
} Ws2_32_dll_exports_t;

DWORD Ws2_32_exports_load_library(Ws2_32_dll_exports_t* exports)
{
	DWORD error = ERROR_SUCCESS;
	HMODULE module = LoadLibraryW(L"Ws2_32.dll");
	if (!module)
	{
		error = GetLastError();
		return error ? error : ERROR_UNIDENTIFIED_ERROR;
	}
	int (WSAAPI* WSAStartup)(WORD wVersionRequested, LPWSADATA lpWSAData) = (int (WSAAPI*)(WORD, LPWSADATA))GetProcAddress(module, "WSAStartup");
	WSAData startup_data;
	if (!WSAStartup || WSAStartup(MAKEWORD(2, 2), &startup_data))
	{
		error = GetLastError();
		FreeLibrary(module);
		return error ? error : ERROR_UNIDENTIFIED_ERROR;
	}
	exports->accept = (SOCKET (WSAAPI*)(SOCKET s, sockaddr* addr, int* addrlen))GetProcAddress(module, "accept");
	exports->bind = (int (WSAAPI*)(SOCKET s, const struct sockaddr FAR* name, int namelen))GetProcAddress(module, "bind");
	exports->closesocket = (int (WSAAPI*)(SOCKET s))GetProcAddress(module, "closesocket");
	exports->connect = (int (WSAAPI*)(SOCKET s, const sockaddr* name, int namelen))GetProcAddress(module, "connect");
	exports->freeaddrinfo = (VOID(WSAAPI*)(PADDRINFOA pAddrInfo))GetProcAddress(module, "freeaddrinfo");
	exports->_FreeAddrInfoEx = (void (WSAAPI*)(PADDRINFOEXA pAddrInfoEx))GetProcAddress(module, "FreeAddrInfoEx");
	exports->FreeAddrInfoExW = (void (WSAAPI*)(PADDRINFOEXW pAddrInfoEx))GetProcAddress(module, "FreeAddrInfoExW");
	exports->FreeAddrInfoW = (VOID(WSAAPI*)(PADDRINFOW pAddrInfo))GetProcAddress(module, "FreeAddrInfoW");
	exports->getaddrinfo = (INT(WSAAPI*)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult))GetProcAddress(module, "getaddrinfo");
	exports->GetAddrInfoExA = (INT(WSAAPI*)(PCSTR pName, PCSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXA* hints, PADDRINFOEXA* ppResult, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle))GetProcAddress(module, "GetAddrInfoExA");
	exports->GetAddrInfoExCancel = (INT(WSAAPI*)(LPHANDLE lpHandle))GetProcAddress(module, "GetAddrInfoExCancel");
	exports->GetAddrInfoExOverlappedResult = (INT(WSAAPI*)(LPOVERLAPPED lpOverlapped))GetProcAddress(module, "GetAddrInfoExOverlappedResult");
	exports->GetAddrInfoExW = (INT(WSAAPI*)(PCWSTR pName, PCWSTR pServiceName, DWORD dwNameSpace, LPGUID lpNspId, const ADDRINFOEXW* hints, PADDRINFOEXW* ppResult, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpHandle))GetProcAddress(module, "GetAddrInfoExW");
	exports->gethostbyaddr = (hostent* (WSAAPI*)(const char* addr, int len, int type))GetProcAddress(module, "gethostbyaddr");
	exports->gethostbyname = (hostent* (WSAAPI*)(const char *name))GetProcAddress(module, "gethostbyname");
	exports->gethostname = (int (WSAAPI*)(char* name, int namelen))GetProcAddress(module, "gethostname");
	exports->GetHostNameW = (int (WSAAPI*)(PWSTR name, int namelen))GetProcAddress(module, "GetHostNameW");
	exports->getnameinfo = (INT(WSAAPI*)(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PCHAR pNodeBuffer, DWORD NodeBufferSize, PCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags))GetProcAddress(module, "getnameinfo");
	exports->GetNameInfoW = (INT(WSAAPI*)(const SOCKADDR* pSockaddr, socklen_t SockaddrLength, PWCHAR pNodeBuffer, DWORD NodeBufferSize, PWCHAR pServiceBuffer, DWORD ServiceBufferSize, INT Flags))GetProcAddress(module, "GetNameInfoW");
	exports->getpeername = (int (WSAAPI*)(SOCKET s, sockaddr* name, int* namelen))GetProcAddress(module, "getpeername");
	exports->getprotobyname = (protoent* (WSAAPI*)(const char* name))GetProcAddress(module, "getprotobyname");
	exports->getprotobynumber = (protoent* (WSAAPI*)(int number))GetProcAddress(module, "getprotobynumber");
	exports->getservbyname = (servent* (WSAAPI*)(const char* name, const char* proto))GetProcAddress(module, "getservbyname");
	exports->getservbyport = (servent* (WSAAPI*)(int port, const char* proto))GetProcAddress(module, "getservbyport");
	exports->getsockname = (int (WSAAPI*)(SOCKET s, sockaddr* name, int* namelen))GetProcAddress(module, "getsockname");
	exports->getsockopt = (int (WSAAPI*)(SOCKET s, int level, int optname, char* optval, int* optlen))GetProcAddress(module, "getsockopt");
	exports->htonl = (u_long(WSAAPI*)(u_long hostlong))GetProcAddress(module, "htonl");
	exports->htons = (u_short(WSAAPI*)(u_short hostshort))GetProcAddress(module, "htons");
	exports->inet_addr = (unsigned long (WSAAPI*)(const char* cp))GetProcAddress(module, "inet_addr");
	exports->inet_ntoa = (char* (WSAAPI*)(in_addr in))GetProcAddress(module, "inet_ntoa");
	exports->inet_ntop = (PCSTR(WSAAPI*)(INT Family, const VOID* pAddr, PSTR pStringBuf, size_t StringBufSize))GetProcAddress(module, "inet_ntop");
	exports->inet_pton = (INT(WSAAPI*)(INT Family, PCSTR pszAddrString, PVOID pAddrBuf))GetProcAddress(module, "inet_pton");
	exports->InetNtopW = (PCWSTR(WSAAPI*)(INT Family, const VOID* pAddr, PWSTR pStringBuf, size_t StringBufSize))GetProcAddress(module, "InetNtopW");
	exports->InetPtonW = (INT(WSAAPI*)(INT Family, PCWSTR pszAddrString, PVOID pAddrBuf))GetProcAddress(module, "InetPtonW");
	exports->ioctlsocket = (int (WSAAPI*)(SOCKET s, long cmd, u_long* argp))GetProcAddress(module, "ioctlsocket");
	exports->listen = (int (WSAAPI*)(SOCKET s, int backlog))GetProcAddress(module, "listen");
	exports->ntohl = (u_long(WSAAPI*)(u_long netlong))GetProcAddress(module, "ntohl");
	exports->ntohs = (u_short(WSAAPI*)(u_short netshort))GetProcAddress(module, "ntohs");
	exports->recv = (int (WSAAPI*)(SOCKET s, char* buf, int len, int flags))GetProcAddress(module, "recv");
	exports->recvfrom = (int (WSAAPI*)(SOCKET s, char FAR* buf, int len, int flags, struct sockaddr FAR* from, int FAR* fromlen))GetProcAddress(module, "recvfrom");
	exports->select = (int (WSAAPI*)(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timeval* timeout))GetProcAddress(module, "select");
	exports->send = (int (WSAAPI*)(SOCKET s, const char* buf, int len, int flags))GetProcAddress(module, "send");
	exports->sendto = (int (WSAAPI*)(SOCKET s, const char FAR* buf, int len, int flags, const struct sockaddr FAR* to, int tolen))GetProcAddress(module, "sendto");
	exports->SetAddrInfoExA = (INT(WSAAPI*)(PCSTR pName, PCSTR pServiceName, SOCKET_ADDRESS* pAddresses, DWORD dwAddressCount, LPBLOB lpBlob, DWORD dwFlags, DWORD dwNameSpace, LPGUID lpNspId, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle))GetProcAddress(module, "SetAddrInfoExA");
	exports->SetAddrInfoExW = (INT(WSAAPI*)(PCWSTR pName, PCWSTR pServiceName, SOCKET_ADDRESS* pAddresses, DWORD dwAddressCount, LPBLOB lpBlob, DWORD dwFlags, DWORD dwNameSpace, LPGUID lpNspId, timeval* timeout, LPOVERLAPPED lpOverlapped, LPLOOKUPSERVICE_COMPLETION_ROUTINE lpCompletionRoutine, LPHANDLE lpNameHandle))GetProcAddress(module, "SetAddrInfoExW");
	exports->setsockopt = (int (WSAAPI*)(SOCKET s, int level, int optname, const char FAR* optval, int optlen))GetProcAddress(module, "setsockopt");
	exports->shutdown = (int (WSAAPI*)(SOCKET s, int how))GetProcAddress(module, "shutdown");
	exports->socket = (SOCKET(WSAAPI*)(int af, int type, int protocol))GetProcAddress(module, "socket");
	exports->WSAGetLastError = (int (WSAAPI*)(void))GetProcAddress(module, "WSAGetLastError");
	exports->WSACleanup = (int (WSAAPI*)(void))GetProcAddress(module, "WSAGetLastError");
	exports->WSAStartup = (int (WSAAPI*)(WORD wVersionRequested, LPWSADATA lpWSAData))GetProcAddress(module, "WSAStartup");
	exports->__WSAFDIsSet = (int (WSAAPI*)(SOCKET fd, fd_set* arg2))GetProcAddress(module, "__WSAFDIsSet");
	return error;
}

void Ws2_32_exports_free_library(Ws2_32_dll_exports_t* exports)
{
	exports->WSACleanup();
	FreeLibrary(GetModuleHandleW(L"Ws2_32.dll"));
}

#ifdef __cplusplus
}
#endif

#endif