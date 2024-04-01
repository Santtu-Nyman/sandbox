#ifdef __cplusplus
extern "C" {
#endif

int _fltused = 0;

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <emmintrin.h>

LPVOID wliv_block_allocate_memory(HANDLE heap, SIZE_T size);

void wliv_block_clear_memory(LPVOID memory, SIZE_T size);

void wliv_block_copy_memory(const LPVOID source, LPVOID destination, SIZE_T size);

DWORD* wliv_allocate_imege_buffer(HANDLE heap, DWORD width, DWORD height);

void wliv_free_imege_buffer(HANDLE heap, DWORD width, DWORD height, DWORD* buffer);

DWORD wliv_get_last_error();

DWORD wliv_get_process_arguments(HANDLE heap, SIZE_T* argument_count, const WCHAR*** argument_values);

DWORD wliv_read_only_map_file(const WCHAR* file_name, SIZE_T* file_size, LPVOID* file_data);

DWORD wliv_load_image_file(const WCHAR* file_name, HANDLE heap, DWORD* image_width, DWORD* image_height, DWORD** image_pixels);

DWORD wliv_random();

void wliv_fit_rectengle_in_rectangle(const RECT* container_rectangle, const RECT* rectangle, RECT* fitted_rectangle);

void wliv_scale_image(SIZE_T source_stride, DWORD source_width, DWORD source_height, const DWORD* source_pixels, SIZE_T destination_stride, DWORD destination_width, DWORD destination_height, DWORD* destination_pixels);

DWORD wliv_register_window_class(const WNDCLASSEXW* window_class_info, DWORD* window_class_identifier);

void wliv_unregister_window_class(HINSTANCE instance, DWORD window_class_identifier);

DWORD wliv_create_window(HINSTANCE instance, DWORD window_class_identifier, DWORD style, DWORD extended_style, LPVOID creation_parameter, LONG width, LONG height, const WCHAR* title, HWND* window);

typedef struct wliv_display_t
{
	HWND window;
	HDC device_context;
	BOOL run;
	DWORD error;
	BITMAPINFOHEADER window_bitmap_header;
	SIZE_T window_bitmap_buffer_size_px;
	DWORD* window_bitmap_pixels;
	MSG window_message;
	DWORD image_width;
	DWORD image_height;
	DWORD* image_pixels;
	HINSTANCE instance;
	DWORD window_class_idetifier;
	HANDLE heap;
} wliv_display_t;

void wliv_draw_display_bitmap(wliv_display_t* display);

LRESULT CALLBACK wliv_window_procedure(HWND window, UINT message, WPARAM w_parameter, LPARAM l_parameter);

DWORD wliv_create_display(HINSTANCE instance, HANDLE heap, const WCHAR* file_name, wliv_display_t** display);

void wliv_close_display(wliv_display_t* display);

DWORD wliv_display_loop(wliv_display_t* display);

DWORD wliv_create_hex_dump(const WCHAR* file_name, SIZE_T size, const void* data);

void main()
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		ExitProcess((UINT)wliv_get_last_error());

	HINSTANCE instance = GetModuleHandleW(0);

	SIZE_T argc;
	const WCHAR** argv;
	DWORD error = wliv_get_process_arguments(heap, &argc, &argv);

	const WCHAR* image_file = 0;
	for (SIZE_T i = 0, e = argc ? (argc - 1) : 0; !image_file && i != e; ++i)
		if (!lstrcmpiW(argv[i], L"-i") || !lstrcmpiW(argv[i], L"--image"))
			image_file = argv[i + 1];

	if (!image_file && argc > 1)
		image_file = argv[1];

	if (image_file)
	{
		for (SIZE_T i = 0, e = argc ? (argc - 1) : 0; i != e; ++i)
			if (!lstrcmpiW(argv[i], L"-h") || !lstrcmpiW(argv[i], L"--image_hex_dump"))
			{
				DWORD image_width;
				DWORD image_height;
				DWORD* image_pixels;
				error = wliv_load_image_file(image_file, heap, &image_width, &image_height, &image_pixels);
				if (error)
				{
					HeapFree(heap, 0, (LPVOID)argv);
					ExitProcess((UINT)error);
				}
				error = wliv_create_hex_dump(argv[i + 1], (SIZE_T)image_width * (SIZE_T)image_height * 4, image_pixels);
				wliv_free_imege_buffer(heap, image_width, image_height, image_pixels);
				if (error)
				{
					HeapFree(heap, 0, (LPVOID)argv);
					ExitProcess((UINT)error);
				}
				i = e - 1;
			}
		wliv_display_t* diplay;
		error = wliv_create_display(instance, heap, image_file, &diplay);
		error = wliv_display_loop(diplay);
		wliv_close_display(diplay);
	}
	else
		error = ERROR_BAD_ARGUMENTS;

	HeapFree(heap, 0, (LPVOID)argv);
	ExitProcess((UINT)error);
}

LPVOID wliv_block_allocate_memory(HANDLE heap, SIZE_T size)
{
	size = (size + 0xF) & ~0xF;
	return HeapAlloc(heap, 0, size);
}

void wliv_block_clear_memory(LPVOID memory, SIZE_T size)
{
	size = (size + 0xF) & ~0xF;
	for (__m128i* i = (__m128i*)memory, * e = (__m128i*)((UINT_PTR)i + size), z = _mm_setzero_si128(); i != e; ++i)
		_mm_store_si128(i, z);
}

void wliv_block_copy_memory(const LPVOID source, LPVOID destination, SIZE_T size)
{
	size = (size + 0xF) & ~0xF;
	for (__m128i* s = (__m128i*)source, *d = (__m128i*)destination, *e = (__m128i*)((UINT_PTR)s + size), b; s != e; ++s, ++d)
	{
		b = _mm_load_si128(s);
		_mm_store_si128(d, b);
	}
}

