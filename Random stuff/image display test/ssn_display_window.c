#ifdef __cplusplus
extern "C" {
#endif

#include "ssn_display_window.h"

void ssn_clear_memory(void* ptr, size_t size)
{
	// This needs some speed testing. Test against rep stos and try to do something about the end part
	uint8_t* clear_i = (uint8_t*)ptr;
	uint8_t* clear_end = (uint8_t*)ptr + (size & (size_t)~0xF);
	__m128i clear_block = _mm_setzero_si128();
	while (clear_i != clear_end)
	{
		_mm_store_si128((__m128i*)clear_i, clear_block);
		clear_i += 16;
	}
	if (size & 0x8)
	{
		*(uint64_t*)clear_i = 0;
		clear_i += 0x8;
	}
	if (size & 0x4)
	{
		*(uint32_t*)clear_i = 0;
		clear_i += 0x4;
	}
	if (size & 0x2)
	{
		*(uint16_t*)clear_i = 0;
		clear_i += 0x2;
	}
	if (size & 0x1)
		*(uint8_t*)clear_i = 0;
}

void ssn_fit_rectangle_in_frame(int frame_width, int frame_height, int rectangle_width, int rectangle_height, int* fitted_x, int* fitted_y, int* fitted_width, int* fitted_height)
{
	uint64_t fp_frame_width = (uint64_t)frame_width << 16;
	uint64_t fp_frame_height = (uint64_t)frame_height << 16;
	uint64_t fp_rectengle_width = (uint64_t)rectangle_width << 16;
	uint64_t fp_rectengle_height = (uint64_t)rectangle_height << 16;
	uint64_t fp_scale = ((fp_frame_width << 16) / fp_rectengle_width);
	uint64_t fp_scaled_dimension = (fp_scale * fp_rectengle_height) >> 16;
	uint64_t fp_width;
	uint64_t fp_height;
	if (fp_scaled_dimension <= fp_frame_height)
	{
		fp_width = fp_frame_width;
		fp_height = fp_scaled_dimension;
	}
	else
	{
		fp_scale = ((fp_frame_height << 16) / fp_rectengle_height);
		fp_scaled_dimension = (fp_scale * fp_rectengle_width) >> 16;
		fp_width = fp_scaled_dimension;
		fp_height = fp_frame_height;
	}
	int x = (int)((fp_frame_width - fp_width) >> 17);
	int y = (int)((fp_frame_height - fp_height) >> 17);
	int width = (int)(fp_width >> 16);
	int height = (int)(fp_height >> 16);
	*fitted_x = x;
	*fitted_y = y;
	*fitted_width = width;
	*fitted_height = height;
}

void ssn_scale_image(size_t source_stride, int source_width, int source_height, const DWORD* source_pixels, size_t destination_stride, int destination_width, int destination_height, DWORD* destination_pixels)
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
		const DWORD* source_row[2] = { (const DWORD*)((UINT_PTR)source_pixels + ((SIZE_T)source_y_i * source_stride)), (const DWORD*)((UINT_PTR)source_pixels + (((SIZE_T)source_y_i + 1) * source_stride)) };
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

DWORD ssn_load_image_file(const char* file_name, HMODULE gdipluss, HANDLE heap, int* image_width, int* image_height, DWORD** image_pixels)
{
	static const DWORD gdiplus_status_error_codes[] = { /*Ok = 0*/ ERROR_SUCCESS, /*GenericError = 1*/ ERROR_UNIDENTIFIED_ERROR, /*InvalidParameter = 2*/ ERROR_INVALID_PARAMETER, /*OutOfMemory = 3*/ ERROR_OUTOFMEMORY, /*ObjectBusy = 4*/ ERROR_BUSY, /*InsufficientBuffer = 5*/ ERROR_INSUFFICIENT_BUFFER, /*NotImplemented = 6*/ ERROR_NOT_SUPPORTED, /*Win32Error = 7*/ ERROR_UNIDENTIFIED_ERROR, /*WrongState = 8*/ ERROR_INVALID_STATE, /*Aborted = 9*/ ERROR_OPERATION_ABORTED, /*FileNotFound = 10*/ ERROR_FILE_NOT_FOUND, /*ValueOverflow = 11*/ ERROR_INVALID_DATA, /*AccessDenied = 12*/ ERROR_ACCESS_DENIED, /*UnknownImageFormat = 13*/ ERROR_UNSUPPORTED_TYPE, /*FontFamilyNotFound = 14*/ ERROR_NOT_SUPPORTED, /*FontStyleNotFound = 15*/ ERROR_NOT_SUPPORTED, /*NotTrueTypeFont = 16*/ ERROR_NOT_SUPPORTED, /*UnsupportedGdiplusVersion = 17*/ ERROR_NOT_SUPPORTED, /*GdiplusNotInitialized = 18*/ ERROR_INVALID_STATE, /*PropertyNotFound = 19*/ ERROR_NOT_SUPPORTED, /*PropertyNotSupported = 20*/ ERROR_NOT_SUPPORTED, /*ProfileNotFound = 21*/ ERROR_UNIDENTIFIED_ERROR };
	DWORD error;

	int file_name_length = 0;
	while (file_name[file_name_length])
	{
		if (file_name_length == (int)(((unsigned int)~0) >> 1))
		{
			error = ERROR_INVALID_PARAMETER;
			return error;
		}
		++file_name_length;
	}
	int native_file_name_length = MultiByteToWideChar(CP_UTF8, 0, file_name, file_name_length, 0, 0);
	if (!native_file_name_length)
	{
		error = GetLastError();
		return error;
	}
	WCHAR* native_file_name = HeapAlloc(heap, 0, ((size_t)native_file_name_length + 1) * sizeof(WCHAR));
	if (!native_file_name)
	{
		error = GetLastError();
		return error;
	}
	if (MultiByteToWideChar(CP_UTF8, 0, file_name, file_name_length, native_file_name, native_file_name_length) != native_file_name_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, native_file_name);
		return error;
	}
	native_file_name[native_file_name_length] = 0;

	int gdipluss_loaded;
	if (gdipluss)
		gdipluss_loaded = 0;
	else
	{
		gdipluss_loaded = 1;
		gdipluss = LoadLibraryW(L"Gdiplus.dll");
		if (!gdipluss)
		{
			error = GetLastError();
			HeapFree(heap, 0, native_file_name);
			return error;
		}
	}

	int (WINAPI * GdiplusStartup)(void** token, const void* input, void* output) = (int (WINAPI*)(void**, const void*, void*))GetProcAddress(gdipluss, "GdiplusStartup");
	void (WINAPI * GdiplusShutdown)(void* token) = (void (WINAPI*)(void*))GetProcAddress(gdipluss, "GdiplusShutdown");
	int (WINAPI * GdipCreateBitmapFromFileICM)(const WCHAR * file_name, void** bitmap_object) = (int (WINAPI*)(const WCHAR*, void**))GetProcAddress(gdipluss, "GdipCreateBitmapFromFileICM");
	int (WINAPI * GdipCreateHBITMAPFromBitmap)(void* bitmap_object, HBITMAP * bitmap_handle, DWORD background) = (int (WINAPI*)(void*, HBITMAP*, DWORD))GetProcAddress(gdipluss, "GdipCreateHBITMAPFromBitmap");
	int (WINAPI * GdipDisposeImage)(void* bitmap_object) = (int (WINAPI*)(void*))GetProcAddress(gdipluss, "GdipDisposeImage");

	struct { UINT32 GdiplusVersion; void* DebugEventCallback; BOOL SuppressBackgroundThread; BOOL SuppressExternalCodecs; } gdiplus_startup_input = { 1, 0, FALSE, FALSE };
	struct { void* NotificationHook; void* NotificationUnhook; } gdiplus_startup_output;
	void* gdiplus_token;

	int gdiplus_status = GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, &gdiplus_startup_output);
	error = ((size_t)gdiplus_status < (sizeof(gdiplus_status_error_codes) / sizeof(*gdiplus_status_error_codes))) ? gdiplus_status_error_codes[(size_t)gdiplus_status] : ERROR_UNIDENTIFIED_ERROR;
	if (error)
	{
		HeapFree(heap, 0, native_file_name);
		return error;
	}

	void* bitmap_object;
	gdiplus_status = GdipCreateBitmapFromFileICM(native_file_name, &bitmap_object);
	error = ((size_t)gdiplus_status < (sizeof(gdiplus_status_error_codes) / sizeof(*gdiplus_status_error_codes))) ? gdiplus_status_error_codes[(size_t)gdiplus_status] : ERROR_UNIDENTIFIED_ERROR;
	HeapFree(heap, 0, native_file_name);
	if (error)
	{
		GdiplusShutdown(gdiplus_token);
		return error;
	}

	HBITMAP bitmap;
	gdiplus_status = GdipCreateHBITMAPFromBitmap(bitmap_object, &bitmap, 0xFF000000);
	error = ((size_t)gdiplus_status < (sizeof(gdiplus_status_error_codes) / sizeof(*gdiplus_status_error_codes))) ? gdiplus_status_error_codes[(size_t)gdiplus_status] : ERROR_UNIDENTIFIED_ERROR;
	GdipDisposeImage(bitmap_object);
	GdiplusShutdown(gdiplus_token);
	if (gdipluss_loaded)
		FreeLibrary(gdipluss);
	if (error)
		return error;

	BITMAP bitmap_info;
	if (!GetObjectW(bitmap, sizeof(BITMAP), &bitmap_info))
	{
		error = GetLastError();
		DeleteObject(bitmap);
		return error;
	}

	DWORD* image = HeapAlloc(heap, 0, (size_t)bitmap_info.bmWidth * (size_t)bitmap_info.bmHeight * sizeof(DWORD));
	if (!image)
	{
		error = GetLastError();
		DeleteObject(bitmap);
		return error;
	}

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
	HDC device_context = CreateCompatibleDC(0);
	error = GetDIBits(device_context, bitmap, 0, (UINT)bitmap_info.bmHeight, image, (BITMAPINFO*)&image_bitmap_header, DIB_RGB_COLORS) ? 0 : GetLastError();
	if (device_context)
		DeleteDC(device_context);
	DeleteObject(bitmap);
	if (error)
	{
		HeapFree(heap, 0, image);
		return 0;
	}

	*image_width = (int)image_bitmap_header.biWidth;
	*image_height = (int)image_bitmap_header.biHeight;
	*image_pixels = image;
	return 0;
}

