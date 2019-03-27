#define _CRT_SECURE_NO_WARNINGS
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define COMMAND_SOLVE_KARNAUGH_MAP 1
#define COMMAND_SOLVE_TRUTH_TABLE_USING_KARNAUGH_MAP 2
#define COMMAND_SOLVE_JK_FLIP_FLOP_INPUTS 3
#define COMMAND_TRANSFORM_TRUTH_TABLE_TO_KARNAUGH_MAP 4
#define COMMAND_CLEAR 5
#define COMMAND_PRINT 6

size_t gray_code(size_t x);

size_t reverse_gray_code(size_t x);

size_t line_length(const char* string);

const char* next_line(const char* string);

bool is_line_empty(const char* string);

bool validate_state_string(bool no_ignored_states, const char* state_string, size_t* length);

bool read_truth_table_input(const char* input, char** variables, char** results);

bool solve_dff_in(const char* next_state, char** input);

bool solve_jkff_in(const char* current_state, const char* next_state, char** j_input, char** k_input);

bool make_all_truth_table_inputs_sequentially(size_t bits, char*** columns);

bool create_karnaugh_map(bool insert_variable_names, bool insert_new_lines, const char* variables, const char* result_states, char** map);

bool get_karnaugh_map_info(const char* map, char** row_variables, char** column_variable, char** map_data);

bool solve_karnaugh_map(const char* row_variables, const char* column_variables, const char* map_data, char** equation);

bool solve_truth_table_using_karnaugh_map(const char* variables, const char* results, char** equation);

bool load_text_file(const char* name, char** data);

bool save_text_file(const char* name, bool append, const char* data);

bool make_default_output_file_name(const char* input_file_name, char** ouput_file_name);

bool get_command(const char* command_line, unsigned int* command_type, const char** command_data, const char** next_command_line);

bool get_command_by_index(const char* commands, size_t index, unsigned int* command_type, const char** command_data, const char** next_command_line);

bool get_command_count(const char* commands, size_t* count);

