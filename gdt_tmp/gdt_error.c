/*
	Graph Drawing Tool version 1.4.0 2019-09-16 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt
*/

#include "gdt_error.h"

struct gdt_error_info_t
{
	int number;
	const char* name;
	const char* description;
};

static const struct gdt_error_info_t gdt_error_table[] = {
	{ 0, "", "No error" },
	{ EPERM, "EPERM", "Operation not permitted" },
	{ ENOENT, "ENOENT", "No such file or directory" },
	{ ESRCH, "ESRCH", "No such process" },
	{ EINTR, "EINTR", "Interrupted system call" },
	{ EIO, "EIO", "I/O error" },
	{ ENXIO, "ENXIO", "No such device or address" },
	{ E2BIG, "E2BIG", "Arg list too long" },
	{ ENOEXEC, "ENOEXEC", "Exec format error" },
	{ EBADF, "EBADF", "Bad file number" },
	{ ECHILD, "ECHILD", "No child processes" },
	{ EAGAIN, "EAGAIN", "Try again" },
	{ ENOMEM, "ENOMEM", "Out of memory" },
	{ EACCES, "EACCES", "Permission denied" },
	{ EFAULT, "EFAULT", "Bad address" },
	{ EBUSY, "EBUSY", "Device or resource busy" },
	{ EEXIST, "EEXIST", "File exists" },
	{ EXDEV, "EXDEV", "Cross-device link" },
	{ ENODEV, "ENODEV", "No such device" },
	{ ENOTDIR, "ENOTDIR", "Not a directory" },
	{ EISDIR, "EISDIR", "Is a directory" },
	{ EINVAL, "EINVAL", "Invalid argument" },
	{ ENFILE, "ENFILE", "File table overflow" },
	{ EMFILE, "EMFILE", "Too many open files" },
	{ ENOTTY, "ENOTTY", "Not a typewriter" },
	{ EFBIG, "EFBIG", "File too large" },
	{ ENOSPC, "ENOSPC", "No space left on device" },
	{ ESPIPE, "ESPIPE", "Illegal seek" },
	{ EROFS, "EROFS", "Read-only file system" },
	{ EMLINK, "EMLINK", "Too many links" },
	{ EPIPE, "EPIPE", "Broken pipe" },
	{ EDOM, "EDOM", "Math argument out of domain of function" },
	{ ERANGE, "ERANGE", "Math result not representable" },
	{ EDEADLK, "EDEADLK", "Resource deadlock would occur" },
	{ ENAMETOOLONG, "ENAMETOOLONG", "File name too long" },
	{ ENOLCK, "ENOLCK", "No record locks available" },
	{ ENOSYS, "ENOSYS", "Function not implemented" },
	{ ENOTEMPTY, "ENOTEMPTY", "Directory not empty" },
	{ EILSEQ, "EILSEQ", "Illegal byte sequence" },
	{ EADDRINUSE, "EADDRINUSE", "Address already in use" },
	{ EADDRNOTAVAIL, "EADDRNOTAVAIL", "Cannot assign requested address" },
	{ EAFNOSUPPORT, "EAFNOSUPPORT", "Address family not supported by protocol" },
	{ EALREADY, "EALREADY", "Operation already in progress" },
	{ EBADMSG, "EBADMSG", "Bad message" },
	{ ECANCELED, "ECANCELED", "Operation canceled" },
	{ ECONNABORTED, "ECONNABORTED", "Software caused connection abort" },
	{ ECONNREFUSED, "ECONNREFUSED", "Connection refused" },
	{ ECONNRESET, "ECONNRESET", "Connection reset by peer" },
	{ EDESTADDRREQ, "EDESTADDRREQ", "Destination address required" },
	{ EHOSTUNREACH, "EHOSTUNREACH", "No route to host" },
	{ EIDRM, "EIDRM", "Identifier removed" },
	{ EINPROGRESS, "EINPROGRESS", "Operation now in progress" },
	{ EISCONN, "EISCONN", "Transport endpoint is already connected" },
	{ ELOOP, "ELOOP", "Too many symbolic links encountered" },
	{ EMSGSIZE, "EMSGSIZE", "Message too long" },
	{ ENETDOWN, "ENETDOWN", "Network is down" },
	{ ENETRESET, "ENETRESET", "Network dropped connection because of reset" },
	{ ENETUNREACH, "ENETUNREACH", "Network is unreachable" },
	{ ENOBUFS, "ENOBUFS", "No buffer space available" },
	{ ENODATA, "ENODATA", "No data available" },
	{ ENOLINK, "ENOLINK", "Link has been severed" },
	{ ENOMSG, "ENOMSG", "No message of desired type" },
	{ ENOPROTOOPT, "ENOPROTOOPT", "Protocol not available" },
	{ ENOSR, "ENOSR", "Out of streams resources" },
	{ ENOSTR, "ENOSTR", "Device not a stream" },
	{ ENOTCONN, "ENOTCONN", "Transport endpoint is not connected" },
	{ ENOTRECOVERABLE, "ENOTRECOVERABLE", "State not recoverable" },
	{ ENOTSOCK, "ENOTSOCK", "Socket operation on non-socket" },
	{ ENOTSUP, "ENOTSUP", "Not supported" },
	{ EOPNOTSUPP, "EOPNOTSUPP", "Operation not supported on transport endpoint" },
	{ EOVERFLOW, "EOVERFLOW", "Value too large for defined data type" },
	{ EOWNERDEAD, "EOWNERDEAD", "Owner dead" },
	{ EPROTO, "EPROTO", "Protocol error" },
	{ EPROTONOSUPPORT, "EPROTONOSUPPORT", "Protocol not supported" },
	{ EPROTOTYPE, "EPROTOTYPE", "Protocol wrong type for socket" },
	{ ETIME, "ETIME", "Timer expired" },
	{ ETIMEDOUT, "ETIMEDOUT", "Connection timed out" },
	{ ETXTBSY, "ETXTBSY", "Text file busy" },
	{ EWOULDBLOCK, "EWOULDBLOCK", "Operation would block" }
#ifdef _WIN32
	, { STRUNCATE, "STRUNCATE", "A string copy or concatenation resulted in a truncated string" }, { EOTHER, "EOTHER", "Other" }
#endif
 };

int gdt_get_error_info(int error, const char** name, const char** description)
{
	if (!error)
		return EINVAL;
	for (int i = 0; i != sizeof(gdt_error_table) / sizeof(struct gdt_error_info_t); ++i)
		if (gdt_error_table[i].number == error)
		{
			if (name)
				*name = gdt_error_table[i].name;
			if (description)
				*description = gdt_error_table[i].description;
			return 0;
		}
	return ENOENT;
}