DWORD* wliv_allocate_imege_buffer(HANDLE heap, DWORD width, DWORD height)
{
	DWORD* buffer = (DWORD*)HeapAlloc(heap, 0, ((((SIZE_T)width + 0x3) & ~0x3) + 8) * ((SIZE_T)height + 2) * sizeof(DWORD));
	if (!buffer)
		return 0;
	return buffer + ((((SIZE_T)width + 0x3) & ~0x3) + 8) + 4;
}

void wliv_free_imege_buffer(HANDLE heap, DWORD width, DWORD height, DWORD* buffer)
{
	HeapFree(heap, 0, buffer - ((((SIZE_T)width + 0x3) & ~0x3) + 8) - 4);
}

DWORD wliv_get_last_error()
{
	DWORD error = GetLastError();
	return error ? error : ERROR_UNIDENTIFIED_ERROR;
}

DWORD wliv_get_process_arguments(HANDLE heap, SIZE_T* argument_count, const WCHAR*** argument_values)
{
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
	if (!shell32)
		return wliv_get_last_error();
	SIZE_T local_argument_count = 0;
	const WCHAR** local_argument_values = ((const WCHAR** (WINAPI*)(const WCHAR*, int*))GetProcAddress(shell32, "CommandLineToArgvW"))(GetCommandLineW(), (int*)&local_argument_count);
	if (!local_argument_values)
	{
		error = wliv_get_last_error();
		FreeLibrary(shell32);
		return error;
	}
	SIZE_T argument_value_data_size = 0;
	for (SIZE_T i = 0; i != local_argument_count; ++i)
		argument_value_data_size += (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
	WCHAR** argument_buffer = (WCHAR**)wliv_block_allocate_memory(heap, local_argument_count * sizeof(WCHAR*) + argument_value_data_size);
	if (!argument_buffer)
	{
		error = wliv_get_last_error();
		LocalFree((HLOCAL)local_argument_values);
		FreeLibrary(shell32);
		return error;
	}
	for (SIZE_T w = local_argument_count * sizeof(WCHAR*), i = 0; i != local_argument_count; ++i)
	{
		WCHAR* p = (WCHAR*)((UINT_PTR)argument_buffer + w);
		SIZE_T s = (((SIZE_T)lstrlenW(local_argument_values[i]) + 1) * sizeof(WCHAR));
		argument_buffer[i] = p;
		for (WCHAR* copy_source = (WCHAR*)local_argument_values[i], *copy_source_end = (WCHAR*)((UINT_PTR)copy_source + s), *copy_destination = argument_buffer[i]; copy_source != copy_source_end; ++copy_source, ++copy_destination)
			*copy_destination = *copy_source;
		w += s;
	}
	LocalFree((HLOCAL)local_argument_values);
	FreeLibrary(shell32);
	*argument_count = local_argument_count;
	*argument_values = (const WCHAR**)argument_buffer;
	return 0;
}

DWORD wliv_read_only_map_file(const WCHAR* file_name, SIZE_T* file_size, LPVOID* file_data)
{
	DWORD error = ERROR_UNIDENTIFIED_ERROR;
	HANDLE file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file != INVALID_HANDLE_VALUE)
	{
		ULONGLONG raw_file_size;
		if (GetFileSizeEx(file, (LARGE_INTEGER*)&raw_file_size))
		{
			if (raw_file_size < (ULONGLONG)((SIZE_T)~0))
			{
				HANDLE mapping = CreateFileMappingW(file, 0, PAGE_READONLY, (DWORD)(raw_file_size >> 32), (DWORD)raw_file_size, 0);
				if (mapping)
				{
					LPVOID view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, (SIZE_T)raw_file_size);
					if (view)
					{
						CloseHandle(mapping);
						CloseHandle(file);
						*file_size = (SIZE_T)raw_file_size;
						*file_data = view;
						return 0;
					}
					else
						error = wliv_get_last_error();
					CloseHandle(mapping);
				}
				else
					error = wliv_get_last_error();
			}
			else
				error = ERROR_FILE_TOO_LARGE;
		}
		else
			error = wliv_get_last_error();
		CloseHandle(file);
	}
	else
		error = wliv_get_last_error();
	return error;
}