int main(int argc, char** argv)
{
	/*
	if (argc < 2)
		return -1;
	char* input_file_name = argv[1];
	*/



	char* input_file_name = (char*)"C:\\Users\\Santtu Nyman\\Desktop\\nibba\\Debug\\lab2_0_1.txt";

	char* input;
	if (!load_text_file(input_file_name, &input))
		return -1;
	char* ouput_file_name;
	if (argc > 2)
		ouput_file_name = argv[2];
	else if (!make_default_output_file_name(input_file_name, &ouput_file_name))
	{
		free(input);
		return -1;
	}
	size_t command_count;
	if (!get_command_count(input, &command_count))
	{
		free(ouput_file_name);
		free(input);
		return -1;
	}
	for (size_t command_index = 0; command_index != command_count; ++command_index)
	{
		unsigned int command_type;
		const char* command_data;
		const char* next_command;
		if (!get_command_by_index(next_command, command_index, &command_type, &command_data, &next_command))
		{
			if (!(argc > 2))
				free(ouput_file_name);
			free(input);
			return -1;
		}
		if (command_type == COMMAND_SOLVE_KARNAUGH_MAP)
		{
			char* row_variables;
			char* column_variable;
			char* map_data;
			if (!get_karnaugh_map_info(command_data, &row_variables, &column_variable, &map_data))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* map_equation;
			if (!solve_karnaugh_map(row_variables, column_variable, map_data, &map_equation))
			{
				free(row_variables);
				free(column_variable);
				free(map_data);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			free(row_variables);
			free(column_variable);
			free(map_data);
			bool result = save_text_file(ouput_file_name, true, map_equation);
			free(map_equation);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_SOLVE_TRUTH_TABLE_USING_KARNAUGH_MAP)
		{
			char* variables;
			char* results;
			if (!read_truth_table_input(command_data, &variables, &results))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* table_equation;
			if (!solve_truth_table_using_karnaugh_map(variables, results, &table_equation))
			{
				free(variables);
				free(results);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			bool result = save_text_file(ouput_file_name, true, table_equation);
			free(variables);
			free(results);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_SOLVE_JK_FLIP_FLOP_INPUTS)
		{
			size_t current_state_count = 0;
			while (command_data[current_state_count] && command_data[current_state_count] != ' ' && command_data[current_state_count] != '\n')
				++current_state_count;
			if (command_data[current_state_count] != ' ' || !command_data[current_state_count + 1])
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			const char* next_states = command_data + current_state_count + 1;
			while (*next_states == ' ')
				++next_states;
			if (!*next_states)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			size_t next_states_count = 0;
			while (next_states[next_states_count] && next_states[next_states_count] != ' ' && next_states[next_states_count] != '\n')
				++next_states_count;
			if (next_states_count > current_state_count)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* buffer = (char*)malloc(2 * (current_state_count + 1));
			if (!buffer)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* buffer_current_states = buffer;
			memcpy(buffer_current_states, command_data, current_state_count);
			buffer_current_states[current_state_count] = 0;
			char* buffer_next_states = buffer + current_state_count + 1;
			memcpy(buffer_next_states, next_states, next_states_count);
			memset(buffer_next_states + next_states_count, 'x', current_state_count - next_states_count);
			buffer_next_states[current_state_count] = 0;
			char* j_inputs;
			char* k_inputs;
			if (!solve_jkff_in(buffer_current_states, buffer_next_states, &j_inputs, &k_inputs))
			{
				free(buffer);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* ouput_data = (char*)malloc(2 * current_state_count + 6);
			if (!ouput_data)
			{
				free(j_inputs);
				free(k_inputs);
				free(buffer);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			memcpy(ouput_data, "j=", 2);
			memcpy(ouput_data + 2, j_inputs, current_state_count);
			memcpy(ouput_data + 2 + current_state_count, " k=", 3);
			memcpy(ouput_data + 2 + current_state_count + 3, k_inputs, current_state_count);
			ouput_data[2 + current_state_count + 3 + current_state_count] = 0;
			bool result = save_text_file(ouput_file_name, true, ouput_data);
			free(ouput_data);
			free(j_inputs);
			free(k_inputs);
			free(buffer);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_TRANSFORM_TRUTH_TABLE_TO_KARNAUGH_MAP)
		{
			char* variables;
			char* results;
			if (!read_truth_table_input(command_data, &variables, &results))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* map;
			if (!create_karnaugh_map(true, true, variables, results, &map))
			{
				free(variables);
				free(results);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			free(variables);
			free(results);
			bool result = save_text_file(ouput_file_name, true, map);
			free(map);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_CLEAR)
		{
			FILE* file = fopen(ouput_file_name, "rb");
			if (file)
			{
				fclose(file);
				if (remove(ouput_file_name))
				{
					if (!(argc > 2))
						free(ouput_file_name);
					free(input);
					return -1;
				}
			}
		}
		else if (command_type == COMMAND_PRINT)
		{
			size_t print_line_length = 0;
			while (command_data[print_line_length] && command_data[print_line_length] != '$')
				++print_line_length;
			char* print = (char*)malloc(print_line_length + 1);
			if (!print)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			memcpy(print, command_data, print_line_length);
			print[print_line_length] = 0;
			bool result = save_text_file(ouput_file_name, true, print);
			free(print);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
	}

	/*
	for (const char* next_command = (const char*)input; next_command;)
	{
		unsigned int command_type;
		const char* command_data;
		if (!get_command(next_command, &command_type, &command_data, &next_command))
		{
			if (!(argc > 2))
				free(ouput_file_name);
			free(input);
			return -1;
		}
		if (command_type == COMMAND_SOLVE_KARNAUGH_MAP)
		{
			char* row_variables;
			char* column_variable;
			char* map_data;
			if (!get_karnaugh_map_info(command_data, &row_variables, &column_variable, &map_data))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* map_equation;
			if (!solve_karnaugh_map(row_variables, column_variable, map_data, &map_equation))
			{
				free(row_variables);
				free(column_variable);
				free(map_data);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			free(row_variables);
			free(column_variable);
			free(map_data);
			bool result = save_text_file(ouput_file_name, true, map_equation);
			free(map_equation);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_SOLVE_TRUTH_TABLE_USING_KARNAUGH_MAP)
		{
			char* variables;
			char* results;
			if (!read_truth_table_input(command_data, &variables, &results))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* table_equation;
			if (!solve_truth_table_using_karnaugh_map(variables, results, &table_equation))
			{
				free(variables);
				free(results);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			bool result = save_text_file(ouput_file_name, true, table_equation);
			free(variables);
			free(results);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_SOLVE_JK_FLIP_FLOP_INPUTS)
		{
			size_t current_state_count = 0;
			while (command_data[current_state_count] && command_data[current_state_count] != ' ' && command_data[current_state_count] != '\n')
				++current_state_count;
			if (command_data[current_state_count] != ' ' || !command_data[current_state_count + 1])
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			const char* next_states = command_data + current_state_count + 1;
			while (*next_states == ' ')
				++next_states;
			if (!*next_states)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			size_t next_states_count = 0;
			while (next_states[next_states_count] && next_states[next_states_count] != ' ' && next_states[next_states_count] != '\n')
				++next_states_count;
			if (next_states_count > current_state_count)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* buffer = (char*)malloc(2 * (current_state_count + 1));
			if (!buffer)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* buffer_current_states = buffer;
			memcpy(buffer_current_states, command_data, current_state_count);
			buffer_current_states[current_state_count] = 0;
			char* buffer_next_states = buffer + current_state_count + 1;
			memcpy(buffer_next_states, next_states, next_states_count);
			memset(buffer_next_states + next_states_count, 'x', current_state_count - next_states_count);
			buffer_next_states[current_state_count] = 0;
			char* j_inputs;
			char* k_inputs;
			if (!solve_jkff_in(buffer_current_states, buffer_next_states, &j_inputs, &k_inputs))
			{
				free(buffer);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* ouput_data = (char*)malloc(2 * current_state_count + 6);
			if (!ouput_data)
			{
				free(j_inputs);
				free(k_inputs);
				free(buffer);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			memcpy(ouput_data, "j=", 2);
			memcpy(ouput_data + 2, j_inputs, current_state_count);
			memcpy(ouput_data + 2 + current_state_count, " k=", 3);
			memcpy(ouput_data + 2 + current_state_count + 3, k_inputs, current_state_count);
			ouput_data[2 + current_state_count + 3 + current_state_count] = 0;
			bool result = save_text_file(ouput_file_name, true, ouput_data);
			free(ouput_data);
			free(j_inputs);
			free(k_inputs);
			free(buffer);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_TRANSFORM_TRUTH_TABLE_TO_KARNAUGH_MAP)
		{
			char* variables;
			char* results;
			if (!read_truth_table_input(command_data, &variables, &results))
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			char* map;
			if (!create_karnaugh_map(true, true, variables, results, &map))
			{
				free(variables);
				free(results);
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			free(variables);
			free(results);
			bool result = save_text_file(ouput_file_name, true, map);
			free(map);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
		else if (command_type == COMMAND_CLEAR)
		{
			FILE* file = fopen(ouput_file_name, "rb");
			if (file)
			{
				fclose(file);
				if (remove(ouput_file_name))
				{
					if (!(argc > 2))
						free(ouput_file_name);
					free(input);
					return -1;
				}
			}
		}
		else if (command_type == COMMAND_PRINT)
		{
			size_t print_line_length = 0;
			while (command_data[print_line_length] && command_data[print_line_length] != '$')
				++print_line_length;
			char* print = (char*)malloc(print_line_length + 1);
			if (!print)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
			memcpy(print, command_data, print_line_length);
			print[print_line_length] = 0;
			bool result = save_text_file(ouput_file_name, true, print);
			free(print);
			if (!result)
			{
				if (!(argc > 2))
					free(ouput_file_name);
				free(input);
				return -1;
			}
		}
	}
	*/
	if (!(argc > 2))
		free(ouput_file_name);
	free(input);
	return 0;
}

size_t gray_code(size_t x)
{
	return x ^ (x >> 1);
}

size_t reverse_gray_code(size_t x)
{
	for (size_t m = x, i = sizeof(size_t) * sizeof(char); i--;)
	{
		m >>= 1;
		x ^= m;
	}
	return x;
}

size_t line_length(const char* string)
{
	const char* i = string;
	while (*i && *i != '\n')
		++i;
	return (size_t)((uintptr_t)i - (uintptr_t)string);
}

const char* next_line(const char* string)
{
	const char* i = string;
	while (*i && *i != '\n')
		++i;
	return *i == '\n' && *(i + 1) != 0 ? i + 1 : 0;
}

bool is_line_empty(const char* string)
{
	while (*string && *string != '\n')
		if (*string++ != ' ')
			return false;
	return true;
}

bool validate_state_string(bool no_ignored_states, const char* state_string, size_t* length)
{
	const char* i = state_string;
	if (no_ignored_states)
		while (*i)
			if (*i == '0' || *i == '1')
				++i;
			else
				return false;
	else
		while (*i)
			if (*i == 'x' || *i == '0' || *i == '1')
				++i;
			else
				return false;
	if (length)
		*length = (size_t)((uintptr_t)i - (uintptr_t)state_string);
	return true;
}

bool read_truth_table_input(const char* input, char** variables, char** results)
{
	while (*input == ' ')
		++input;
	if (!*input)
		return false;
	size_t variable_count = 0;
	while (input[variable_count] && input[variable_count] != ' ' && input[variable_count] != '\n')
		++variable_count;
	if (input[variable_count] != ' ' || !input[variable_count + 1])
		return false;
	const char* input_results = input + variable_count + 1;
	while (*input_results == ' ')
		++input_results;
	if (!*input_results)
		return false;
	size_t input_result_count = 0;
	while (input_results[input_result_count] && input_results[input_result_count] != ' ' && input_results[input_result_count] != '\n')
		if (input_results[input_result_count] == 'x' || input_results[input_result_count] == '0' || input_results[input_result_count] == '1')
			++input_result_count;
		else
			return false;
	if (((size_t)1 << variable_count) < input_result_count)
		return false;
	char* variable_buffer = (char*)malloc(variable_count + 1);
	if (!variable_buffer)
		return false;
	memcpy(variable_buffer, input, variable_count);
	variable_buffer[variable_count] = 0;
	char* result_buffer = (char*)malloc(((size_t)1 << variable_count) + 1);
	if (!result_buffer)
	{
		free(variable_buffer);
		return false;
	}
	memcpy(result_buffer, input_results, input_result_count);
	memset(result_buffer + input_result_count, 'x', ((size_t)1 << variable_count) - input_result_count);
	result_buffer[((size_t)1 << variable_count)] = 0;
	*variables = variable_buffer;
	*results = result_buffer;
	return true;
}

bool solve_dff_in(const char* next_state, char** input)
{
	size_t next_state_count;
	if (!validate_state_string(false, next_state, &next_state_count))
		return false;
	char* i = (char*)malloc(next_state_count + 1);
	if (!i)
		return false;
	memcpy(i, next_state, next_state_count + 1);
	*input = i;
	return true;
}

bool solve_jkff_in(const char* current_state, const char* next_state, char** j_input, char** k_input)
{
	size_t current_state_count;
	size_t next_state_count;
	if (!validate_state_string(true, current_state, &current_state_count) || !validate_state_string(false, next_state, &next_state_count) || current_state_count != next_state_count)
		return false;
	char* j = (char*)malloc(next_state_count + 1);
	if (!j)
		return false;
	char* k = (char*)malloc(next_state_count + 1);
	if (!k)
	{
		free(j);
		return false;
	}
	for (size_t i = 0, s; i != next_state_count; ++i)
	{
		s = next_state[i] != 'x' ? (current_state[i] - '0') << 1 | (next_state[i] - '0') : 4;
		j[i] = "01xxx"[s];
		k[i] = "xx10x"[s];
	}
	j[next_state_count] = 0;
	k[next_state_count] = 0;
	*j_input = j;
	*k_input = k;
	return true;
}

bool make_all_truth_table_inputs_sequentially(size_t bits, char*** table_columns)
{
	char** columns = (char**)malloc(bits * (sizeof(char*) + (((size_t)1 << bits) + 1)));
	if (!columns)
		return false;
	for (size_t i = 0; i != bits; ++i)
		columns[i] = (char*)columns + ((bits * (sizeof(char*))) + (i * (((size_t)1 << bits) + 1)));
	for (size_t i = 0; i != bits; ++i)
	{
		for (size_t c = 0; c != ((size_t)1 << bits); ++c)
			columns[i][c] = '0' + (char)((c / ((size_t)1 << i)) & (size_t)1);
		columns[i][(size_t)1 << bits] = 0;
	}
	*table_columns = columns;
	return true;
}

bool create_karnaugh_map(bool insert_variable_names, bool insert_new_lines, const char* variables, const char* result_states, char** map)
{
	size_t result_state_count;
	size_t variable_count = strlen(variables);
	if ((insert_variable_names && !insert_new_lines) || !variable_count || !validate_state_string(false, result_states, &result_state_count) || ((size_t)1 << variable_count) != result_state_count)
		return false;
	size_t row_variable_count = ((variable_count >> 1) + (variable_count & 1));
	size_t column_variable_count = (variable_count >> 1);
	size_t map_width = (size_t)1 << row_variable_count;
	size_t map_heigth = (size_t)1 << column_variable_count;
	size_t row_length = (insert_variable_names ? column_variable_count : 0) + map_width + (insert_new_lines ? 1 : 0);
	char* new_map = (char*)malloc(((insert_variable_names ? 1 : 0) + map_heigth) * row_length + 1);
	if (!new_map)
		return false;
	if (insert_variable_names)
	{
		for (size_t x = 0; x != column_variable_count + (map_width >> 1) - (row_variable_count >> 1); ++x)
			new_map[x] = ' ';
		for (size_t x = 0; x != row_variable_count; ++x)
			new_map[column_variable_count + (map_width >> 1) - (row_variable_count >> 1) + x] = variables[x];
		for (size_t x = 0; x != map_width - ((map_width >> 1) - (row_variable_count >> 1)); ++x)
			new_map[column_variable_count + (map_width >> 1) - (row_variable_count >> 1) + row_variable_count + x] = ' ';
		if (insert_new_lines)
			new_map[row_length - 1] = '\n';
	}
	for (size_t y = 0; y != map_heigth; ++y)
	{
		if (insert_variable_names)
		{
			if (y == (map_heigth >> 1) - ((column_variable_count >> 1) + (column_variable_count & 1)))
				for (size_t x = 0; x != column_variable_count; ++x)
					new_map[(1 + y) * row_length + x] = variables[row_variable_count + x];
			else
				for (size_t x = 0; x != column_variable_count; ++x)
					new_map[(1 + y) * row_length + x] = ' ';
		}
		for (size_t x = 0; x != map_width; ++x)
			new_map[((insert_variable_names ? 1 : 0) + y) * row_length + (insert_variable_names ? column_variable_count : 0) + x] = result_states[gray_code(x) * map_heigth + gray_code(y)];
		if (insert_new_lines)
			new_map[((insert_variable_names ? 1 : 0) + y) * row_length + (insert_variable_names ? column_variable_count : 0) + map_width] = '\n';
	}
	new_map[((insert_variable_names ? 1 : 0) + map_heigth) * row_length] = 0;
	*map = new_map;
	return true;
}

typedef struct karnaugh_map_info_line
{
	const char* string;
	size_t length;
	size_t beginning;
	size_t ending;
} karnaugh_map_info_line;

bool get_karnaugh_map_info(const char* map, char** row_variables, char** column_variable, char** map_data)
{
	while (is_line_empty(map))
		map = next_line(map);
	if (!*map)
		return false;
	const char* read = map;
	size_t line_count = 0;
	while (read && !is_line_empty(read))
	{
		++line_count;
		read = next_line(read);
	}
	if (line_count < 2)
		return false;
	karnaugh_map_info_line* lines = (karnaugh_map_info_line*)malloc(line_count * sizeof(karnaugh_map_info_line));
	if (!lines)
		return false;
	read = map;
	for (size_t i = 0; i != line_count; ++i)
	{
		lines[i].string = read;
		lines[i].length = 0;
		lines[i].beginning = (size_t)~1;
		lines[i].ending = (size_t)~1;
		while (lines[i].string[lines[i].length] && lines[i].string[lines[i].length] != '\n')
		{
			if (lines[i].beginning == (size_t)~1 && lines[i].string[lines[i].length] != ' ')
				lines[i].beginning = lines[i].length;
			if (lines[i].string[lines[i].length] != ' ')
				lines[i].ending = lines[i].length + 1;
			++lines[i].length;
		}
		read = next_line(read);
	}
	if (lines[0].beginning == (size_t)~1)
	{
		free(lines);
		return false;
	}
	size_t row_variable_count = 0;
	read = lines[0].string + lines[0].beginning;
	for (const char* end = lines[0].string + lines[0].ending; read != end; ++read)
		if (*read != ' ' && *read != 'x' && *read != '0' && *read != '1')
			++row_variable_count;
	size_t column_variable_line = 1;
	for (size_t i = 2; i != line_count; ++i)
		if (lines[i].beginning < lines[column_variable_line].beginning)
			column_variable_line = i;
	size_t column_variable_count = 0;
	read = lines[column_variable_line].string;
	while (*read && *read != '\n')
	{
		if (*read != ' ' && *read != 'x' && *read != '0' && *read != '1')
			++column_variable_count;
		++read;
	}
	if (!row_variable_count && !column_variable_count)
	{
		free(lines);
		return false;
	}
	size_t data_top_left_y = (size_t)~1;
	size_t data_top_left_x = (size_t)~1;
	for (size_t i = 0; i != line_count && (data_top_left_y == (size_t)~1 && data_top_left_x == (size_t)~1); ++i)
	{
		read = lines[i].string;
		for (const char* end = read + lines[i].ending; read != end && (data_top_left_y == (size_t)~1 && data_top_left_x == (size_t)~1); ++read)
			if (*read == 'x' || *read == '0' || *read == '1')
			{
				data_top_left_y = i;
				data_top_left_x = (uintptr_t)read - (uintptr_t)lines[i].string;
			}
	}
	if (data_top_left_y == (size_t)~1 || data_top_left_x == (size_t)~1)
	{
		free(lines);
		return false;
	}
	size_t data_bottom_right_y = (size_t)~1;
	size_t data_bottom_right_x = (size_t)~1;
	for (size_t i = line_count; i-- && (data_bottom_right_y == (size_t)~1 && data_bottom_right_x == (size_t)~1);)
	{
		read = lines[i].string + lines[i].ending - 1;
		while (read != lines[i].string - 1 && (data_bottom_right_y == (size_t)~1 && data_bottom_right_x == (size_t)~1))
		{
			if (*read == 'x' || *read == '0' || *read == '1')
			{
				data_bottom_right_y = i;
				data_bottom_right_x = (uintptr_t)read - (uintptr_t)lines[i].string;
			}
			--read;
		}
	}
	if (data_bottom_right_y == (size_t)~1 || data_bottom_right_y == (size_t)~1)
	{
		free(lines);
		return false;
	}
	size_t map_height = data_bottom_right_y - data_top_left_y + 1;
	size_t map_width = data_bottom_right_x - data_top_left_x + 1;
	if ((1 << row_variable_count) != map_width || (1 << column_variable_count) != map_height)
	{
		free(lines);
		return false;
	}
	for (size_t i = data_top_left_y; i != data_bottom_right_y + 1; ++i)
	{
		if (data_top_left_x + map_width > lines[i].ending)
		{
			free(lines);
			return false;
		}
		read = lines[i].string + data_top_left_x;
		for (const char* end = read + map_width; read != end; ++read)
			if (*read != 'x' && *read != '0' && *read != '1')
			{
				free(lines);
				return false;
			}
	}
	char* row_variables_buffer = (char*)malloc(row_variable_count + 1);
	if (!row_variables_buffer)
	{
		free(lines);
		return false;
	}
	char* column_variable_buffer = (char*)malloc(column_variable_count + 1);
	if (!column_variable_buffer)
	{
		free(row_variables_buffer);
		free(lines);
		return false;
	}
	char* map_data_buffer = (char*)malloc(map_height * map_width + 1);
	if (!map_data_buffer)
	{
		free(column_variable_buffer);
		free(row_variables_buffer);
		free(lines);
		return false;
	}
	size_t row_variables_read = 0;
	read = lines[0].string + lines[0].beginning;
	for (const char* end = lines[0].string + lines[0].ending; read != end; ++read)
		if (*read != ' ' && *read != 'x' && *read != '0' && *read != '1')
			row_variables_buffer[row_variables_read++] = *read;
	row_variables_buffer[row_variables_read] = 0;
	size_t column_variables_read = 0;
	read = lines[column_variable_line].string;
	while (*read && *read != '\n')
	{
		if (*read != ' ' && *read != 'x' && *read != '0' && *read != '1')
			column_variable_buffer[column_variables_read++] = *read;
		++read;
	}
	column_variable_buffer[column_variables_read] = 0;
	if (row_variables_read != row_variable_count || column_variables_read != column_variable_count)
	{
		free(column_variable_buffer);
		free(row_variables_buffer);
		free(lines);
		return false;
	}
	for (size_t y = 0; y != map_height; ++y)
		for (size_t x = 0; x != map_width; ++x)
			map_data_buffer[y * map_height + x] = lines[data_top_left_y + y].string[data_top_left_x + x];
	map_data_buffer[map_height * map_width] = 0;
	*row_variables = row_variables_buffer;
	*column_variable = column_variable_buffer;
	*map_data = map_data_buffer;
	return true;
}

typedef struct karnaugh_map_cell
{
	bool ignored;
	bool value;
	bool selected;
} karnaugh_map_cell;

typedef struct karnaugh_map_rectangle
{
	size_t x;
	size_t y;
	size_t w;
	size_t h;
	size_t trues_selected;
} karnaugh_map_rectangle;

bool solve_karnaugh_map(const char* row_variables, const char* column_variables, const char* map_data, char** equation)
{
	size_t column_variable_count = strlen(column_variables);
	size_t row_variables_count = strlen(row_variables);
	size_t height = (size_t)1 << column_variable_count;
	size_t width = (size_t)1 << row_variables_count;
	if ((height == 1 && width == 1) || strlen(map_data) != height * width)
		return false;
	size_t map_true_count = 0;
	size_t map_false_count = 0;
	for (const char* i = map_data, *e = i + height * width; i != e; ++i)
		if (*i == '1')
			++map_true_count;
		else if (*i == '0')
			++map_false_count;
		else if (*i != 'x')
			return false;
	if (!map_true_count)
	{
		char* false_equation = (char*)malloc(2);
		false_equation[0] = '0';
		false_equation[1] = 0;
		*equation = false_equation;
		return true;
	}
	else if (!map_false_count)
	{
		char* true_equation = (char*)malloc(2);
		true_equation[0] = '1';
		true_equation[1] = 0;
		*equation = true_equation;
		return true;
	}
	karnaugh_map_rectangle* rectangles = (karnaugh_map_rectangle*)malloc(height * width * sizeof(karnaugh_map_rectangle));
	if (!rectangles)
		return false;
	memset(rectangles, 0, height * width * sizeof(karnaugh_map_rectangle));
	karnaugh_map_cell* map = (karnaugh_map_cell*)malloc(height * width * sizeof(karnaugh_map_cell));
	if (!map)
	{
		free(rectangles);
		return false;
	}
	for (size_t i = 0, e = height * width; i != e; ++i)
		if (map_data[i] == '0')
		{
			map[i].ignored = false;
			map[i].value = false;
			map[i].selected = false;
		}
		else if (map_data[i] == '1')
		{
			map[i].ignored = false;
			map[i].value = true;
			map[i].selected = false;
		}
		else
		{
			map[i].ignored = true;
			map[i].value = false;
			map[i].selected = false;
		}
	size_t rectangle_count = 0;
	for (size_t trues_selected = 0, h = height; h; h >>= 1)
		for (size_t w = width; w; w >>= 1)
			for (bool new_rectangle_added = true; new_rectangle_added;)
			{
				new_rectangle_added = false;
				karnaugh_map_rectangle new_rectangle = { 0, 0, 0, 0, 0 };
				for (bool new_rectangle_found = true; new_rectangle_found;)
				{
					new_rectangle_found = false;
					for (size_t logical_y = 0; logical_y != height / h * 2; ++logical_y)
					{
						size_t rectangle_y = logical_y * h / 2;
						for (size_t logical_x = 0; logical_x != width / w * 2; ++logical_x)
						{
							size_t rectangle_x = logical_x * w / 2;
							size_t rectangle_new_trues_count = 0;
							bool rectangle_contains_falses = false;
							for (size_t y = 0; y != h && !rectangle_contains_falses; ++y)
								for (size_t x = 0; x != w && !rectangle_contains_falses; ++x)
									if (!map[((rectangle_y + y) % height) * width + ((rectangle_x + x) % width)].ignored && !map[((rectangle_y + y) % height) * width + ((rectangle_x + x) % width)].selected)
										if (map[((rectangle_y + y) % height) * width + ((rectangle_x + x) % width)].value)
											++rectangle_new_trues_count;
										else
											rectangle_contains_falses = true;
							if (!rectangle_contains_falses && rectangle_new_trues_count > new_rectangle.trues_selected)
							{
								new_rectangle.x = rectangle_x;
								new_rectangle.y = rectangle_y;
								new_rectangle.w = w;
								new_rectangle.h = h;
								new_rectangle.trues_selected = rectangle_new_trues_count;
								new_rectangle_found = true;
							}
						}
					}
				}
				if (new_rectangle.h && new_rectangle.w)
				{
					rectangles[rectangle_count].x = new_rectangle.x;
					rectangles[rectangle_count].y = new_rectangle.y;
					rectangles[rectangle_count].w = new_rectangle.w;
					rectangles[rectangle_count].h = new_rectangle.h;
					rectangles[rectangle_count].trues_selected = new_rectangle.trues_selected;
					++rectangle_count;
					trues_selected += new_rectangle.trues_selected;
					if (trues_selected == map_true_count)
					{
						free(map);
						char* new_equation = (char*)malloc(rectangle_count * (((column_variable_count + row_variables_count) * 2) + 1));
						if (!new_equation)
						{
							free(rectangles);
							return false;
						}
						char* write_equation = new_equation;
						for (size_t i = 0; i != rectangle_count; ++i)
						{
							size_t gray_code_y = gray_code(rectangles[i].y % height);
							size_t column_variable_mask = 0;
							for (size_t y = 0; y != rectangles[i].h; ++y)
								column_variable_mask |= (gray_code_y ^ gray_code((rectangles[i].y + y) % height));
							column_variable_mask = ~column_variable_mask & (((size_t)1 << column_variable_count) - 1);
							for (size_t b = 0; column_variable_mask >> b; ++b)
								if ((column_variable_mask >> b) & 1)
								{
									if (!((gray_code_y >> b) & 1))
										*write_equation++ = '~';
									*write_equation++ = column_variables[column_variable_count - 1 - b];
								}
							size_t gray_code_x = gray_code(rectangles[i].x % width);
							size_t row_variable_mask = 0;
							for (size_t x = 0; x != rectangles[i].w; ++x)
								row_variable_mask |= (gray_code_x ^ gray_code((rectangles[i].x + x) % width));
							row_variable_mask = ~row_variable_mask & (((size_t)1 << row_variables_count) - 1);
							for (size_t b = 0; row_variable_mask >> b; ++b)
								if ((row_variable_mask >> b) & 1)
								{
									if (!((gray_code_x >> b) & 1))
										*write_equation++ = '~';
									*write_equation++ = row_variables[row_variables_count - 1 - b];
								}
							if (i + 1 != rectangle_count)
								*write_equation++ = '+';
							else
								*write_equation++ = 0;
						}
						free(rectangles);
						*equation = new_equation;
						return true;
					}
					for (size_t y = 0; y != new_rectangle.h; ++y)
						for (size_t x = 0; x != new_rectangle.w; ++x)
							map[((new_rectangle.y + y) % height) * width + ((new_rectangle.x + x) % width)].selected = true;
					new_rectangle_added = true;
				}
			}
	return false;
}

bool solve_truth_table_using_karnaugh_map(const char* variables, const char* results, char** equation)
{
	if (((size_t)1 << strlen(variables)) != strlen(results))
		return false;
	char* map;
	if (!create_karnaugh_map(true, true, variables, results, &map))
		return false;
	char* map_row_variables;
	char* map_column_variables;
	char* map_data;
	if (!get_karnaugh_map_info(map, &map_row_variables, &map_column_variables, &map_data))
	{
		free(map);
		return false;
	}
	bool result = solve_karnaugh_map(map_row_variables, map_column_variables, map_data, equation);
	free(map_row_variables);
	free(map_column_variables);
	free(map_data);
	return result;
}

bool load_text_file(const char* name, char** data)
{
	FILE* file = fopen(name, "rb");
	if (!file)
		return false;
	fseek(file, 0, SEEK_END);
	if (ferror(file))
	{
		fclose(file);
		return false;
	}
	long long_file_size = ftell(file);
	if (long_file_size == -1L || long_file_size > SIZE_MAX)
	{
		fclose(file);
		return false;
	}
	fseek(file, 0, SEEK_SET);
	if (ferror(file))
	{
		fclose(file);
		return false;
	}
	size_t file_size = (size_t)long_file_size;
	char* raw_file_data = (char*)malloc(file_size);
	if (!raw_file_data)
	{
		fclose(file);
		return false;
	}
	for (size_t file_loaded = 0; file_loaded != file_size;)
	{
		size_t read_result = fread(raw_file_data + file_loaded, 1, file_size - file_loaded, file);
		if (!read_result)
		{
			free(raw_file_data);
			fclose(file);
			return false;
		}
		file_loaded += read_result;
	}
	fclose(file);
	char* file_data = (char*)malloc(file_size + 1);
	if (!file_data)
	{
		free(raw_file_data);
		return false;
	}
	char* write = file_data;
	for (char* i = raw_file_data, *e = i + file_size; i != e; ++i)
		if (*i != '\r')
			*write++ = *i;
	*write = 0;
	free(raw_file_data);
	*data = file_data;
	return true;
}

bool save_text_file(const char* name, bool append, const char* data)
{
	char* temporal_output_file_name;
	char* temporal_output_file_delete_name;
	FILE* file = append ? 0 : fopen(name, "rb");
	if (!file)
		temporal_output_file_name = 0;
	else
	{
		abort();
		fclose(file);
		size_t name_length = strlen(name);
		temporal_output_file_name = (char*)malloc((name_length + 22) + (name_length + 18));
		if (!temporal_output_file_name)
			return false;
		temporal_output_file_delete_name = temporal_output_file_name + (name_length + 22);
		memcpy(temporal_output_file_name, name, name_length);
		time_t raw_time;
		time(&raw_time);
		strftime(temporal_output_file_name + name_length, 22, "-OVERWRITE-%H%M%S.tmp", localtime(&raw_time));
		memcpy(temporal_output_file_delete_name, temporal_output_file_name, name_length + 1);
		memcpy(temporal_output_file_delete_name + name_length + 1, "TRASH", 5);
		memcpy(temporal_output_file_delete_name + name_length + 1 + 5, temporal_output_file_name + name_length + 10, 12);
	}
	file = fopen(temporal_output_file_name ? temporal_output_file_name : name, append ? "ab" : "wb");
	if (!file)
	{
		if (temporal_output_file_name)
			free(temporal_output_file_name);
		return false;
	}
	for (size_t file_size = strlen(data), file_saved = 0; file_saved != file_size;)
	{
		size_t write_result = fwrite(data + file_saved, 1, file_size - file_saved, file);
		if (!write_result)
		{
			fclose(file);
			remove(temporal_output_file_name ? temporal_output_file_name : name);
			if (temporal_output_file_name)
				free(temporal_output_file_name);
			return false;
		}
		file_saved += write_result;
	}
	if (fflush(file))
	{
		fclose(file);
		remove(temporal_output_file_name ? temporal_output_file_name : name);
		if (temporal_output_file_name)
			free(temporal_output_file_name);
		return false;
	}
	fclose(file);
	if (temporal_output_file_name)
	{
		if (rename(name, temporal_output_file_delete_name))
		{
			remove(temporal_output_file_name);
			free(temporal_output_file_name);
			return false;
		}
		if (rename(temporal_output_file_name, name))
		{
			if (rename(temporal_output_file_delete_name, name))
				remove(temporal_output_file_delete_name);
			remove(temporal_output_file_name);
			free(temporal_output_file_name);
			return false;
		}
		remove(temporal_output_file_delete_name);
		free(temporal_output_file_name);
	}
	return true;
}

bool make_default_output_file_name(const char* input_file_name, char** ouput_file_name)
{
	size_t input_file_name_length = strlen(input_file_name);
	char* name = (char*)malloc(input_file_name_length + 29);
	if (!name)
		return false;
	time_t raw_time;
	time(&raw_time);
	memcpy(name, input_file_name, input_file_name_length);
	strftime(name + input_file_name_length, 29, "-out-%F-%H-%M-%S.txt", localtime(&raw_time));
	*ouput_file_name = name;
	return true;
}

bool get_command(const char* command_line, unsigned int* command_type, const char** command_data, const char** next_command_line)
{
	while (*command_line == ' ')
		++command_line;
	size_t command_line_length = line_length(command_line);
	unsigned int command = 0;
	if ((command_line_length > 26 && !memcmp(command_line, "$command_solve_karnaugh_map", 27)) && (command_line[27] == 0 || command_line[27] == ' ' || command_line[27] == '\n'))
		command = COMMAND_SOLVE_KARNAUGH_MAP;
	else if ((command_line_length > 44 && !memcmp(command_line, "$command_solve_truth_table_using_karnaugh_map", 45)) && (command_line[45] == 0 || command_line[45] == ' ' || command_line[45] == '\n'))
		command = COMMAND_SOLVE_TRUTH_TABLE_USING_KARNAUGH_MAP;
	else if ((command_line_length > 33 && !memcmp(command_line, "$command_solve_jk_flip_flop_inputs", 34)) && (command_line[34] == 0 || command_line[34] == ' ' || command_line[34] == '\n'))
		command = COMMAND_SOLVE_JK_FLIP_FLOP_INPUTS;
	else if ((command_line_length > 45 && !memcmp(command_line, "$command_transform_truth_table_to_karnaugh_map", 46)) && (command_line[46] == 0 || command_line[46] == ' ' || command_line[46] == '\n'))
		command = COMMAND_TRANSFORM_TRUTH_TABLE_TO_KARNAUGH_MAP;
	else if ((command_line_length > 13 && !memcmp(command_line, "$command_clear", 14)) && (command_line[14] == 0 || command_line[14] == ' ' || command_line[14] == '\n'))
		command = COMMAND_CLEAR;
	else if ((command_line_length > 13 && !memcmp(command_line, "$command_print", 14)) && (command_line[14] == 0 || command_line[14] == ' ' || command_line[14] == '\n'))
		command = COMMAND_PRINT;
	if (!command)
		return false;
	const char* data = next_line(command_line);
	if (!data || *data == '$')
		return false;
	const char* next = data;
	while (*next && *next != '$')
		++next;
	if (!*next)
		next = 0;
	*command_type = command;
	*command_data = data;
	*next_command_line = next;
	return true;
}

bool get_command_by_index(const char* commands, size_t index, unsigned int* command_type, const char** command_data, const char** next_command_line)
{


	const char* command_line;
	if (!get_command(commands, command_type, command_data, &command_line))
		return false;
	size_t i = 0;
	while (i != index && command_line)
		if (get_command(command_line, command_type, command_data, &command_line))
			++i;
		else
			return false;
	*next_command_line = command_line;
	return i == index;
}

bool get_command_count(const char* commands, size_t* count)
{
	unsigned int ignored_type;
	const char* ignored_data;
	size_t i = 0;
	const char* next_command_line = commands;
	while (next_command_line)
		if (get_command_by_index(next_command_line, i, &ignored_type, &ignored_data, &next_command_line))
			++i;
		else
			return false;
	*count = i;
	return true;
}