DWORD ssn_create_window(DWORD ex_style, const WNDCLASSEXW* window_class, const char* window_name, DWORD style, int y, int x, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, void* parameter, HWND* window_handle_adress)
{
	DWORD error;
	if (window_class->cbSize != sizeof(WNDCLASSEXW) || window_class->hInstance != instance || width < 0 || height < 0)
	{
		error = ERROR_INVALID_PARAMETER;
		return error;
	}

	HANDLE heap = GetProcessHeap();
	if (!heap)
	{
		error = GetLastError();
		return error;
	}

	if (style != WS_OVERLAPPED)
	{
		RECT window_area = { 0, 0, width, height };
		if (!AdjustWindowRectEx(&window_area, style, menu ? TRUE : FALSE, ex_style))
		{
			error = GetLastError();
			return error;
		}
		width = window_area.right - window_area.left;
		height = window_area.bottom - window_area.top;
	}

	if (!instance)
		instance = GetModuleHandleW(0);

	WCHAR window_class_name[23]; /* L"<hex process id>-<hex current file time low 32 bits>-<hex 16 bit sequence number>" */
	WNDCLASSEXW full_window_class;
	full_window_class.cbSize = sizeof(WNDCLASSEXW);
	full_window_class.style = window_class->style ? window_class->style : (CS_HREDRAW | CS_VREDRAW);
	full_window_class.lpfnWndProc = window_class->lpfnWndProc;
	full_window_class.cbClsExtra = window_class->cbClsExtra;
	full_window_class.cbWndExtra = window_class->cbWndExtra;
	full_window_class.hInstance = instance;
	full_window_class.hIcon = window_class->hIcon ? window_class->hIcon : LoadIconW(0, IDI_APPLICATION);
	full_window_class.hCursor = window_class->hCursor ? window_class->hCursor : LoadCursorW(0, IDC_ARROW);
	full_window_class.hbrBackground = window_class->hbrBackground;
	full_window_class.lpszMenuName = window_class->lpszMenuName;
	full_window_class.lpszClassName = window_class_name;
	full_window_class.hIconSm = window_class->hIconSm ? window_class->hIconSm : (HICON)LoadImageW(instance, MAKEINTRESOURCEW(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

	static const WCHAR hex_table[16] = {
		L'0', L'1', L'2', L'3',
		L'4', L'5', L'6', L'7',
		L'8', L'9', L'A', L'B',
		L'C', L'D', L'E', L'F' };
	for (DWORD process_id = GetCurrentProcessId(), i = 0; i != 8; ++i)
		window_class_name[i] = hex_table[(process_id >> ((7 - i) << 2)) & 0xF];
	window_class_name[8] = '-';
	FILETIME current_time;
	GetSystemTimeAsFileTime(&current_time);
	for (DWORD current_time_low = current_time.dwLowDateTime, i = 0; i != 8; ++i)
		window_class_name[9 + i] = hex_table[(current_time_low >> ((7 - i) << 2)) & 0xF];
	window_class_name[17] = '-';
	window_class_name[22] = 0;
	for (DWORD not_registered = 1, sequence_number = 0; not_registered;)
	{
		for (DWORD i = 0; i != 4; ++i)
			window_class_name[18 + i] = hex_table[(sequence_number >> ((3 - i) << 2)) & 0xF];
		if (RegisterClassExW(&full_window_class))
			not_registered = 0;
		else
		{
			error = GetLastError();
			if (error != ERROR_CLASS_ALREADY_EXISTS || sequence_number == 0xFFFF)
				return error;
			else
				++sequence_number;
		}
	}

	int window_name_length = 0;
	while (window_name[window_name_length])
	{
		if (window_name_length == (int)(((unsigned int)~0) >> 1))
		{
			error = ERROR_INVALID_PARAMETER;
			UnregisterClassW(window_class_name, instance);
			return error;
		}
		++window_name_length;
	}
	int native_window_name_length = MultiByteToWideChar(CP_UTF8, 0, window_name, window_name_length, 0, 0);
	if (!native_window_name_length)
	{
		error = GetLastError();
		UnregisterClassW(window_class_name, instance);
		return error;
	}
	WCHAR* native_window_name = HeapAlloc(heap, 0, ((size_t)native_window_name_length + 1) * sizeof(WCHAR));
	if (!native_window_name)
	{
		error = GetLastError();
		UnregisterClassW(window_class_name, instance);
		return error;
	}
	if (MultiByteToWideChar(CP_UTF8, 0, window_name, window_name_length, native_window_name, native_window_name_length) != native_window_name_length)
	{
		error = GetLastError();
		HeapFree(heap, 0, native_window_name);
		UnregisterClassW(window_class_name, instance);
		return error;
	}
	native_window_name[native_window_name_length] = 0;

	HWND window_handle = CreateWindowExW(ex_style, window_class_name, native_window_name, style, x, y, width, height, parent, menu, instance, parameter);
	HeapFree(heap, 0, native_window_name);
	if (!window_handle)
	{
		error = GetLastError();
		UnregisterClassW(window_class_name, instance);
		return error;
	}

	*window_handle_adress = window_handle;
	return 0;
}

void ssn_destroy_window(HWND window_handle)
{
	WCHAR window_class_name[257];/* The maximum length for window class name is 256 */
	if (!RealGetWindowClassW(window_handle, window_class_name, sizeof(window_class_name) / sizeof(*window_class_name)))
	{
		HINSTANCE instance = (HINSTANCE)GetWindowLongPtrW(window_handle, GWLP_HINSTANCE);
		DestroyWindow(window_handle);
		UnregisterClassW(window_class_name, instance);
	}
	else
		ExitProcess(EXIT_FAILURE);
}

void ssn_display_window_draw_image(size_t source_stride, int source_width, int source_height, const DWORD* source_pixels, size_t destination_stride, int destination_width, int destination_height, DWORD* destination_pixels)
{
	int x;
	int y;
	int width;
	int height;
	ssn_fit_rectangle_in_frame(destination_width, destination_height, source_width, source_height, &x, &y, &width, &height);

	// fix later
	ssn_clear_memory(destination_pixels, (size_t)destination_height * destination_stride);

	ssn_scale_image(source_stride, source_width, source_height, source_pixels, destination_stride, width, height,
		(DWORD*)((uintptr_t)destination_pixels + ((size_t)y * destination_stride) + ((size_t)x * sizeof(DWORD))));
}

LRESULT CALLBACK ssn_display_window_procedure(HWND window, UINT message, WPARAM w_parameter, LPARAM l_parameter)
{
	display_window_instance_t* display = (display_window_instance_t*)GetWindowLongPtrW(window, GWLP_USERDATA);
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
				display->bitmap_header.biHeight - update_rectangle.bottom,
				update_rectangle.right - update_rectangle.left,
				update_rectangle.bottom - update_rectangle.top,
				display->bitmap_pixels, (const BITMAPINFO*)&display->bitmap_header, DIB_RGB_COLORS, SRCCOPY);
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
					display->error = GetLastError();
					SetWindowPos(window, HWND_TOP, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
					display->running = FALSE;
					return 0;
				}
				int new_width = (int)(window_rectangle.right - window_rectangle.left);
				int new_height = (int)(window_rectangle.bottom - window_rectangle.top);
				if (display->bitmap_header.biWidth != new_width || display->bitmap_header.biHeight != new_height)
				{
					size_t required_bitmap_buffer_size = (size_t)new_width * (size_t)new_height * sizeof(DWORD);
					if (required_bitmap_buffer_size > display->bitmap_buffer_size)
					{
						DWORD* new_bitmap_pixels = (DWORD*)HeapReAlloc(display->heap, 0, display->bitmap_pixels, required_bitmap_buffer_size);
						if (!new_bitmap_pixels)
						{
							display->error = GetLastError();
							SetWindowPos(window, HWND_TOP, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
							display->running = FALSE;
							return 0;
						}
						display->bitmap_buffer_size = required_bitmap_buffer_size;
						display->bitmap_pixels = new_bitmap_pixels;
					}
					display->bitmap_header.biWidth = new_width;
					display->bitmap_header.biHeight = new_height;

					ssn_display_window_draw_image(
						display->image_width * sizeof(DWORD),
						display->image_width,
						display->image_height,
						display->image_pixels,
						display->bitmap_header.biWidth * sizeof(DWORD),
						display->bitmap_header.biWidth,
						display->bitmap_header.biHeight,
						display->bitmap_pixels);

					InvalidateRect(window, 0, 0);
				}
			}
			return 0;
		}
		case WM_ERASEBKGND:
			return 1;
		case WM_TIMER:
			return 0;
		case WM_KEYDOWN:
			return 0;
		case WM_DESTROY:
			return 0;
		case WM_CLOSE:
		case WM_QUIT:
		{
			ssn_clear_memory(display->bitmap_pixels, (size_t)display->bitmap_header.biWidth * (size_t)display->bitmap_header.biHeight * sizeof(DWORD));
			StretchDIBits(display->device_context, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, display->bitmap_pixels, (const BITMAPINFO*)&display->bitmap_header, DIB_RGB_COLORS, SRCCOPY);
			ValidateRect(window, 0);
			display->running = FALSE;
			return 0;
		}
		case WM_CREATE:
		{
			display = (display_window_instance_t*)((CREATESTRUCT*)l_parameter)->lpCreateParams;
			SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)display);
			SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			if (GetWindowLongPtrW(window, GWLP_USERDATA) != (LONG_PTR)display)
			{
				display->error = GetLastError();
				return (LRESULT)-1;
			}
			HANDLE heap = GetProcessHeap();
			if (!heap)
			{
				display->error = GetLastError();
				return (LRESULT)-1;
			}
			RECT window_rectangle;
			if (!GetClientRect(window, &window_rectangle))
			{
				display->error = GetLastError();
				return (LRESULT)-1;
			}
			HDC device_context = GetDC(window);
			if (!device_context)
			{
				display->error = GetLastError();
				return (LRESULT)-1;
			}

			display->window_handle = window;
			display->device_context = device_context;
			display->heap = heap;
			display->running = TRUE;
			display->error = 0;
			display->bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
			display->bitmap_header.biWidth = (window_rectangle.right - window_rectangle.left);
			display->bitmap_header.biHeight = (window_rectangle.bottom - window_rectangle.top);
			display->bitmap_header.biPlanes = 1;
			display->bitmap_header.biBitCount = 32;
			display->bitmap_header.biCompression = BI_RGB;
			display->bitmap_header.biSizeImage = 0;
			display->bitmap_header.biXPelsPerMeter = 0;
			display->bitmap_header.biYPelsPerMeter = 0;
			display->bitmap_header.biClrUsed = 0;
			display->bitmap_header.biClrImportant = 0;

			display->bitmap_buffer_size = (size_t)display->bitmap_header.biWidth * (size_t)display->bitmap_header.biHeight * sizeof(DWORD);
			display->bitmap_pixels = (DWORD*)HeapAlloc(display->heap, 0, display->bitmap_buffer_size);
			if (!display->bitmap_pixels)
			{
				display->error = GetLastError();
				ReleaseDC(window, display->device_context);
				return (LRESULT)-1;
			}

			ssn_display_window_draw_image(
				display->image_width * sizeof(DWORD),
				display->image_width,
				display->image_height,
				display->image_pixels,
				display->bitmap_header.biWidth * sizeof(DWORD),
				display->bitmap_header.biWidth,
				display->bitmap_header.biHeight,
				display->bitmap_pixels);

			StretchDIBits(display->device_context, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, 0, 0, (int)display->bitmap_header.biWidth, (int)display->bitmap_header.biHeight, display->bitmap_pixels, (const BITMAPINFO*)&display->bitmap_header, DIB_RGB_COLORS, SRCCOPY);
			ValidateRect(window, 0);
			return (LRESULT)0;
		}
		default:
			return DefWindowProcW(window, message, w_parameter, l_parameter);
	}
}