DWORD wliv_load_image_file(const WCHAR* file_name, HANDLE heap, DWORD* image_width, DWORD* image_height, DWORD** image_pixels)
{
	DWORD error;
	HBITMAP bitmap;
	HMODULE Gdiplus = LoadLibraryW(L"Gdiplus.dll");
	if (Gdiplus)
	{
		int (WINAPI* GdiplusStartup)(void** token, const void* input, void* output) = (int (WINAPI*)(void**, const void*, void*))GetProcAddress(Gdiplus, "GdiplusStartup");
		void (WINAPI* GdiplusShutdown)(void* token) = (void (WINAPI*)(void*))GetProcAddress(Gdiplus, "GdiplusShutdown");
		int (WINAPI* GdipCreateBitmapFromFileICM)(const WCHAR* file_name, void** bitmap_object) = (int (WINAPI*)(const WCHAR*, void**))GetProcAddress(Gdiplus, "GdipCreateBitmapFromFileICM");
		int (WINAPI* GdipCreateHBITMAPFromBitmap)(void* bitmap_object, HBITMAP* bitmap_handle, DWORD background) = (int (WINAPI*)(void*, HBITMAP*, DWORD))GetProcAddress(Gdiplus, "GdipCreateHBITMAPFromBitmap");
		int (WINAPI* GdipDisposeImage)(void* bitmap_object) = (int (WINAPI*)(void*))GetProcAddress(Gdiplus, "GdipDisposeImage");

		struct { UINT32 GdiplusVersion; void* DebugEventCallback; BOOL SuppressBackgroundThread; BOOL SuppressExternalCodecs; } gdiplus_startup_input = { 1, 0, FALSE, FALSE };
		struct { void* NotificationHook; void* NotificationUnhook; } gdiplus_startup_output;
		void* gdiplus_token;

		int gdiplus_status = GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, &gdiplus_startup_output);

		if (!gdiplus_status)
		{
			void* bitmap_object;
			gdiplus_status = GdipCreateBitmapFromFileICM(file_name, &bitmap_object);

			if (!gdiplus_status)
			{
				gdiplus_status = GdipCreateHBITMAPFromBitmap(bitmap_object, &bitmap, 0xFF000000);
				GdipDisposeImage(bitmap_object);
			}

			GdiplusShutdown(gdiplus_token);
		}

		FreeLibrary(Gdiplus);

		const DWORD gdiplus_status_error_codes[] = { /*Ok = 0*/ ERROR_SUCCESS, /*GenericError = 1*/ ERROR_UNIDENTIFIED_ERROR, /*InvalidParameter = 2*/ ERROR_INVALID_PARAMETER, /*OutOfMemory = 3*/ ERROR_OUTOFMEMORY, /*ObjectBusy = 4*/ ERROR_BUSY, /*InsufficientBuffer = 5*/ ERROR_INSUFFICIENT_BUFFER, /*NotImplemented = 6*/ ERROR_NOT_SUPPORTED, /*Win32Error = 7*/ ERROR_UNIDENTIFIED_ERROR, /*WrongState = 8*/ ERROR_INVALID_STATE, /*Aborted = 9*/ ERROR_OPERATION_ABORTED, /*FileNotFound = 10*/ ERROR_FILE_NOT_FOUND, /*ValueOverflow = 11*/ ERROR_INVALID_DATA, /*AccessDenied = 12*/ ERROR_ACCESS_DENIED, /*UnknownImageFormat = 13*/ ERROR_UNSUPPORTED_TYPE, /*FontFamilyNotFound = 14*/ ERROR_NOT_SUPPORTED, /*FontStyleNotFound = 15*/ ERROR_NOT_SUPPORTED, /*NotTrueTypeFont = 16*/ ERROR_NOT_SUPPORTED, /*UnsupportedGdiplusVersion = 17*/ ERROR_NOT_SUPPORTED, /*GdiplusNotInitialized = 18*/ ERROR_INVALID_STATE, /*PropertyNotFound = 19*/ ERROR_NOT_SUPPORTED, /*PropertyNotSupported = 20*/ ERROR_NOT_SUPPORTED, /*ProfileNotFound = 21*/ ERROR_UNIDENTIFIED_ERROR };
		error = gdiplus_status_error_codes[((int)gdiplus_status < (sizeof(gdiplus_status_error_codes) / sizeof(DWORD))) ? (int)gdiplus_status : ERROR_UNIDENTIFIED_ERROR];
	}
	else
		error = wliv_get_last_error();

	if (!error)
	{
		BITMAP bitmap_info;
		if (GetObjectW(bitmap, sizeof(BITMAP), &bitmap_info))
		{
			BITMAPINFOHEADER image_bitmap_header;
			image_bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
			image_bitmap_header.biWidth = bitmap_info.bmWidth;
			image_bitmap_header.biHeight = bitmap_info.bmHeight;
			image_bitmap_header.biPlanes = 1;
			image_bitmap_header.biBitCount = 32;
			image_bitmap_header.biCompression = BI_RGB;
			image_bitmap_header.biSizeImage = 0;
			image_bitmap_header.biXPelsPerMeter = 0;
			image_bitmap_header.biYPelsPerMeter = 0;
			image_bitmap_header.biClrUsed = 0;
			image_bitmap_header.biClrImportant = 0;

			DWORD* image = wliv_allocate_imege_buffer(heap, (SIZE_T)image_bitmap_header.biWidth, (SIZE_T)image_bitmap_header.biHeight);
			if (image)
			{
				HDC device_context = CreateCompatibleDC(0);
				if (GetDIBits(device_context, bitmap, 0, (UINT)bitmap_info.bmHeight, image, (BITMAPINFO*)&image_bitmap_header, DIB_RGB_COLORS))
				{
					*image_width = (DWORD)image_bitmap_header.biWidth;
					*image_height = (DWORD)image_bitmap_header.biHeight;
					*image_pixels = image;
					if (device_context)
						DeleteDC(device_context);
					DeleteObject(bitmap);
					return 0;
				}
				else
					error = wliv_get_last_error();
				if (device_context)
					DeleteDC(device_context);
				wliv_free_imege_buffer(heap, (DWORD)image_bitmap_header.biWidth, (DWORD)image_bitmap_header.biHeight, image);
			}
			else
				error = wliv_get_last_error();
		}
		else
			error = wliv_get_last_error();
		DeleteObject(bitmap);
	}

	return error;
}

DWORD wliv_random()
{
	POINT cursor_position;
	if (GetCursorPos(&cursor_position))
	{
		cursor_position.x &= 0x1F;
		cursor_position.y &= 0x1F;
	}
	else
	{
		cursor_position.x = 0;
		cursor_position.y = 0;
	}
	FILETIME time;
	GetSystemTimeAsFileTime(&time);
	ULARGE_INTEGER performance_frequency;
	if (!QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frequency))
	{
		performance_frequency.LowPart = 0;
		performance_frequency.HighPart = 0;
	}
	ULARGE_INTEGER performance_count;
	if (!QueryPerformanceCounter((LARGE_INTEGER*)&performance_count))
	{
		performance_count.LowPart = 0;
		performance_count.HighPart = 0;
	}
	performance_count.LowPart ^= performance_frequency.LowPart;
	performance_count.HighPart ^= performance_frequency.HighPart;
	return (((((time.dwLowDateTime << (DWORD)cursor_position.x) | (time.dwLowDateTime >> (0x20 - (DWORD)cursor_position.x))) ^
		((time.dwHighDateTime << (DWORD)cursor_position.y) | (time.dwHighDateTime >> (0x20 - (DWORD)cursor_position.y)))) ^
		(((performance_count.LowPart & 0xFF) ^ ((performance_count.HighPart >> 24) & 0xFF)) |
		((((performance_count.LowPart >> 8) & 0xFF) ^ ((performance_count.HighPart >> 16) & 0xFF)) << 8) |
		((((performance_count.LowPart >> 16) & 0xFF) ^ ((performance_count.HighPart >> 8) & 0xFF)) << 16) |
		((((performance_count.LowPart >> 24) & 0xFF) ^ (performance_count.HighPart & 0xFF)) << 24))) + GetTickCount()) ^ GetCurrentThreadId();
}

