#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <Windows.h>

void gdt_scale_image(SIZE_T source_stride, DWORD source_width, DWORD source_height, const DWORD* source_pixels, SIZE_T destination_stride, DWORD destination_width, DWORD destination_height, DWORD* destination_pixels)
{
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
}

DWORD gdt_get_last_error()
{
	DWORD error = GetLastError();
	return error ? error : ERROR_UNIDENTIFIED_ERROR;
}

void gdt_print_window_class_name(DWORD gdt_window_class_identifier, WCHAR* window_class_name)
{
	for (DWORD process = GetCurrentProcessId(), i = 0; i != 8; ++i)
		window_class_name[i] = (WCHAR)L"0123456789ABCDEF"[((process >> ((7 - i) << 2)) & 0xF)];
	for (DWORD i = 0; i != 7; ++i)
		window_class_name[8 + i] = (WCHAR)L"0123456789ABCDEF"[((gdt_window_class_identifier >> ((6 - i) << 2)) & 0xF)];
	window_class_name[15] = 0;
}

DWORD gdt_register_window_class(const WNDCLASSEXW* window_class_info, DWORD* gdt_window_class_identifier)
{
	WCHAR class_name[16];
	WNDCLASSEXW class_info = *window_class_info;
	class_info.lpszClassName = class_name;
	for (DWORD class_index = 0; class_index != 0x10000000; ++class_index)
	{
		gdt_print_window_class_name(class_index, class_name);
		ATOM atom = RegisterClassExW(&class_info);
		if (atom)
		{
			*gdt_window_class_identifier = class_index;
			return 0;
		}
		DWORD error = gdt_get_last_error();
		if (error != ERROR_CLASS_ALREADY_EXISTS)
			return error;
	}
	return ERROR_CLASS_ALREADY_EXISTS;
}

void gdt_unregister_window_class(HINSTANCE instance, DWORD gdt_window_class_identifier)
{
	WCHAR class_name[16];
	gdt_print_window_class_name(gdt_window_class_identifier, class_name);
	UnregisterClassW(class_name, instance);
}

void gdt_scale_rectengle_in_rectangle(const RECT* bounding_rectangle, const RECT* rectangle, RECT* scaled_rectangle)
{
	ULONGLONG container_width = (ULONGLONG)(bounding_rectangle->right - bounding_rectangle->left) << 16;
	ULONGLONG container_height = (ULONGLONG)(bounding_rectangle->bottom - bounding_rectangle->top) << 16;
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
	scaled_rectangle->left = (LONG)((container_width - width) >> 17);
	scaled_rectangle->top = (LONG)((container_height - height) >> 17);
	scaled_rectangle->right = scaled_rectangle->left + (LONG)(width >> 16);
	scaled_rectangle->bottom = scaled_rectangle->top + (LONG)(height >> 16);
}

DWORD gdt_create_window(HINSTANCE instance, DWORD window_class_identifier, DWORD style, DWORD extended_style, LPVOID creation_parameter, LONG width, LONG height, const WCHAR* title, HWND* window)
{
	WCHAR class_name[16];
	gdt_print_window_class_name(window_class_identifier, class_name);
	RECT rectangle = { 0, 0, 0, 0 };
	if (width != CW_USEDEFAULT || height != CW_USEDEFAULT)
	{
		rectangle.right = (width != CW_USEDEFAULT) ? width : (LONG)GetSystemMetrics(SM_CXSCREEN);
		rectangle.bottom = (height != CW_USEDEFAULT) ? height : (LONG)GetSystemMetrics(SM_CYSCREEN);
		if (!AdjustWindowRectEx(&rectangle, style, FALSE, extended_style))
			return gdt_get_last_error();
	}
	HWND window_handle = CreateWindowExW(extended_style, class_name, title, style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		(width != CW_USEDEFAULT) ? (int)(rectangle.right - rectangle.left) : CW_USEDEFAULT,
		(height != CW_USEDEFAULT) ? (int)(rectangle.bottom - rectangle.top) : CW_USEDEFAULT,
		0, 0, instance, creation_parameter);
	if (!window_handle)
		return gdt_get_last_error();
	*window = window_handle;
	return 0;
}

