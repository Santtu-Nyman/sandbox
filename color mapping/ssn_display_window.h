#ifdef __cplusplus
extern "C" {
#endif

#include <Windows.h>
#include <stddef.h>
#include <stdint.h>
#include <emmintrin.h>

void ssn_clear_memory(void* ptr, size_t size);

void ssn_fit_rectangle_in_frame(int frame_width, int frame_height, int rectangle_width, int rectangle_height, int* fitted_x, int* fitted_y, int* fitted_width, int* fitted_height);

void ssn_scale_image(size_t source_stride, int source_width, int source_height, const DWORD* source_pixels, size_t destination_stride, int destination_width, int destination_height, DWORD* destination_pixels);

DWORD ssn_load_image_file(const char* file_name, HMODULE gdipluss, HANDLE heap, int* image_width, int* image_height, DWORD** image_pixels);

DWORD ssn_create_window(DWORD ex_style, const WNDCLASSEXW* window_class, const char* window_name, DWORD style, int y, int x, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, void* parameter, HWND* window_handle_adress);

void ssn_destroy_window(HWND window_handle);

void ssn_display_window_draw_image(size_t source_stride, int source_width, int source_height, const DWORD* source_pixels, size_t destination_stride, int destination_width, int destination_height, DWORD* destination_pixels);

typedef struct ssn_display_window_instance_t
{
	HDC device_context;
	BITMAPINFOHEADER bitmap_header;

	BOOL running;
	DWORD error;
	MSG message;

	size_t bitmap_buffer_size;
	DWORD* bitmap_pixels;

	int image_width;
	int image_height;
	size_t image_buffer_size;
	DWORD* image_pixels;

	HANDLE heap;
	HWND window_handle;
} display_window_instance_t;

LRESULT CALLBACK ssn_display_window_procedure(HWND window, UINT message, WPARAM w_parameter, LPARAM l_parameter);

DWORD ssn_display_window(const char* window_title, int image_width, int image_height, DWORD* image_pixels);

#ifdef __cplusplus
}
#endif