void wliv_fit_rectengle_in_rectangle(const RECT* container_rectangle, const RECT* rectangle, RECT* fitted_rectangle)
{
	ULONGLONG container_width = (ULONGLONG)(container_rectangle->right - container_rectangle->left) << 16;
	ULONGLONG container_height = (ULONGLONG)(container_rectangle->bottom - container_rectangle->top) << 16;
	ULONGLONG rectengle_width = (ULONGLONG)(rectangle->right - rectangle->left) << 16;
	ULONGLONG rectengle_height = (ULONGLONG)(rectangle->bottom - rectangle->top) << 16;
	ULONGLONG scale = ((container_width << 16) / rectengle_width);
	ULONGLONG scaled_dimension = (scale * rectengle_height) >> 16;
	ULONGLONG width;
	ULONGLONG height;
	if (scaled_dimension <= container_height)
	{
		width = container_width;
		height = scaled_dimension;
	}
	else
	{
		scale = ((container_height << 16) / rectengle_height);
		scaled_dimension = (scale * rectengle_width) >> 16;
		width = scaled_dimension;
		height = container_height;
	}
	fitted_rectangle->left = (LONG)((container_width - width) >> 17);
	fitted_rectangle->top = (LONG)((container_height - height) >> 17);
	fitted_rectangle->right = fitted_rectangle->left + (LONG)(width >> 16);
	fitted_rectangle->bottom = fitted_rectangle->top + (LONG)(height >> 16);
}

void wliv_scale_image(SIZE_T source_stride, DWORD source_width, DWORD source_height, const DWORD* source_pixels, SIZE_T destination_stride, DWORD destination_width, DWORD destination_height, DWORD* destination_pixels)
{
	float y_end = (float)destination_height;
	float x_end = (float)destination_width;
	float y_scale = ((float)source_height - 1.0f) / (y_end - 1.0f);
	float x_scale = ((float)source_width - 1.0f) / (x_end - 1.0f);
	for (float y = 0.0f; y < y_end; y += 1.0f)
	{
		float source_y = y * y_scale;
		int source_y_i = (int)source_y;
		float source_y_b[2] = { 1.0f - (source_y - (float)source_y_i), source_y - (float)source_y_i };
		const DWORD* source_row[2] = { (const DWORD*)((UINT_PTR)source_pixels + ((SIZE_T)source_y_i * source_stride)), (const DWORD*)((UINT_PTR)source_pixels + ((SIZE_T)(source_y_i + 1) * source_stride)) };
		DWORD* destination_row = (DWORD*)((UINT_PTR)destination_pixels + ((SIZE_T)y * destination_stride));
		for (float x = 0.0f; x < x_end; x += 1.0f)
		{
			float source_x = x * x_scale;
			int source_x_i = (int)source_x;
			float source_x_b[2] = { 1.0f - (source_x - (float)source_x_i), source_x - (float)source_x_i };
			float colors[2][2][4] = {
				{ { (float)(source_row[0][source_x_i] & 0xFF), (float)((source_row[0][source_x_i] >> 8) & 0xFF), (float)((source_row[0][source_x_i] >> 16) & 0xFF), (float)(source_row[0][source_x_i] >> 24) },
				{ (float)(source_row[0][source_x_i + 1] & 0xFF), (float)((source_row[0][source_x_i + 1] >> 8) & 0xFF), (float)((source_row[0][source_x_i + 1] >> 16) & 0xFF), (float)(source_row[0][source_x_i + 1] >> 24) } },
				{ { (float)(source_row[1][source_x_i] & 0xFF), (float)((source_row[1][source_x_i] >> 8) & 0xFF), (float)((source_row[1][source_x_i] >> 16) & 0xFF), (float)(source_row[1][source_x_i] >> 24) },
				{ (float)(source_row[1][source_x_i + 1] & 0xFF), (float)((source_row[1][source_x_i + 1] >> 8) & 0xFF), (float)((source_row[1][source_x_i + 1] >> 16) & 0xFF), (float)(source_row[1][source_x_i + 1] >> 24) } } };
			destination_row[(SIZE_T)x] =
				(DWORD)(source_y_b[0] * (source_x_b[0] * colors[0][0][0] + source_x_b[1] * colors[0][1][0]) + source_y_b[1] * (source_x_b[0] * colors[1][0][0] + source_x_b[1] * colors[1][1][0])) |
				((DWORD)(source_y_b[0] * (source_x_b[0] * colors[0][0][1] + source_x_b[1] * colors[0][1][1]) + source_y_b[1] * (source_x_b[0] * colors[1][0][1] + source_x_b[1] * colors[1][1][1])) << 8) |
				((DWORD)(source_y_b[0] * (source_x_b[0] * colors[0][0][2] + source_x_b[1] * colors[0][1][2]) + source_y_b[1] * (source_x_b[0] * colors[1][0][2] + source_x_b[1] * colors[1][1][2])) << 16) |
				((DWORD)(source_y_b[0] * (source_x_b[0] * colors[0][0][3] + source_x_b[1] * colors[0][1][3]) + source_y_b[1] * (source_x_b[0] * colors[1][0][3] + source_x_b[1] * colors[1][1][3])) << 24);
		}
	}

	/*
	float y_end = (float)destination_height;
	float x_end = (float)destination_width;
	float y_scale = (float)source_height / y_end;
	float x_scale = (float)source_width / x_end;
	for (float y = 0.0f; y < y_end; y += 1.0f)
	{
		const DWORD* source_row = (const DWORD*)((UINT_PTR)source_pixels + ((SIZE_T)(y * y_scale) * source_stride));
		DWORD* destination_row = (DWORD*)((UINT_PTR)destination_pixels + ((SIZE_T)y * destination_stride));
		for (float x = 0.0f; x < x_end; x += 1.0f)
			destination_row[(SIZE_T)x] = source_row[(SIZE_T)(x * x_scale)];
	}
	*/
}