struct gdt_image_t
{
	LONG width;
	LONG height;
	DWORD* pixels;
};

#define LDL_DISPLAY_STATUS_INITIALIZE 0
#define LDL_DISPLAY_STATUS_RUNNING 1
#define LDL_DISPLAY_STATUS_WAITING 2
#define LDL_DISPLAY_STATUS_CLOSED 3
struct gdt_display_t
{
	volatile struct gdt_display_t* next_image;
	volatile HWND window;
	volatile DWORD status;
	volatile DWORD error;
	volatile BOOL is_client_waiting;
	volatile HANDLE client_waiting_event;
	volatile HANDLE close_event;
	volatile HANDLE exit_event;
	HANDLE heap;
};

struct gdt_window_t
{
	BOOL run;
	HDC device_context;
	BITMAPINFOHEADER window_bitmap_header;
	DWORD* window_bitmap_pixels;
	HBRUSH black_brush;
	SIZE_T window_bitmap_buffer_size;
	SIZE_T image_bitmap_buffer_size;
	struct gdt_image_t image;
	volatile BOOL* exit_signal;
	HANDLE heap;
	DWORD error;
};

void gdt_draw_window_image(struct gdt_window_t* window)
{
	RECT window_rectangle = { 0, 0, window->window_bitmap_header.biWidth, window->window_bitmap_header.biHeight };
	RECT image_rectangle = { 0, 0, window->image.width, window->image.height };
	RECT output_rectangle;
	gdt_scale_rectengle_in_rectangle(&window_rectangle, &image_rectangle, &output_rectangle);
	DWORD* source_pixels = window->image.pixels;
	DWORD* destination_pixels = window->window_bitmap_pixels;
	float y_end = (float)(output_rectangle.bottom - output_rectangle.top);
	float x_end = (float)(output_rectangle.right - output_rectangle.left);
	float y_scale = (float)window->image.height / y_end;
	float x_scale = (float)window->image.width / x_end;
	for (DWORD* e = destination_pixels + output_rectangle.top * window->window_bitmap_header.biWidth; destination_pixels != e;)
		*destination_pixels++ = 0;
	for (float y = 0.0f; y < y_end; y += 1.0f)
	{
		const DWORD* source_row = (const DWORD*)((UINT_PTR)source_pixels + ((SIZE_T)(y * y_scale) * ((SIZE_T)window->image.width * sizeof(DWORD))));
		DWORD* destination_row = (DWORD*)((UINT_PTR)destination_pixels + ((SIZE_T)y * ((SIZE_T)window->window_bitmap_header.biWidth * sizeof(DWORD))));
		for (DWORD* e = destination_row + output_rectangle.left; destination_row != e;)
			*destination_row++ = 0;
		for (float x = 0.0f; x < x_end; x += 1.0f)
			*destination_row++ = source_row[(SIZE_T)(x * x_scale)];
		for (DWORD* e = destination_row + (window->window_bitmap_header.biWidth - (output_rectangle.left + (output_rectangle.bottom - output_rectangle.top))); destination_row != e;)
			*destination_row++ = 0;
	}
	destination_pixels = window->window_bitmap_pixels + output_rectangle.bottom * window->window_bitmap_header.biWidth;
	for (DWORD* e = window->window_bitmap_pixels + window->window_bitmap_header.biWidth * window->window_bitmap_header.biHeight; destination_pixels != e;)
		*destination_pixels++ = 0;
}

