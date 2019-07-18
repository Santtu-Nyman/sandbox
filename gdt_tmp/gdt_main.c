/*
	Graph Drawing Tool 0.1.0 2019-07-18 by Santtu Nyman.
	git repository https://github.com/Santtu-Nyman/gdt

	Description
		THIS PROGRAM IS INCOMPLETE!
		Command line tool for drawing graphs from data points to bmp files.
		The main purpose of the program is drawing graph from very large sample sizes
		that other programs (such as Excel) strugle to deal with.
		This program works on both Windows and Linux.
		Instructions how to use the program are contained in the program and printed out with -h or --help parameter.

	Version history
		version 0.0.0 2019-07-18
			Testing Linux version.
		version 0.0.0 2019-07-15
			First incomplete version.
*/

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gdt_file.h"
#include "gdt_make_bitmap.h"
#include "gdt_runtime_parameters.h"
#include "gdt_error.h"
#include "gdt_core.h"

size_t gdt_maximun_relevant_line_count(size_t file_size, const void* file_data);

float gdt_round_grid_number(float delta);

int gdt_find_reasonable_graph_grid_deltas(int soa, size_t element_count, size_t x_element_index, size_t point_count, const float* samples, float* x_delta, float* y_delta);

int gdt_decode_text_line_input_file(size_t file_size, const void* file_data, int vector_length, size_t* graph_point_count, float** graph_point_table);

int gdt_find_font_file(char** font_file_name);

int gdt_decode_axis_title_argument(const char* axis_title_argument, size_t* axis_title_count, char*** axis_title_table);

void gdt_exit_process(const char* error_string, int error_code);

void gdt_print_program_info();