DWORD wliv_register_window_class(const WNDCLASSEXW* window_class_info, DWORD* window_class_identifier)
{
	WCHAR class_name[21];
	WNDCLASSEXW class_info = *window_class_info;
	class_info.lpszClassName = class_name;
	class_name[0] = L'W';
	class_name[1] = L'L';
	class_name[2] = L'I';
	class_name[3] = L'V';
	for (DWORD process = GetCurrentProcessId(), i = 0; i != 8; ++i)
		class_name[4 + i] = (WCHAR)L"0123456789ABCDEF"[((process >> ((7 - i) << 2)) & 0xF)];
	class_name[20] = 0;
	for (DWORD random = wliv_random(), i = 0;; ++random, ++i)
	{
		for (DWORD j = 0; j != 8; ++j)
			class_name[12 + j] = (WCHAR)L"0123456789ABCDEF"[((random >> ((7 - j) << 2)) & 0xF)];
		ATOM atom = RegisterClassExW(&class_info);
		if (atom)
		{
			*window_class_identifier = random;
			return 0;
		}
		DWORD error = wliv_get_last_error();
		if (error != ERROR_CLASS_ALREADY_EXISTS || i == 0xFFFFFFFF)
		{
			*window_class_identifier = 0;
			return error;
		}
	}
}

void wliv_unregister_window_class(HINSTANCE instance, DWORD window_class_identifier)
{
	WCHAR class_name[21];
	class_name[0] = L'W';
	class_name[1] = L'L';
	class_name[2] = L'I';
	class_name[3] = L'V';
	for (DWORD process = GetCurrentProcessId(), i = 0; i != 8; ++i)
		class_name[4 + i] = (WCHAR)L"0123456789ABCDEF"[((process >> ((7 - i) << 2)) & 0xF)];
	for (DWORD i = 0; i != 8; ++i)
		class_name[12 + i] = (WCHAR)L"0123456789ABCDEF"[((window_class_identifier >> ((7 - i) << 2)) & 0xF)];
	class_name[20] = 0;
	UnregisterClassW(class_name, instance);
}

DWORD wliv_create_window(HINSTANCE instance, DWORD window_class_identifier, DWORD style, DWORD extended_style, LPVOID creation_parameter, LONG width, LONG height, const WCHAR* title, HWND* window)
{
	WCHAR class_name[21];
	class_name[0] = L'W';
	class_name[1] = L'L';
	class_name[2] = L'I';
	class_name[3] = L'V';
	for (DWORD process = GetCurrentProcessId(), i = 0; i != 8; ++i)
		class_name[4 + i] = (WCHAR)L"0123456789ABCDEF"[((process >> ((7 - i) << 2)) & 0xF)];
	for (DWORD i = 0; i != 8; ++i)
		class_name[12 + i] = (WCHAR)L"0123456789ABCDEF"[((window_class_identifier >> ((7 - i) << 2)) & 0xF)];
	class_name[20] = 0;
	RECT rectangle = { 0, 0, 0, 0 };
	if (width != CW_USEDEFAULT || height != CW_USEDEFAULT)
	{
		rectangle.right = (width != CW_USEDEFAULT) ? width : (LONG)GetSystemMetrics(SM_CXSCREEN);
		rectangle.bottom = (height != CW_USEDEFAULT) ? height : (LONG)GetSystemMetrics(SM_CYSCREEN);
		if (!AdjustWindowRectEx(&rectangle, style, FALSE, extended_style))
			return wliv_get_last_error();
	}
	HWND window_handle = CreateWindowExW(extended_style, class_name, title, style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		(width != CW_USEDEFAULT) ? (int)(rectangle.right - rectangle.left) : CW_USEDEFAULT,
		(height != CW_USEDEFAULT) ? (int)(rectangle.bottom - rectangle.top) : CW_USEDEFAULT,
		0, 0, instance, creation_parameter);
	if (!window_handle)
		return wliv_get_last_error();
	*window = window_handle;
	return 0;
}

void wliv_draw_display_bitmap(wliv_display_t* display)
{
	RECT window_rectangle = { 0, 0, display->window_bitmap_header.biWidth, display->window_bitmap_header.biHeight };
	RECT image_rectangle = { 0, 0, (LONG)display->image_width, (LONG)display->image_height };
	RECT window_image_rectangle;
	wliv_fit_rectengle_in_rectangle(&window_rectangle, &image_rectangle, &window_image_rectangle);
	wliv_block_clear_memory(display->window_bitmap_pixels, (SIZE_T)display->window_bitmap_header.biWidth * (SIZE_T)display->window_bitmap_header.biHeight * sizeof(DWORD));
	wliv_scale_image((SIZE_T)display->image_width * sizeof(DWORD), display->image_width, display->image_height, display->image_pixels,
		(SIZE_T)display->window_bitmap_header.biWidth * sizeof(DWORD),
		(DWORD)(window_image_rectangle.right - window_image_rectangle.left),
		(DWORD)(window_image_rectangle.bottom - window_image_rectangle.top),
		(DWORD*)((UINT_PTR)display->window_bitmap_pixels +
		((SIZE_T)window_image_rectangle.top * ((SIZE_T)display->window_bitmap_header.biWidth * sizeof(DWORD))) +
		((SIZE_T)window_image_rectangle.left * sizeof(DWORD))));
}