LRESULT CALLBACK gdt_window_procedure(HWND window, UINT message, WPARAM w_parameter, LPARAM l_parameter)
{
	struct gdt_window_t* gui = (struct gdt_window_t*)GetWindowLongPtrW(window, GWLP_USERDATA);
	switch (message)
	{
		case WM_PAINT:
		{
			if (gui)
			{
				RECT update_rectangle;
				GetUpdateRect(window, &update_rectangle, FALSE);
				if (gui->run)
					StretchDIBits(gui->device_context,
						update_rectangle.left,
						update_rectangle.top,
						update_rectangle.right - update_rectangle.left,
						update_rectangle.bottom - update_rectangle.top,
						update_rectangle.left,
						gui->window_bitmap_header.biHeight - update_rectangle.bottom,
						update_rectangle.right - update_rectangle.left,
						update_rectangle.bottom - update_rectangle.top,
						gui->window_bitmap_pixels, (const BITMAPINFO*)&gui->window_bitmap_header, DIB_RGB_COLORS, SRCCOPY);
				else
					FillRect(gui->device_context, &update_rectangle, gui->black_brush);
				ValidateRect(window, &update_rectangle);
			}
			else
				ValidateRect(window, 0);
			return 0;
		}
		case WM_USER:
		{
			if (gui)
			{
				struct gdt_image_t* image = (struct gdt_image_t*)w_parameter;
				SIZE_T required_image_bitmap_buffer_size = (((SIZE_T)image->width * (SIZE_T)image->height * sizeof(DWORD)) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1);
				if (gui->image_bitmap_buffer_size < required_image_bitmap_buffer_size)
				{
					LPVOID new_window_buffer = HeapReAlloc(gui->heap, 0, gui, ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + gui->window_bitmap_buffer_size + required_image_bitmap_buffer_size);
					if (!new_window_buffer)
					{
						gui->error = gdt_get_last_error();
						gui->run = FALSE;
						return 0;
					}
					if (new_window_buffer != (LPVOID)gui)
					{
						gui = (struct gdt_window_t*)new_window_buffer;
						gui->window_bitmap_pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)));
						gui->image.pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + gui->window_bitmap_buffer_size);
						SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)gui);
						SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
					}
					gui->image_bitmap_buffer_size = required_image_bitmap_buffer_size;
				}
				gui->image.width = image->width;
				gui->image.height = image->height;
				CopyMemory(gui->image.pixels, image->pixels, (SIZE_T)image->width * (SIZE_T)image->height * sizeof(DWORD));
				gdt_draw_window_image(gui);
				MemoryBarrier();
				*(volatile BOOL*)l_parameter = FALSE;
				MemoryBarrier();
				InvalidateRect(window, 0, 0);
			}
			return 0;
		}
		case WM_SIZE:
		{
			if (gui)
			{
				if (w_parameter != SIZE_MINIMIZED)
				{
					RECT window_rectangle;
					if (GetClientRect(window, &window_rectangle))
					{
						BOOL set_window_pointer = FALSE;
						LONG width = window_rectangle.right - window_rectangle.left;
						LONG height = window_rectangle.bottom - window_rectangle.top;
						SIZE_T required_window_bitmap_buffer_size = (((SIZE_T)width * (SIZE_T)height * sizeof(DWORD)) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1);
						if (required_window_bitmap_buffer_size > gui->window_bitmap_buffer_size)
						{
							LPVOID new_window_buffer = HeapReAlloc(gui->heap, 0, gui, ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + required_window_bitmap_buffer_size + gui->image_bitmap_buffer_size);
							if (!new_window_buffer)
							{
								gui->error = gdt_get_last_error();
								gui->run = FALSE;
								return 0;
							}
							if (new_window_buffer != (LPVOID)gui)
							{
								set_window_pointer = TRUE;
								gui = (struct gdt_window_t*)new_window_buffer;
								gui->window_bitmap_pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)));
							}
							gui->image.pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + required_window_bitmap_buffer_size);
							DWORD* old_image_pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + gui->window_bitmap_buffer_size);
							MoveMemory(gui->image.pixels, old_image_pixels, (SIZE_T)gui->image.width * (SIZE_T)gui->image.height * sizeof(DWORD));
							gui->window_bitmap_buffer_size = required_window_bitmap_buffer_size;
						}
						gui->window_bitmap_header.biWidth = width;
						gui->window_bitmap_header.biHeight = height;
						gdt_draw_window_image(gui);
						if (set_window_pointer)
						{
							SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)gui);
							SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
						}
						InvalidateRect(window, 0, 0);
					}
					else
					{
						gui->error = gdt_get_last_error();
						gui->run = FALSE;
						return 0;
					}
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
		{
			gui->run = FALSE;
			volatile BOOL* exit_signal = gui->exit_signal;
			if (exit_signal)
			{
				MemoryBarrier();
				*exit_signal = TRUE;
				MemoryBarrier();
			}
			return 0;
		}
		case WM_DESTROY:
		{
			if (gui)
			{
				SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
				RECT window_rectangle = { 0, 0, gui->window_bitmap_header.biWidth, gui->window_bitmap_header.biHeight };
				FillRect(gui->device_context, &window_rectangle, gui->black_brush);
				ValidateRect(window, 0);
				HDC device_context = gui->device_context;
				HBRUSH black_brush = gui->black_brush;
				DWORD error = gui->error;
				HANDLE heap = gui->heap;
				HeapFree(heap, 0, gui);
				DeleteObject((HGDIOBJ)black_brush);
				ReleaseDC(window, device_context);
				SetLastError(error);
			}
			return 0;
		}
		case WM_QUIT:
			return 0;
		case WM_CREATE:
		{
			DWORD error;
			RECT window_rectangle;
			if (!GetClientRect(window, &window_rectangle))
			{
				SetLastError(gdt_get_last_error());
				return (LRESULT)-1;
			}
			HDC device_context = GetDC(window);
			if (!device_context)
			{
				SetLastError(gdt_get_last_error());
				return (LRESULT)-1;
			}
			HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));
			if (!black_brush)
			{
				error = gdt_get_last_error();
				ReleaseDC(window, device_context);
				SetLastError(error);
				return (LRESULT)-1;
			}
			if (!FillRect(device_context, &window_rectangle, black_brush) || !ValidateRect(window, 0))
			{
				error = gdt_get_last_error();
				DeleteObject((HGDIOBJ)black_brush);
				ReleaseDC(window, device_context);
				SetLastError(error);
				return (LRESULT)-1;
			}
			HANDLE heap = GetProcessHeap();
			if (!heap)
			{
				error = gdt_get_last_error();
				DeleteObject((HGDIOBJ)black_brush);
				ReleaseDC(window, device_context);
				SetLastError(error);
				return (LRESULT)-1;
			}
			LONG width = window_rectangle.right - window_rectangle.left;
			LONG height = window_rectangle.bottom - window_rectangle.top;
			SIZE_T required_window_bitmap_buffer_size = (((SIZE_T)width * (SIZE_T)height * sizeof(DWORD)) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1);
			struct gdt_window_t* gui = (struct gdt_window_t*)HeapAlloc(heap, HEAP_ZERO_MEMORY, ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + (2 * required_window_bitmap_buffer_size));
			if (!gui)
			{
				error = gdt_get_last_error();
				DeleteObject((HGDIOBJ)black_brush);
				ReleaseDC(window, device_context);
				SetLastError(error);
				return (LRESULT)-1;
			}
			SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)gui);
			SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			if (GetWindowLongPtrW(window, GWLP_USERDATA) != (LONG_PTR)gui)
			{
				error = gdt_get_last_error();
				HeapFree(heap, 0, gui);
				DeleteObject((HGDIOBJ)black_brush);
				ReleaseDC(window, device_context);
				SetLastError(error);
				return (LRESULT)-1;
			}
			gui->run = TRUE;
			gui->device_context = device_context;
			gui->window_bitmap_header.biSize = sizeof(BITMAPINFOHEADER);
			gui->window_bitmap_header.biWidth = width;
			gui->window_bitmap_header.biHeight = height;
			gui->window_bitmap_header.biPlanes = 1;
			gui->window_bitmap_header.biBitCount = 32;
			gui->window_bitmap_header.biCompression = BI_RGB;
			gui->window_bitmap_pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)));
			gui->black_brush = black_brush;
			gui->window_bitmap_buffer_size = required_window_bitmap_buffer_size;
			gui->image_bitmap_buffer_size = required_window_bitmap_buffer_size;
			gui->image.width = width;
			gui->image.height = height;
			gui->image.pixels = (DWORD*)((UINT_PTR)gui + ((sizeof(struct gdt_window_t) + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1)) + gui->window_bitmap_buffer_size);
			gui->exit_signal = (volatile BOOL*)((CREATESTRUCTA*)l_parameter)->lpCreateParams;
			gui->heap = heap;
			gui->error = 0;
			return (LRESULT)0;
		}
		default:
			return DefWindowProcW(window, message, w_parameter, l_parameter);
	}
}