DWORD ssn_display_window(const char* window_title, int image_width, int image_height, DWORD* image_pixels)
{
	display_window_instance_t instance;
	instance.image_width = image_width;
	instance.image_height = image_height;
	instance.image_buffer_size = (size_t)image_height * (size_t)image_width * sizeof(DWORD);
	instance.image_pixels = image_pixels;

	WNDCLASSEXW window_class;
	window_class.cbSize = sizeof(WNDCLASSEXW);
	window_class.style = 0;
	window_class.lpfnWndProc = ssn_display_window_procedure;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = 0;
	window_class.hIcon = 0;
	window_class.hCursor = 0;
	window_class.hbrBackground = 0;
	window_class.lpszMenuName = 0;
	window_class.lpszClassName = 0;
	window_class.hIconSm = 0;

	HWND window;
	DWORD error = ssn_create_window(WS_EX_APPWINDOW, &window_class, "Test", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, 0, 0, 0, &instance, &window);
	if (error)
		return error;

	for (BOOL loop = instance.running ? GetMessageW(&instance.message, instance.window_handle, 0, 0) : FALSE; instance.running && loop && loop != (BOOL)-1; loop = GetMessageW(&instance.message, instance.window_handle, 0, 0))
	{
		TranslateMessage(&instance.message);
		DispatchMessageW(&instance.message);
	}

	ssn_destroy_window(window);
	return 0;
}

#ifdef __cplusplus
}
#endif