int main(int argc, char** argv)
{
	const int default_image_width = 1920;
	const int default_image_height = 1080;
	const int default_file_format = 0;
	const char* input_file_format_table[2] = { "text_lines", "float" };

	int error;
	size_t argument_count;
	char** arguments;
	int input_file_format;
	int vector_length;
	char* input_file_name;
	char* output_file_name;
	char* font_file_name;
	char* graph_title;
	char* axis_title_argument;
	size_t axis_title_count;
	char** axis_title_table;
	float grid_x;
	float grid_y;
	int image_width;
	int image_height;
	size_t input_file_size;
	void* input_file_data;
	size_t graph_point_count;
	float* graph_point_table;
	size_t image_file_size;
	void* image_file_data;

	error = gdt_get_process_arguments(&argument_count, &arguments);
	if (error)
		gdt_exit_process("Unable to read process arguments", error);
	error = gdt_get_enumeration_argument(argument_count, arguments, "-f", sizeof(input_file_format_table) / sizeof(*input_file_format_table), input_file_format_table, &input_file_format);
	if (error)
	{
		if (error != ENOENT)
			gdt_exit_process("Invalid input file format specified", error);
		input_file_format = default_file_format;
	}
	if (argument_count == 1)
	{
		gdt_print_program_info();
		exit(EXIT_SUCCESS);
	}
	error = gdt_get_flag_argument(argument_count, arguments, "-h");
	if (error && error != ENOENT)
		gdt_exit_process("Error reading process arguments", error);
	if (!error)
	{
		gdt_print_program_info();
		exit(EXIT_SUCCESS);
	}
	error = gdt_get_flag_argument(argument_count, arguments, "--help");
	if (error && error != ENOENT)
		gdt_exit_process("Error reading process arguments", error);
	if (!error)
	{
		gdt_print_program_info();
		exit(EXIT_SUCCESS);
	}
	error = gdt_get_string_argument(argument_count, arguments, "-g", &graph_title);
	if (error)
	{
		if (error != ENOENT)
			gdt_exit_process("Unable to read graph title argument", error);
		graph_title = 0;
	}
	error = gdt_get_string_argument(argument_count, arguments, "-a", &axis_title_argument);
	if (error)
	{
		if (error != ENOENT)
			gdt_exit_process("Unable to read axis title argument", error);
		axis_title_argument = 0;
		axis_title_count = 0;
		axis_title_table = 0;
	}
	else
	{
		error = gdt_decode_axis_title_argument(axis_title_argument, &axis_title_count, &axis_title_table);
		if (error)
			gdt_exit_process("Unable to read axis title argument", error);
		if (axis_title_count > (0x7FFF / sizeof(float)))
			gdt_exit_process("Too many axis titles", 0);
	}
	error = gdt_get_little_integer_argument(argument_count, arguments, "-v", &vector_length);
	if (error && error != ENOENT)
		gdt_exit_process("Unable to read vector length argument", error);
	if (axis_title_count)
	{
		if (error)
			vector_length = (int)axis_title_count;
		else if (vector_length != (int)axis_title_count)
			gdt_exit_process("Vector length must be same as axis title count", 0);
	}
	else if (error)
		gdt_exit_process("Vector length was not specified", error);
	error = gdt_get_string_argument(argument_count, arguments, "-i", &input_file_name);
	if (error)
		gdt_exit_process("No input file name specified", error);
	error = gdt_get_string_argument(argument_count, arguments, "-o", &output_file_name);
	if (error)
		gdt_exit_process("No output file name specified", error);
	error = gdt_get_float_argument(argument_count, arguments, "-x", &grid_x);
	if (error || grid_x <= 0.0f)
	{
		if (error != ENOENT)
			gdt_exit_process("Invalid grid x spacing specified", error);
		grid_x = 0.0f;
	}
	error = gdt_get_float_argument(argument_count, arguments, "-y", &grid_y);
	if (error || grid_y <= 0.0f)
	{
		if (error != ENOENT)
			gdt_exit_process("Invalid grid y spacing specified", error);
		grid_y = 0.0f;
	}
	if ((grid_x != 0.0f && grid_y == 0.0f) || (grid_x == 0.0f && grid_y != 0.0f))
		gdt_exit_process("Both grid x and y spacing must be specified", error);
	error = gdt_get_little_integer_argument(argument_count, arguments, "--image_width", &image_width);
	if (error || image_width < 1)
	{
		if (error != ENOENT)
			gdt_exit_process("Invalid image width specified", error);
		image_width = default_image_width;
	}
	if (image_width > 0x4000)
		gdt_exit_process("Invalid image width too large", 0);
	error = gdt_get_little_integer_argument(argument_count, arguments, "--image_height", &image_height);
	if (error || image_height < 1)
	{
		if (error != ENOENT)
			gdt_exit_process("Invalid image height specified", error);
		image_height = default_image_height;
	}
	if (image_height > 0x4000)
		gdt_exit_process("Invalid image height too large", 0);
	error = gdt_read_map_file(input_file_name, &input_file_size, &input_file_data);
	if (error)
		gdt_exit_process("Unable to read input file", error);
	if (input_file_format == 1)
	{
		graph_point_count = input_file_size / ((size_t)vector_length * sizeof(float));
		graph_point_table = (float*)input_file_data;
	}
	else if (input_file_format == 0)
	{
		error = gdt_decode_text_line_input_file(input_file_size, input_file_data, vector_length, &graph_point_count, &graph_point_table);
		if (error)
			gdt_exit_process("Unable to decode input file's text content", error);
	}
	else
		gdt_exit_process("Input file format not supported", error);
	if (grid_x == 0.0f && grid_y == 0.0f)
	{
		error = gdt_find_reasonable_graph_grid_deltas(0, (size_t)vector_length, 0, graph_point_count, graph_point_table, &grid_x, &grid_y);
		if (error)
			gdt_exit_process("Unable to calculate reasonable grid geometry for input data", error);
	}
	uint32_t* bitmap = (uint32_t*)malloc((size_t)image_height * (size_t)image_width * sizeof(uint32_t));
	if (!bitmap)
		gdt_exit_process("Not enough memory for graph image", ENOMEM);
	if (graph_title || axis_title_count)
	{
		error = gdt_find_font_file(&font_file_name);
		if (error)
			gdt_exit_process("Unable to find a truetype font file", error);
		error = gdt_simplified_draw_graph_with_titles_to_bitmap(grid_x, grid_y, image_width, image_height, image_width * sizeof(uint32_t),
			bitmap, (size_t)vector_length, graph_point_count, graph_point_table, font_file_name, (const char*)graph_title, (const char**)axis_title_table, 0);
		free(font_file_name);
	}
	else
 		error = gdt_simplified_draw_graph_to_bitmap(grid_x, grid_y, image_width, image_height, image_width * sizeof(uint32_t),
			bitmap, (size_t)vector_length, graph_point_count, graph_point_table, 0);
	error = gdt_make_bitmap(image_width, image_height, image_width * sizeof(uint32_t), bitmap, &image_file_size, &image_file_data);
	if (error)
		gdt_exit_process("Unable to create output image file data", error);
	error = gdt_store_file(output_file_name, image_file_size, image_file_data);
	if (error)
		gdt_exit_process("Unable to write output file", error);
	gdt_exit_process(0, 0);
	return 0;
}