DWORD gdt_create_gui_window_class(HINSTANCE instance, DWORD* gdt_window_class_idetifier)
{
	WNDCLASSEXW window_class_info;
	window_class_info.cbSize = sizeof(WNDCLASSEXW);
	window_class_info.style = CS_HREDRAW | CS_VREDRAW;
	window_class_info.lpfnWndProc = gdt_window_procedure;
	window_class_info.cbClsExtra = 0;
	window_class_info.cbWndExtra = 0;
	window_class_info.hInstance = instance;
	window_class_info.hIcon = 0;// LoadIconW(0, (const WCHAR*)IDI_APPLICATION);
	window_class_info.hCursor = LoadCursorW(0, (const WCHAR*)IDC_ARROW);
	window_class_info.hbrBackground = 0;
	window_class_info.lpszMenuName = 0;
	window_class_info.lpszClassName = 0;
	window_class_info.hIconSm = 0;// (HICON)LoadImageW(window_class_info.hInstance, MAKEINTRESOURCEW(5), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	return gdt_register_window_class(&window_class_info, gdt_window_class_idetifier);
}

void gdt_gui_process_exit(struct gdt_display_t* display, DWORD error, HINSTANCE instance, DWORD* gdt_window_class_idetifier)
{
	HWND window_handle = display->window;
	if (window_handle)
		ShowWindow(window_handle, SW_HIDE);
	HANDLE client_waiting_event = display->client_waiting_event;
	HANDLE close_event = display->close_event;
	HANDLE exit_event = display->exit_event;
	display->error = error;
	MemoryBarrier();
	display->status = LDL_DISPLAY_STATUS_WAITING;
	MemoryBarrier();
	SetEvent(client_waiting_event);
	WaitForSingleObject(close_event, INFINITE);
	display->status = LDL_DISPLAY_STATUS_CLOSED;
	SetEvent(exit_event);
	if (window_handle)
		DestroyWindow(window_handle);
	if (*gdt_window_class_idetifier)
		gdt_unregister_window_class(instance, *gdt_window_class_idetifier);
	ExitThread(0);
}