LRESULT CALLBACK wliv_window_procedure(HWND window, UINT message, WPARAM w_parameter, LPARAM l_parameter)
{
	wliv_display_t* display = (wliv_display_t*)GetWindowLongPtrW(window, GWLP_USERDATA);
	switch (message)
	{
		case WM_PAINT:
		{
			RECT update_rectangle;
			GetUpdateRect(window, &update_rectangle, FALSE);
			StretchDIBits(display->device_context,
				update_rectangle.left,
				update_rectangle.top,
				update_rectangle.right - update_rectangle.left,
				update_rectangle.bottom - update_rectangle.top,
				update_rectangle.left,
				display->window_bitmap_header.biHeight - update_rectangle.bottom,
				update_rectangle.right - update_rectangle.left,
				update_rectangle.bottom - update_rectangle.top,
				display->window_bitmap_pixels, (const BITMAPINFO*)&display->window_bitmap_header, DIB_RGB_COLORS, SRCCOPY);
			ValidateRect(window, &update_rectangle);
			return 0;
		}
		case WM_SIZE:
		{
			if (w_parameter != SIZE_MINIMIZED)
			{
				RECT window_rectangle;
				if (!GetClientRect(window, &window_rectangle))
				{
					display->error = wliv_get_last_error();
					SetWindowPos(window, HWND_TOP, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
					display->run = FALSE;
					return 0;
				}
				window_rectangle.right -= window_rectangle.left;
				window_rectangle.bottom -= window_rectangle.top;
				if (display->window_bitmap_header.biWidth != window_rectangle.right || display->window_bitmap_header.biHeight != window_rectangle.bottom)
				{
					SIZE_T required_window_bitmap_size_px = (SIZE_T)window_rectangle.right * (SIZE_T)window_rectangle.bottom;
					if (required_window_bitmap_size_px > display->window_bitmap_buffer_size_px)
					{
						DWORD* required_window_bitmap_pixels = (DWORD*)wliv_block_allocate_memory(display->heap, required_window_bitmap_size_px * sizeof(DWORD));
						if (!required_window_bitmap_pixels)
						{
							display->error = wliv_get_last_error();
							SetWindowPos(window, HWND_TOP, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
							display->run = FALSE;
							return 0;
						}
						HeapFree(display->heap, 0, display->window_bitmap_pixels);
						display->window_bitmap_buffer_size_px = required_window_bitmap_size_px;
						display->window_bitmap_pixels = required_window_bitmap_pixels;
					}
					display->window_bitmap_header.biWidth = window_rectangle.right;
					display->window_bitmap_header.biHeight = window_rectangle.bottom;
					wliv_draw_display_bitmap(display);
					InvalidateRect(window, 0, 0);
				}
			}
			return 0;
		}
		case WM_ERASEBKGND:
			return 1;
		case WM_TIMER:
		case WM_KEYDOWN:
			return 0;
		case WM_CLOSE:
		case WM_DESTROY:
		case WM_QUIT:
		{
			wliv_block_clear_memory(display->window_bitmap_pixels, (SIZE_T)display->window_bitmap_header.biWidth * (SIZE_T)display->window_bitmap_header.biHeight * sizeof(DWORD));
			StretchDIBits(display->device_context, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, display->window_bitmap_pixels, (const BITMAPINFO*)&display->window_bitmap_header, DIB_RGB_COLORS, SRCCOPY);
			ValidateRect(window, 0);
			display->run = FALSE;
			return 0;
		}
		case WM_CREATE:
		{
			display = (wliv_display_t*)((CREATESTRUCT*)l_parameter)->lpCreateParams;
			SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)display);
			SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			if (GetWindowLongPtrW(window, GWLP_USERDATA) != (LONG_PTR)display)
			{
				display->error = wliv_get_last_error();
				return (LRESULT)-1;
			}
			RECT window_rectangle;
			if (!GetClientRect(window, &window_rectangle))
			{
				display->error = wliv_get_last_error();
				return (LRESULT)-1;
			}
			HDC device_context = GetDC(window);
			if (!device_context)
			{
				display->error = wliv_get_last_error();
				return (LRESULT)-1;
			}
			display->window = window;
			display->device_context = device_context;
			display->run = TRUE;
			display->error = 0;
			display->window_bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
			display->window_bitmap_header.biWidth = (window_rectangle.right - window_rectangle.left);
			display->window_bitmap_header.biHeight = (window_rectangle.bottom - window_rectangle.top);
			display->window_bitmap_header.biPlanes = 1;
			display->window_bitmap_header.biBitCount = 32;
			display->window_bitmap_header.biCompression = BI_RGB;
			display->window_bitmap_header.biSizeImage = 0;
			display->window_bitmap_header.biXPelsPerMeter = 0;
			display->window_bitmap_header.biYPelsPerMeter = 0;
			display->window_bitmap_header.biClrUsed = 0;
			display->window_bitmap_header.biClrImportant = 0;
			display->instance = (HINSTANCE)GetWindowLongPtrW(window, GWLP_HINSTANCE);
			display->window_bitmap_buffer_size_px = (SIZE_T)display->window_bitmap_header.biWidth * (SIZE_T)display->window_bitmap_header.biHeight;
			display->window_bitmap_pixels = (DWORD*)wliv_block_allocate_memory(display->heap, display->window_bitmap_buffer_size_px * sizeof(DWORD));
			if (!display->window_bitmap_pixels)
			{
				display->error = wliv_get_last_error();
				ReleaseDC(window, display->device_context);
				return (LRESULT)-1;
			}
			wliv_draw_display_bitmap(display);
			StretchDIBits(display->device_context, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, 0, 0, (int)display->window_bitmap_header.biWidth, (int)display->window_bitmap_header.biHeight, display->window_bitmap_pixels, (const BITMAPINFO*)&display->window_bitmap_header, DIB_RGB_COLORS, SRCCOPY);
			ValidateRect(window, 0);
			return (LRESULT)0;
		}
		default:
			return DefWindowProcW(window, message, w_parameter, l_parameter);
	}
}

