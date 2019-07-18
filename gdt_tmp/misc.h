#ifndef LDL_MISC_H
#define LDL_MISC_H

#include <stdlib.h>
#include <stdio.h>

const float tau = 6.28318530718f;

char* get_sum_of_products(const char* variables, const char* output_table)
{
	size_t input_variable_count = strlen(variables);
	size_t output_count = (size_t)1 << input_variable_count;
	size_t relevant_minterm_count = 0;
	size_t output_buffer_size = 0;
	for (size_t i = 0; i != output_count; ++i)
		if (output_table[i] == '1')
		{
			if (relevant_minterm_count)
				output_buffer_size += 1;
			for (size_t j = 0; j != input_variable_count; ++j)
				if ((i >> j) & 1)
					output_buffer_size += 1;
				else
					output_buffer_size += 2;
			++relevant_minterm_count;
		}
	output_buffer_size += 1;
	char* result = (char*)malloc(output_buffer_size * sizeof(char));
	if (!result)
		return 0;
	char* write_result = result;
	for (size_t c = 0, i = 0; c != relevant_minterm_count; ++i)
		if (output_table[i] == '1')
		{
			if (c)
				*write_result++ = '+';
			for (size_t j = 0; j != input_variable_count; ++j)
			{
				if (!((i >> j) & 1))
					*write_result++ = '~';
				*write_result++ = variables[j];
			}
			++c;
		}
	*write_result = 0;
	return result;
}

char* get_product_of_sums(const char* variables, const char* output_table)
{
	size_t input_variable_count = strlen(variables);
	size_t output_count = (size_t)1 << input_variable_count;
	size_t relevant_maxterm_count = 0;
	size_t output_buffer_size = 0;
	for (size_t i = 0; i != output_count; ++i)
		if (output_table[i] == '0')
		{
			output_buffer_size += 2 + (input_variable_count - 1);
			for (size_t j = 0; j != input_variable_count; ++j)
				if ((i >> j) & 1)
					output_buffer_size += 2;
				else
					output_buffer_size += 1;
			++relevant_maxterm_count;
		}
	output_buffer_size += 1;
	char* result = (char*)malloc(output_buffer_size * sizeof(char));
	if (!result)
		return 0;
	char* write_result = result;
	for (size_t c = 0, i = 0; c != relevant_maxterm_count; ++i)
		if (output_table[i] == '0')
		{
			*write_result++ = '(';
			for (size_t j = 0; j != input_variable_count; ++j)
			{
				if (j)
					*write_result++ = '+';
				if (!((i >> j) & 1))
					*write_result++ = '~';
				*write_result++ = variables[j];
			}
			*write_result++ = ')';
			++c;
		}
	*write_result = 0;
	return result;
}

#endif