size_t gdt_maximun_relevant_line_count(size_t file_size, const void* file_data)
{
	size_t line_count = 0;
	int line_not_ended = 0;
	for (const char* read = (const char*)file_data, *read_end = (const char*)((uintptr_t)file_data + file_size); read != read_end; ++read)
	{
		char character = *read;
		if (character != '\r')
		{
			if (character == '\n')
			{
				if (line_not_ended)
					++line_count;
				line_not_ended = 0;
			}
			else
				line_not_ended = 1;
		}
	}
	if (line_not_ended)
		++line_count;
	return line_count;
}

float gdt_round_grid_number(float delta)
{
	if (delta > 0.0f)
	{
		if (delta < 100.0f)
		{
			int digit_selector = 1;
			while ((int)(delta * (float)digit_selector) < 10)
				digit_selector *= 10;
			int primary_digits = (((int)(delta * (float)digit_selector) / 5) * 5);
			return (float)primary_digits / (float)digit_selector;
		}
		else
		{
			int digit_selector = 1;
			while ((int)(delta / (float)digit_selector) > 99)
				digit_selector *= 10;
			int primary_digits = (((int)(delta / (float)digit_selector) / 5) * 5);
			return (float)primary_digits * (float)digit_selector;
		}
	}
	else
		return 1.0f;
}

int gdt_find_reasonable_graph_grid_deltas(int soa, size_t element_count, size_t x_element_index, size_t sample_count, const float* samples, float* x_delta, float* y_delta)
{
	if (element_count < 2 || !sample_count || x_element_index >= element_count ||
		(((element_count * sizeof(float)) / sizeof(float)) != element_count) ||
		(((sample_count * (element_count * sizeof(float))) / (element_count * sizeof(float))) != sample_count))
		return EINVAL;
	float x_min;
	float x_max;
	float y_min;
	float y_max;
	if (soa)
	{
		x_min = samples[sample_count * x_element_index];
		x_max = x_min;
		y_min = samples[sample_count * ((x_element_index + 1) % element_count)];
		y_max = y_min;
		for (size_t i = 0; i != element_count; ++i)
			if (i == x_element_index)
				for (size_t j = 0; j != sample_count; ++j)
				{
					float value = samples[sample_count * i + j];
					if (value < x_min)
						x_min = value;
					if (value > x_max)
						x_max = value;
				}
			else
				for (size_t j = 0; j != sample_count; ++j)
				{
					float value = samples[sample_count * i + j];
					if (value < y_min)
						y_min = value;
					if (value > y_max)
						y_max = value;
				}
	}
	else
	{
		x_min = samples[x_element_index];
		x_max = x_min;
		y_min = samples[(x_element_index + 1) % element_count];
		y_max = y_min;
		for (size_t i = 0; i != sample_count; ++i)
			for (size_t j = 0; j != element_count; ++j)
			{
				float value = samples[element_count * i + j];
				if (j == x_element_index)
				{
					if (value < x_min)
						x_min = value;
					if (value > x_max)
						x_max = value;
				}
				else
				{
					if (value < y_min)
						y_min = value;
					if (value > y_max)
						y_max = value;
				}
			}
	}
	float x_range = x_max - x_min;
	float y_range = y_max - y_min;
	*x_delta = gdt_round_grid_number(x_range / 6.0f);
	*y_delta = gdt_round_grid_number(y_range / 5.0f);
	return 0;
}