DWORD CALLBACK gdt_gui_process(LPVOID parameter)
{
	volatile BOOL exit_signal = FALSE;
	struct gdt_display_t* display = (struct gdt_display_t*)parameter;
	HINSTANCE instance = (HINSTANCE)GetModuleHandleW(0);
	DWORD gdt_window_class_idetifier;
	DWORD error = gdt_create_gui_window_class(instance, &gdt_window_class_idetifier);
	if (error)
		gdt_gui_process_exit(display, error, 0, 0);
	HWND window_handle;
	error = gdt_create_window(instance, gdt_window_class_idetifier, WS_OVERLAPPEDWINDOW | WS_VISIBLE, WS_EX_APPWINDOW | WS_EX_DLGMODALFRAME, (LPVOID)&exit_signal, CW_USEDEFAULT, CW_USEDEFAULT, (const WCHAR*)display->next_image, &window_handle);
	if (error)
		gdt_gui_process_exit(display, error, instance, &gdt_window_class_idetifier);
	display->next_image = 0;
	display->window = window_handle;
	display->status = LDL_DISPLAY_STATUS_RUNNING;
	SetEvent(display->client_waiting_event);
	for (MSG window_message; !error && !exit_signal; MemoryBarrier())
		if (GetMessageW(&window_message, window_handle, 0, 0))
		{
			TranslateMessage(&window_message);
			DispatchMessageW(&window_message);
		}
		else
			error = gdt_get_last_error();
	gdt_gui_process_exit(display, error, instance, &gdt_window_class_idetifier);
	return 0;
}