DWORD wliv_create_display(HINSTANCE instance, HANDLE heap, const WCHAR* file_name, wliv_display_t** display)
{
	DWORD image_width;
	DWORD image_height;
	DWORD* image_pixels;
	DWORD error = wliv_load_image_file(file_name, heap, &image_width, &image_height, &image_pixels);
	if (error)
		return error;
	WNDCLASSEXW window_class_info;
	window_class_info.cbSize = sizeof(WNDCLASSEXW);
	window_class_info.style = CS_HREDRAW | CS_VREDRAW;
	window_class_info.lpfnWndProc = wliv_window_procedure;
	window_class_info.cbClsExtra = 0;
	window_class_info.cbWndExtra = 0;
	window_class_info.hInstance = instance;
	window_class_info.hIcon = 0;// LoadIconW(0, (const WCHAR*)IDI_APPLICATION);
	window_class_info.hCursor = LoadCursorW(0, (const WCHAR*)IDC_ARROW);
	window_class_info.hbrBackground = 0;
	window_class_info.lpszMenuName = 0;
	window_class_info.lpszClassName = 0;
	window_class_info.hIconSm = 0;// (HICON)LoadImageW(window_class_info.hInstance, MAKEINTRESOURCEW(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	DWORD window_class_idetifier;
	error = wliv_register_window_class(&window_class_info, &window_class_idetifier);
	if (error)
	{
		wliv_free_imege_buffer(heap, image_width, image_height, image_pixels);
		return error;
	}
	wliv_display_t* image_display = (wliv_display_t*)wliv_block_allocate_memory(heap, sizeof(wliv_display_t));
	if (!image_display)
	{
		error = wliv_get_last_error();
		wliv_unregister_window_class(instance, window_class_idetifier);
		wliv_free_imege_buffer(heap, image_width, image_height, image_pixels);
		return error;
	}
	image_display->run = FALSE;
	image_display->error = ERROR_UNIDENTIFIED_ERROR;
	image_display->window_class_idetifier = window_class_idetifier;
	image_display->heap = heap;
	image_display->image_width = image_width;
	image_display->image_height = image_height;
	image_display->image_pixels = image_pixels;
	error = wliv_create_window(instance, window_class_idetifier, WS_OVERLAPPEDWINDOW | WS_VISIBLE, WS_EX_APPWINDOW | WS_EX_DLGMODALFRAME, image_display, CW_USEDEFAULT, CW_USEDEFAULT, file_name, &image_display->window);
	if (error)
	{
		HeapFree(heap, 0, image_display);
		wliv_unregister_window_class(instance, window_class_idetifier);
		wliv_free_imege_buffer(heap, image_width, image_height, image_pixels);
		return error;
	}
	if (image_display->error)
	{
		if (IsWindow(image_display->window))
			DestroyWindow(image_display->window);
		if (image_display->window_bitmap_pixels)
			HeapFree(heap, 0, image_display->window_bitmap_pixels);
		HeapFree(heap, 0, image_display);
		wliv_unregister_window_class(instance, window_class_idetifier);
		wliv_free_imege_buffer(heap, image_width, image_height, image_pixels);
		return error;
	}
	*display = image_display;
	return 0;
}

void wliv_close_display(wliv_display_t* display)
{
	HANDLE heap = display->heap;
	DestroyWindow(display->window);
	wliv_unregister_window_class(display->instance, display->window_class_idetifier);
	HeapFree(heap, 0, display->window_bitmap_pixels);
	wliv_free_imege_buffer(heap, display->image_width, display->image_height, display->image_pixels);
	HeapFree(heap, 0, display);
}

DWORD wliv_display_loop(wliv_display_t* display)
{
	HWND window_handle = display->window;
	for (BOOL loop = display->run ? GetMessageW(&display->window_message, window_handle, 0, 0) : FALSE; display->run && loop && loop != (BOOL)-1; loop = GetMessageW(&display->window_message, window_handle, 0, 0))
	{
		TranslateMessage(&display->window_message);
		DispatchMessageW(&display->window_message);
	}
	return display->error;
}