int gdt_decode_text_line_input_file(size_t file_size, const void* file_data, int vector_length, size_t* graph_point_count, float** graph_point_table)
{
	if (vector_length < 1 || vector_length > (0x7FFF / sizeof(float)))
		return EINVAL;
	size_t allocated_point_count = gdt_maximun_relevant_line_count(file_size, file_data);
	if (!allocated_point_count)
		return EBADMSG;
	if (((allocated_point_count * (size_t)vector_length) / (size_t)vector_length) != allocated_point_count)
		return EBADMSG;
	allocated_point_count *= (size_t)vector_length;
	if (((allocated_point_count * sizeof(float)) / sizeof(float)) != allocated_point_count)
		return ENOMEM;
	float* point_table = (float*)malloc(allocated_point_count * ((size_t)vector_length * sizeof(float)));
	if (!point_table)
		return ENOMEM;
	size_t point_count = 0;
	char previus_character = '\n';
	int element_index = 0;
	int currently_reading_number = 0;
	int number_is_negative;
	int number_high_decimal_count;
	int number_low_decimal_shift;
	uint32_t number_high_value;
	uint32_t number_low_value;
	uint32_t overflow_check;
	for (const char* read = (const char*)file_data, * read_end = (const char*)((uintptr_t)file_data + file_size); read != read_end; ++read)
	{
		char character = *read;
		if (character != '\r')
		{
			if (currently_reading_number)
			{
				if (character >= '0' && character <= '9')
				{
					if (number_low_decimal_shift == 1 && previus_character != '.')
					{
						if (number_high_decimal_count != 9)
						{
							overflow_check = (uint32_t)10 * number_high_value;
							if ((overflow_check / 10) != number_high_value)
							{
								free(point_table);
								return ERANGE;
							}
							number_high_value = overflow_check;
							overflow_check = number_high_value + (uint32_t)(character - '0');
							if (overflow_check < number_high_value)
							{
								free(point_table);
								return ERANGE;
							}
							number_high_value = overflow_check;
							++number_high_decimal_count;
						}
						else
						{
							free(point_table);
							return ERANGE;
						}
					}
					else if (number_low_decimal_shift != 1000000000)
					{
						number_low_value = ((uint32_t)10 * number_low_value) + (uint32_t)(character - '0');
						number_low_decimal_shift *= 10;
					}
				}
				else if (character == '.')
				{
					if (number_low_decimal_shift != 1 || previus_character == '.')
					{
						free(point_table);
						return EBADMSG;
					}
				}
				else
				{
					if (point_count == allocated_point_count)
					{
						free(point_table);
						return EBADMSG;
					}
					point_table[point_count++] = number_is_negative ? -((float)number_high_value + ((float)number_low_value / (float)number_low_decimal_shift)) : ((float)number_high_value + ((float)number_low_value / (float)number_low_decimal_shift));
					++element_index;
					currently_reading_number = 0;
					if (character == '\n')
					{
						if (element_index != vector_length)
						{
							free(point_table);
							return EBADMSG;
						}
						element_index = 0;
					}
				}
			}
			else
			{
				if (character == '.')
				{
					if (element_index == vector_length)
					{
						free(point_table);
						return EBADMSG;
					}
					currently_reading_number = 1;
					number_is_negative = 0;
					number_high_decimal_count = 0;
					number_low_decimal_shift = 1;
					number_high_value = 0;
					number_low_value = 0;
				}
				else if (character == '-')
				{
					if (element_index == vector_length)
					{
						free(point_table);
						return EBADMSG;
					}
					currently_reading_number = 1;
					number_is_negative = 1;
					number_high_decimal_count = 0;
					number_low_decimal_shift = 1;
					number_high_value = 0;
					number_low_value = 0;
				}
				else if (character >= '0' && character <= '9')
				{
					if (element_index == vector_length)
					{
						free(point_table);
						return EBADMSG;
					}
					currently_reading_number = 1;
					number_is_negative = 0;
					number_high_decimal_count = 1;
					number_low_decimal_shift = 1;
					number_high_value = (uint32_t)(character - '0');
					number_low_value = 0;
				}
				else if (character == '\n')
				{
					if (element_index && element_index != vector_length)
					{
						free(point_table);
						return EBADMSG;
					}
					element_index = 0;
				}
			}
			previus_character = character;
		}
	}
	if (currently_reading_number)
	{
		if (point_count == allocated_point_count)
		{
			free(point_table);
			return EBADMSG;
		}
		point_table[point_count++] = number_is_negative ? -((float)number_high_value + ((float)number_low_value / (float)number_low_decimal_shift)) : ((float)number_high_value + ((float)number_low_value / (float)number_low_decimal_shift));
		++element_index;
		if (element_index != vector_length)
		{
			free(point_table);
			return EBADMSG;
		}
	}
	if (point_count % (size_t)vector_length)
	{
		free(point_table);
		return EBADMSG;
	}
	*graph_point_count = point_count / (size_t)vector_length;
	*graph_point_table = point_table;
	return 0;
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int gdt_find_font_file(char** font_file_name)
{
	const int csidl_fonts = 0x0014;
	char* buffer = (char*)malloc((MAX_PATH + 1) * sizeof(char));
	if (!buffer)
		return ENOMEM;
	HMODULE Shell32 = LoadLibraryA("Shell32.dll");
	HRESULT (WINAPI* Shell32_SHGetFolderPathA)(HWND hwnd, int csidl, HANDLE token, DWORD flags, char* path) = (HRESULT (WINAPI*)(HWND, int, HANDLE, DWORD, char*))GetProcAddress(Shell32, "SHGetFolderPathA");
	if (Shell32_SHGetFolderPathA(0, csidl_fonts, 0, 0, buffer))
	{
		int windows_directory_length = (int)GetWindowsDirectoryA(buffer, MAX_PATH + 1);
		if (!windows_directory_length)
		{
			FreeLibrary(Shell32);
			free(buffer);
			return ENOSYS;
		}
		else if (windows_directory_length == MAX_PATH + 1)
		{
			FreeLibrary(Shell32);
			free(buffer);
			return ENAMETOOLONG;
		}
		if (windows_directory_length + 6 > MAX_PATH + 1)
		{
			FreeLibrary(Shell32);
			free(buffer);
			return ENAMETOOLONG;
		}
		memcpy(buffer + windows_directory_length, "\\Fonts", 6 * sizeof(char));
	}
	FreeLibrary(Shell32);
	size_t directory_name_length = strlen(buffer);
	static const char* default_font_files[] = { "arial.ttf", "tahoma.ttf", "calibri.ttf", "consola.ttf" };
	for (size_t i = 0; i != (sizeof(default_font_files) / sizeof(char*)); ++i)
	{
		size_t font_file_name_length = strlen(default_font_files[i]);
		if (directory_name_length + 1 + font_file_name_length + 1 <= MAX_PATH + 1)
		{
			buffer[directory_name_length] = '\\';
			memcpy(buffer + directory_name_length + 1, default_font_files[i], (font_file_name_length + 1) * sizeof(char));
			DWORD default_font_file_atributes = GetFileAttributesA(buffer);
			if (default_font_file_atributes != INVALID_FILE_ATTRIBUTES && !(default_font_file_atributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				*font_file_name = buffer;
				return 0;
			}
		}
	}
	free(buffer);
	return ENOSYS;
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int gdt_find_font_file(char** font_file_name)
{
	const char* file_name_table[] = {
		"/usr/share/fonts/truetype/freefont/FreeSans.ttf",
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
		"/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf" };
	struct stat file_info;
	for (size_t i = 0; i != (sizeof(file_name_table) / sizeof(char*)); ++i)
		if (!stat(file_name_table[i], &file_info) && (S_ISREG(file_info.st_mode) || S_ISLNK(file_info.st_mode)))
		{
			size_t file_name_size = (strlen(file_name_table[i]) + 1) * sizeof(char);
			char* file_name = (char*)malloc(file_name_size);
			if (!file_name)
				return ENOMEM;
			memcpy(file_name, file_name_table[i], file_name_size);
			*font_file_name = file_name;
			return 0;
		}
	return ENOENT;
}

#endif

int gdt_decode_axis_title_argument(const char* axis_title_argument, size_t* axis_title_count, char*** axis_title_table)
{
	size_t axis_title_argument_length = strlen(axis_title_argument);
	const char* axis_title_argument_end = axis_title_argument + axis_title_argument_length;
	size_t count = 1;
	for (const char* i = axis_title_argument; i != axis_title_argument_end; ++i)
		if (*i == ',')
			++count;
	char** table = (char**)malloc((count * sizeof(char*)) + ((axis_title_argument_length - (count - 1) + count) * sizeof(char)));
	if (!table)
		return ENOMEM;
	char* name_writer = (char*)((uintptr_t)table + (count * sizeof(char*)));
	for (size_t i = 0; i != count; ++i)
	{
		table[i] = name_writer;
		while (*axis_title_argument && *axis_title_argument != ',')
			*name_writer++ = *axis_title_argument++;
		*name_writer++ = 0;
		axis_title_argument++;
	}
	*axis_title_count = count;
	*axis_title_table = table;
	return 0;
}

void gdt_exit_process(const char* error_string, int error_code)
{
	if (error_string)
	{
		const char* error_name;
		if (gdt_get_error_info(error_code, &error_name, 0))
			error_name = "Unknown error";
		printf("Error: %s.\nError code: %s.\nProcess failed.\n", error_string, error_name);
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("Process successful.\n");
		exit(EXIT_SUCCESS);
	}
}

void gdt_print_program_info()
{
	printf(
		"Program description:\n"
		"	Command line tool for drawing graphs from data points to bmp files.\n"
		"	The main purpose of the program is drawing graph from very large sample sizes that other programs(such as Excel) strugle to deal with.\n"
		"Parameter List:\n"
		"	-h or --help displays help message.\n"
		"	-f Specifies input data format. This parameter can be \"text_lines\" for text data where single lines of text contains single data point or \"float\" for raw 32 bit float data where consecutive elements make single data point. If this argument is not given, input data format is set to \"text_lines\".\n"
		"	-g Specifies title of the graph. This parameter is optional.\n"
		"	-a Specifies titles for the axis of the graph. The titles are separated with commas. This parameter is optional.\n"
		"	-v Specifies number of elements per data point. If axis titles are specified this this parameter is optional. Elements per data point must be same as number of axis titles.\n"
		"	-i Specifies name of input file containing data points for the graph.\n"
		"	-o Specifies name of output file where the graph image will be written.\n"
		"	-x Specifies approximate horizontal distance between graph grid lines. This parameter is optional.\n"
		"	-y Specifies approximate vertical distance between graph grid lines. This parameter is optional.\n"
		"	--image_width Specifies width of the graph image. This parameter is optional.\n"
		"	--image_height Specifies height of the graph image. This parameter is optional.\n");
};