DWORD gdt_create_display(struct gdt_display_t** new_display, const WCHAR* title)
{
	HANDLE heap = GetProcessHeap();
	if (!heap)
		return gdt_get_last_error();
	struct gdt_display_t* display = (struct gdt_display_t*)HeapAlloc(heap, 0, sizeof(struct gdt_display_t));
	if (!display)
		return gdt_get_last_error();
	DWORD error;
	HANDLE close_event = CreateEventW(0, FALSE, FALSE, 0);
	if (!close_event)
	{
		error = gdt_get_last_error();
		HeapFree(heap, 0, (LPVOID)display);
		return error;
	}
	HANDLE exit_event = CreateEventW(0, FALSE, FALSE, 0);
	if (!exit_event)
	{
		error = gdt_get_last_error();
		CloseHandle(close_event);
		HeapFree(heap, 0, (LPVOID)display);
		return error;
	}
	HANDLE client_waiting_event = CreateEventW(0, FALSE, FALSE, 0);
	if (!client_waiting_event)
	{
		error = gdt_get_last_error();
		CloseHandle(exit_event);
		CloseHandle(close_event);
		HeapFree(heap, 0, (LPVOID)display);
		return error;
	}
	display->next_image = (struct gdt_display_t*)title;
	display->window = 0;
	display->status = LDL_DISPLAY_STATUS_INITIALIZE;
	display->error = 0;
	display->is_client_waiting = TRUE;
	display->client_waiting_event = client_waiting_event;
	display->close_event = close_event;
	display->exit_event = exit_event;
	display->heap = heap;
	MemoryBarrier();
	HANDLE thread = CreateThread(0, 0, gdt_gui_process, (LPVOID)display, 0, 0);
	if (!thread)
	{
		error = gdt_get_last_error();
		CloseHandle(client_waiting_event);
		CloseHandle(exit_event);
		CloseHandle(close_event);
		HeapFree(heap, 0, (LPVOID)display);
		return error;
	}
	CloseHandle(thread);
	WaitForSingleObject(client_waiting_event, INFINITE);
	if (display->status == LDL_DISPLAY_STATUS_WAITING)
	{
		error = display->error;
		MemoryBarrier();
		SetEvent(close_event);
		WaitForSingleObject(exit_event, INFINITE);
		CloseHandle(client_waiting_event);
		CloseHandle(exit_event);
		CloseHandle(close_event);
		HeapFree(heap, 0, (LPVOID)display);
		return error;
	}
	*new_display = display;
	return 0;
}

int gdt_is_display_open(struct gdt_display_t* display)
{
	return display->status == LDL_DISPLAY_STATUS_RUNNING;
}

void gdt_close_display(struct gdt_display_t* display)
{
	HANDLE heap = display->heap;
	HANDLE client_waiting_event = display->client_waiting_event;
	HANDLE close_event = display->close_event;
	HANDLE exit_event = display->exit_event;
	HWND window_handle = display->window;
	PostMessageW(window_handle, WM_CLOSE, 0, 0);
	WaitForSingleObject(client_waiting_event, INFINITE);
	SetEvent(close_event);
	WaitForSingleObject(exit_event, INFINITE);
	CloseHandle(exit_event);
	CloseHandle(close_event);
	HeapFree(heap, 0, (LPVOID)display);
}

DWORD gdt_set_display_image(struct gdt_display_t* display, LONG width, LONG height, const DWORD* pixels)
{
	struct gdt_image_t image = { width, height, (DWORD*)pixels };
	volatile BOOL in_progress = TRUE;
	MemoryBarrier();
	PostMessageW(display->window, WM_USER, (WPARAM)&image, (LPARAM)&in_progress);
	while (in_progress)
	{
		if (display->status != LDL_DISPLAY_STATUS_RUNNING)
			return ERROR_CAN_NOT_COMPLETE;
		SwitchToThread();
		MemoryBarrier();
	}
	return 0;
}