DWORD wliv_create_hex_dump(const WCHAR* file_name, SIZE_T size, const void* data)
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return wliv_get_last_error();
	SIZE_T file_name_length = (SIZE_T)lstrlenW(file_name);
	if (!file_name_length)
		return ERROR_INVALID_PARAMETER;
	const SIZE_T line_bytes = 32;
	SIZE_T full_lines = size / line_bytes;
	if (full_lines && !(size % line_bytes))
		--full_lines;
	SIZE_T last_line_bytes = size - (full_lines * line_bytes);
	SIZE_T file_size = (full_lines * line_bytes * 6) + ((last_line_bytes * 6) - (last_line_bytes ? 2 : 0));
	WCHAR* temporal_file_name = (WCHAR*)HeapAlloc(heap, 0, (file_name_length + 14) * sizeof(WCHAR));
	if (!temporal_file_name)
		return wliv_get_last_error();
	SIZE_T file_extension_offset = file_name_length - 1;
	if (!(file_name[file_extension_offset] == L'\\' || file_name[file_extension_offset] == L'/'))
		while (file_extension_offset && !(file_name[file_extension_offset - 1] == L'\\' || file_name[file_extension_offset - 1] == L'/') && file_name[file_extension_offset] != L'.')
			--file_extension_offset;
	if (file_name[file_extension_offset] != L'.')
		file_extension_offset = file_name_length;
	for (WCHAR* d = temporal_file_name, * s = (WCHAR*)file_name, * e = s + file_extension_offset; s != e; ++s, ++d)
		*d = *s;
	temporal_file_name[file_extension_offset] = L'-';
	for (WCHAR* d = temporal_file_name + file_extension_offset + 9, * s = (WCHAR*)L".tmp", *e  = s + 5; s != e; ++s, ++d)
		*d = *s;
	DWORD error = ERROR_FILE_EXISTS;
	HANDLE file;
	for (DWORD random = wliv_random(), i = 0; error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS; ++i, ++random)
	{
		for (DWORD random = wliv_random(), j = 0; j != 8; ++j)
			*(temporal_file_name + file_extension_offset + 1 + (SIZE_T)j) = L"0123456789ABCDEF"[((random >> ((7 - j) << 2)) & 0xF)];
		file = CreateFileW(temporal_file_name, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
		if (file != INVALID_HANDLE_VALUE)
			error = 0;
		else
			error = wliv_get_last_error();
		if (error && (!(error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) || i == 0xFFFFFFFF))
		{
			HeapFree(heap, 0, temporal_file_name);
			return error;
		}
	}
	ULONGLONG zero_file_size = 0;
	ULONGLONG raw_file_size = (ULONGLONG)file_size;
	if (!SetFilePointerEx(file, *(LARGE_INTEGER*)&raw_file_size, 0, FILE_BEGIN) || !SetEndOfFile(file) || !SetFilePointerEx(file, *(LARGE_INTEGER*)&zero_file_size, 0, FILE_BEGIN))
	{
		error = wliv_get_last_error();
		CloseHandle(file);
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	HANDLE file_mapping = CreateFileMappingW(file, 0, PAGE_READWRITE, (DWORD)(raw_file_size >> 32), (DWORD)(raw_file_size & 0xFFFFFFFF), 0);
	if (!file_mapping)
	{
		error = wliv_get_last_error();
		CloseHandle(file);
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	BYTE* file_data = (BYTE*)MapViewOfFile(file_mapping, FILE_MAP_WRITE | FILE_MAP_READ, 0, 0, file_size);
	if (!file_data)
	{
		error = wliv_get_last_error();
		CloseHandle(file_mapping);
		CloseHandle(file);
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	const BYTE ascii_hex_table[16] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };
	for (BYTE* d = file_data, * s = (BYTE*)data, * e = s + (full_lines * line_bytes), t; s != e;)
	{
		for (SIZE_T c = line_bytes - 1; c--;)
		{
			t = *s++;
			*d++ = 0x30;
			*d++ = 0x78;
			*d++ = ascii_hex_table[t >> 0x4];
			*d++ = ascii_hex_table[t & 0xF];
			*d++ = 0x2C;
			*d++ = 0x20;
		}
		t = *s++;
		*d++ = 0x30;
		*d++ = 0x78;
		*d++ = ascii_hex_table[t >> 0x4];
		*d++ = ascii_hex_table[t & 0xF];
		*d++ = 0x2C;
		*d++ = 0x0A;
	}
	if (last_line_bytes)
	{
		BYTE* d = file_data + (full_lines * line_bytes * 6);
		BYTE* s = (BYTE*)data + (full_lines * line_bytes);
		BYTE t;
		for (SIZE_T c = last_line_bytes - 1; c--;)
		{
			t = *s++;
			*d++ = 0x30;
			*d++ = 0x78;
			*d++ = ascii_hex_table[t >> 0x4];
			*d++ = ascii_hex_table[t & 0xF];
			*d++ = 0x2C;
			*d++ = 0x20;
		}
		t = *s++;
		*d++ = 0x30;
		*d++ = 0x78;
		*d++ = ascii_hex_table[t >> 0x4];
		*d++ = ascii_hex_table[t & 0xF];
	}
	if (!FlushViewOfFile(file_data, file_size))
	{
		error = wliv_get_last_error();
		UnmapViewOfFile(file_data);
		CloseHandle(file_mapping);
		CloseHandle(file);
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	UnmapViewOfFile(file_data);
	CloseHandle(file_mapping);
	CloseHandle(file);
	if (MoveFileW(temporal_file_name, file_name))
	{
		HeapFree(heap, 0, temporal_file_name);
		return 0;
	}
	error = wliv_get_last_error();
	if (!(error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS))
	{
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	WCHAR* remove_file_name = (WCHAR*)HeapAlloc(heap, 0, (file_name_length + 14) * sizeof(WCHAR));
	if (!remove_file_name)
	{
		error = wliv_get_last_error();
		DeleteFileW(temporal_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return error;
	}
	for (WCHAR* d = remove_file_name, * s = (WCHAR*)temporal_file_name, * e = s + file_extension_offset + 14; s != e; ++s, ++d)
		*d = *s;
	error = ERROR_FILE_EXISTS;
	for (DWORD random = wliv_random(), i = 0; error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS; ++i, ++random)
	{
		for (DWORD random = wliv_random(), j = 0; j != 8; ++j)
			*(remove_file_name + file_extension_offset + 1 + (SIZE_T)j) = L"0123456789ABCDEF"[((random >> ((7 - j) << 2)) & 0xF)];
		if (MoveFileW(file_name, remove_file_name))
			error = 0;
		else
			error = wliv_get_last_error();
		if (error && (!(error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) || i == 0xFFFFFFFF))
		{
			HeapFree(heap, 0, remove_file_name);
			DeleteFileW(temporal_file_name);
			HeapFree(heap, 0, temporal_file_name);
			return error;
		}
	}
	if (MoveFileW(temporal_file_name, file_name))
	{
		DeleteFileW(remove_file_name);
		HeapFree(heap, 0, remove_file_name);
		HeapFree(heap, 0, temporal_file_name);
		return 0;
	}
	error = wliv_get_last_error();
	DeleteFileW(temporal_file_name);
	MoveFileW(remove_file_name, file_name);
	HeapFree(heap, 0, remove_file_name);
	HeapFree(heap, 0, temporal_file_name);
	return error;
}

#ifdef __cplusplus
}
#endif