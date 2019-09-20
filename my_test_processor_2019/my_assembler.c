#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSEBLY_COMMAND_TYPE_NONE 0
#define ASSEBLY_COMMAND_TYPE_SET_CURSOR 1
#define ASSEBLY_COMMAND_TYPE_WRITE_RAW_DATA 2
#define ASSEBLY_COMMAND_TYPE_WRITE_INSTRUCTION 3
#define ASSEBLY_COMMAND_TYPE_ERROR 0xFF
#define ACCUMULATOR_REGISTER_INDEX 0
#define GENERAL_PURPOSE_REGISTER_INDEX 1
#define STACK_POINTER_REGISTER_INDEX 2
#define INSTRUCTION_POINTER_REGISTER_INDEX 3

typedef struct assembly_instruction_t
{
	const char* name;
	const char* intermediate;
	const char* left_register;
	const char* rigth_register;
	uint8_t machine_code_number;
} assembly_instruction_t;

typedef struct assembly_command_set_cursor_t
{
	uint8_t position;
} assembly_command_set_cursor_t;

typedef struct assembly_command_write_raw_data_t
{
	uint8_t data;
} assembly_command_write_raw_data_t;

typedef struct assembly_command_write_instruction_t
{
	assembly_instruction_t* instruction;
	uint8_t intermediate;
	uint8_t left_register;
	uint8_t right_register;
} assembly_command_write_instruction_t;

typedef union assembly_command_union_t
{
	assembly_command_set_cursor_t set_cursor;
	assembly_command_write_raw_data_t write_raw_data;
	assembly_command_write_instruction_t write_instruction;
} assembly_command_union_t;

typedef struct assembly_command_t
{
	uint8_t type;
	assembly_command_union_t command;
} assembly_command_t;

static const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static const assembly_instruction_t instruction_table[16] = {
	{ "MOVE", 0, "source", "destination", 0 },
	{ "CMP", 0, "left operand", "right operand", 1 },
	{ "OR", 0, "left operand", "right operand", 2 },
	{ "CJMP", "distance", 0, 0, 3 },
	{ "AND", 0, "left operand", "right operand", 4 },
	{ "LD", 0, "source address", "load value destination", 5 },
	{ "XOR", 0, "left operand", "right operand", 6 },
	{ "ST", 0, "destination address", "store value source", 7 },
	{ "LDIL", "low nibble constant", 0, 0, 8 },
	{ "LDIH", "high nibble constant", 0, 0, 9 },
	{ "SL", 0, "source", "destination", 10 },
	{ "SR", 0, "source", "destination", 11 },
	{ "INC", 0, "source", "destination", 12 },
	{ "DEC", 0, "source", "destination", 13 },
	{ "ADD", 0, "left operand", "right operand", 14 },
	{ "SUB", 0, "left operand", "right operand", 15 } };

size_t get_white_space_length(int this_line, const char* text, size_t length);

size_t get_printable_space_length(const char* text, size_t length);

int is_valid_character(char character);

size_t count_valid_characters(const char* text, size_t length);

size_t get_new_line_legth(const char* text, size_t length);

size_t get_line_length(const char* text, size_t length);

size_t get_line_count(const char* text, size_t length);

int printable_text_match(const char* text, size_t length, const char* match_text, size_t match_length);

size_t read_8bit_integer(const char* text, size_t length, uint8_t* value);

size_t decode_assembly_line(const char* assembly_line, size_t length, assembly_command_t* command);

int load_file(const char* file_name, size_t* file_size, void** file_data);

int store_file(const char* file_name, size_t file_size, const void* file_data);

int validate_assembly(const char* assembly, size_t length, size_t* command_count);

int write_machine_code(uint8_t* code_buffer, uint8_t write_offset, const char* assembly, size_t assembly_length, uint8_t* used_size);

int main(int argc, char** argv)
{
	uint8_t machine_code_buffer[0x100];
	char machine_hex_code_buffer[0x200];
	size_t assembly_file_size;
	char* assembly_file_data;
	uint8_t machine_code_used;

	if (argc < 3)
	{
		printf("No file names specified\n");
		return 0;
	}

	int no_error = load_file(argv[1], &assembly_file_size, (void**)&assembly_file_data);
	if (!no_error)
	{
		printf("Unable to read assembly file\n");
		return 0;
	}

	memset(machine_code_buffer, 0, 0x100);
	no_error = write_machine_code(machine_code_buffer, 0, assembly_file_data, assembly_file_size, &machine_code_used);
	free(assembly_file_data);
	if (!no_error)
	{
		printf("Invalid assembly\n");
		return 0;
	}

	for (size_t byte_index = 0; byte_index != machine_code_used; ++byte_index)
	{
		machine_hex_code_buffer[2 * byte_index] = hex_table[machine_code_buffer[byte_index] >> 4];
		machine_hex_code_buffer[2 * byte_index + 1] = hex_table[machine_code_buffer[byte_index] & 0xF];
	}

	no_error = store_file(argv[2], 2 * machine_code_used, machine_hex_code_buffer);
	if (!no_error)
	{
		printf("Unable to write hex file\n");
		return 0;
	}

	return 0;
}

size_t get_white_space_length(int this_line, const char* text, size_t length)
{
	size_t text_index = 0;
	if (this_line)
		while ((text_index != length) && (text[text_index] == ' ') || (text[text_index] == '\t'))
			++text_index;
	else
		while ((text_index != length) && (text[text_index] == ' ') || (text[text_index] == '\t') || (text[text_index] == '\n') || (text[text_index] == '\r'))
			++text_index;
	return text_index;
}

size_t get_printable_space_length(const char* text, size_t length)
{
	size_t text_index = 0;
	while (text_index != length && text[text_index] > 0x20 && text[text_index] < 0x7F)
		++text_index;
	return text_index;
}

int is_valid_character(char character)
{
	return ((character > 0x20) && (character < 0x7F)) || (character == ' ') || (character == '\t') || (character == '\n') || (character == '\r');
}

size_t count_valid_characters(const char* text, size_t length)
{
	size_t text_index = 0;
	while (is_valid_character(text[text_index]))
		++text_index;
	return text_index;
}

size_t get_new_line_legth(const char* text, size_t length)
{
	if (length)
	{
		if (length > 1)
		{
			if (text[0] == '\n')
				return 1 + (int)(text[1] == '\r');
			else if (text[0] == '\r')
				return 1 + (int)(text[1] == '\n');
			else
				return 0;
		}
		else
			return (text[0] == '\n') || (text[0] == '\r');
	}
	return 0;
}

size_t get_line_length(const char* text, size_t length)
{
	size_t text_index = 0;
	while (text_index != length && is_valid_character(text[text_index]) && text[text_index] != '\n' && text[text_index] != '\r')
		++text_index;
	return text_index;
}

size_t get_line_count(const char* text, size_t length)
{
	length = count_valid_characters(text, length);
	size_t line_count = 0;
	size_t read_offset = 0;
	while (read_offset != length)
	{
		++line_count;
		size_t line_length = get_line_length(text + read_offset, length - read_offset);
		read_offset += line_length;
		size_t new_line_length = get_new_line_legth(text + read_offset, length - read_offset);
		read_offset += new_line_length;
	}
	return line_count;
}

int printable_text_match(const char* text, size_t length, const char* match_text, size_t match_length)
{
	if (get_printable_space_length(text, length) == match_length)
	{
		for (size_t compare_index = 0; compare_index != match_length; ++compare_index)
			if (text[compare_index] != match_text[compare_index])
				return 0;
		return 1;
	}
	else
		return 0;
}

size_t read_8bit_integer(const char* text, size_t length, uint8_t* value)
{
	length = get_printable_space_length(text, length);
	if (!length)
		return 0;
	uint8_t temporal_value = 0;
	uint8_t new_temporal_value;
	if (text[length - 1] == 'B')
	{
		--length;
		if (!length)
			return 0;
		for (size_t digit = 0; digit != length; ++digit)
		{
			if (text[digit] >= '0' && text[digit] <= '1')
				new_temporal_value = (temporal_value << 1) | (uint8_t)(text[digit] - '0');
			else
				return 0;
			if (new_temporal_value < temporal_value)
				return 0;
			temporal_value = new_temporal_value;
		}
		*value = temporal_value;
		return 1 + length;
	}
	else if (text[length - 1] == 'H')
	{
		--length;
		if (!length)
			return 0;
		for (size_t digit = 0; digit != length; ++digit)
		{
			if (text[digit] >= '0' && text[digit] <= '9')
				new_temporal_value = (temporal_value << 4) | (uint8_t)(text[digit] - '0');
			else if (text[digit] >= 'A' && text[digit] <= 'F')
				new_temporal_value = (temporal_value << 4) | (10 + (uint8_t)(text[digit] - 'A'));
			else if (text[digit] >= 'a' && text[digit] <= 'f')
				new_temporal_value = (temporal_value << 4) | (10 + (uint8_t)(text[digit] - 'a'));
			else
				return 0;
			if (new_temporal_value < temporal_value)
				return 0;
			temporal_value = new_temporal_value;
		}
		*value = temporal_value;
		return 1 + length;
	}
	else if (text[length - 1] == 'D')
	{
		--length;
		if (!length)
			return 0;
		for (size_t digit = 0; digit != length; ++digit)
		{
			if (text[digit] >= '0' && text[digit] <= '9')
				new_temporal_value = (temporal_value * 10) + (uint8_t)(text[digit] - '0');
			else
				return 0;
			if (new_temporal_value < temporal_value)
				return 0;
			temporal_value = new_temporal_value;
		}
		*value = temporal_value;
		return 1 + length;
	}
	else if (text[length - 1] >= '0' && text[length - 1] <= '9')
	{
		for (size_t digit = 0; digit != length; ++digit)
		{
			if (text[digit] >= '0' && text[digit] <= '9')
				new_temporal_value = (temporal_value * 10) + (uint8_t)(text[digit] - '0');
			else
				return 0;
			if (new_temporal_value < temporal_value)
				return 0;
			temporal_value = new_temporal_value;
		}
		*value = temporal_value;
		return length;
	}
	return 0;
}

size_t decode_assembly_line(const char* assembly_line, size_t length, assembly_command_t* command)
{
	command->type = ASSEBLY_COMMAND_TYPE_ERROR;
	size_t line_beginning_skip = get_white_space_length(1, assembly_line, length);
	size_t first_white_space_length;
	size_t second_printable_length;
	size_t second_white_space_length;
	length -= line_beginning_skip;
	assembly_line += line_beginning_skip;
	length = get_line_length(assembly_line, length);
	if (!length || ((length > 1) && (assembly_line[0] == '-') && (assembly_line[1] == '-')))
	{
		command->type = ASSEBLY_COMMAND_TYPE_NONE;
		return line_beginning_skip + length;
	}
	else if (printable_text_match(assembly_line, length, "@", 1))
	{
		first_white_space_length = get_white_space_length(1, assembly_line + 1, length - 1);
		if (!first_white_space_length)
			return 0;
		second_printable_length = read_8bit_integer(assembly_line + 1 + first_white_space_length, length - 1 - first_white_space_length, &command->command.set_cursor.position);
		if (!second_printable_length)
			return 0;
		command->type = ASSEBLY_COMMAND_TYPE_SET_CURSOR;
		return line_beginning_skip + length;
	}
	else if (printable_text_match(assembly_line, length, "$", 1))
	{
		first_white_space_length = get_white_space_length(1, assembly_line + 1, length - 1);
		if (!first_white_space_length)
			return 0;
		second_printable_length = read_8bit_integer(assembly_line + 1 + first_white_space_length, length - 1 - first_white_space_length, &command->command.write_raw_data.data);
		if (!second_printable_length)
			return 0;
		command->type = ASSEBLY_COMMAND_TYPE_WRITE_RAW_DATA;
		return line_beginning_skip + length;
	}
	else
	{
		for (size_t table_index = 0; table_index != 16; ++table_index)
		{
			size_t instruction_name_length = strlen(instruction_table[table_index].name);
			if (printable_text_match(assembly_line, length, instruction_table[table_index].name, instruction_name_length))
				if (instruction_table[table_index].intermediate)
				{
					first_white_space_length = get_white_space_length(1, assembly_line + instruction_name_length, length - instruction_name_length);
					if (!first_white_space_length)
						return 0;
					second_printable_length = read_8bit_integer(assembly_line + instruction_name_length + first_white_space_length, length - instruction_name_length - first_white_space_length, &command->command.write_instruction.intermediate);
					if (!second_printable_length)
						return 0;
					command->command.write_instruction.instruction = (assembly_instruction_t*)(instruction_table + table_index);
					command->command.write_instruction.left_register = 0xFF;
					command->command.write_instruction.right_register = 0xFF;
					command->type = ASSEBLY_COMMAND_TYPE_WRITE_INSTRUCTION;
					return line_beginning_skip + length;
				}
				else
				{
					first_white_space_length = get_white_space_length(1, assembly_line + instruction_name_length, length - instruction_name_length);
					if (!first_white_space_length)
						return 0;
					if (!printable_text_match(assembly_line + instruction_name_length + first_white_space_length, length - instruction_name_length - first_white_space_length, "A", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length, length - instruction_name_length - first_white_space_length, "G", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length, length - instruction_name_length - first_white_space_length, "S", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length, length - instruction_name_length - first_white_space_length, "I", 1))
						return 0;
					second_white_space_length = get_white_space_length(1, assembly_line + instruction_name_length + first_white_space_length + 1, length - instruction_name_length - first_white_space_length - 1);
					if (!second_white_space_length)
						return 0;
					if (!printable_text_match(assembly_line + instruction_name_length + first_white_space_length + 1 + second_white_space_length, length - instruction_name_length - first_white_space_length - 1 - second_white_space_length, "A", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length + 1 + second_white_space_length, length - instruction_name_length - first_white_space_length - 1 - second_white_space_length, "G", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length + 1 + second_white_space_length, length - instruction_name_length - first_white_space_length - 1 - second_white_space_length, "S", 1) &&
						!printable_text_match(assembly_line + instruction_name_length + first_white_space_length + 1 + second_white_space_length, length - instruction_name_length - first_white_space_length - 1 - second_white_space_length, "I", 1))
						return 0;
					if (assembly_line[instruction_name_length + first_white_space_length] == 'A')
						command->command.write_instruction.left_register = ACCUMULATOR_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length] == 'G')
						command->command.write_instruction.left_register = GENERAL_PURPOSE_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length] == 'S')
						command->command.write_instruction.left_register = STACK_POINTER_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length] == 'I')
						command->command.write_instruction.left_register = INSTRUCTION_POINTER_REGISTER_INDEX;
					else
						return 0;
					if (assembly_line[instruction_name_length + first_white_space_length + 1 + second_white_space_length] == 'A')
						command->command.write_instruction.right_register = ACCUMULATOR_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length + 1 + second_white_space_length] == 'G')
						command->command.write_instruction.right_register = GENERAL_PURPOSE_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length + 1 + second_white_space_length] == 'S')
						command->command.write_instruction.right_register = STACK_POINTER_REGISTER_INDEX;
					else if (assembly_line[instruction_name_length + first_white_space_length + 1 + second_white_space_length] == 'I')
						command->command.write_instruction.right_register = INSTRUCTION_POINTER_REGISTER_INDEX;
					else
						return 0;
					command->command.write_instruction.instruction = (assembly_instruction_t*)(instruction_table + table_index);
					command->command.write_instruction.intermediate = 0xFF;
					command->type = ASSEBLY_COMMAND_TYPE_WRITE_INSTRUCTION;
					return line_beginning_skip + length;
				}
		}
		return 0;
	}
}

int load_file(const char* file_name, size_t* file_size, void** file_data)
{
	FILE* handle = fopen(file_name, "rb");
	if (!handle)
		return 0;
	if (fseek(handle, 0, SEEK_END))
	{
		fclose(handle);
		return 0;
	}
	size_t size = (size_t)ftell(handle);
	if (size == (size_t)-1)
	{
		fclose(handle);
		return 0;
	}
	if (fseek(handle, 0, SEEK_SET))
	{
		fclose(handle);
		return 0;
	}
	uintptr_t data = (uintptr_t)malloc(size);
	if (!data)
	{
		fclose(handle);
		return 0;
	}
	for (size_t loaded = 0; loaded != size;)
	{
		size_t io_result = fread((void*)(data + loaded), 1, size - loaded, handle);
		if (io_result)
			loaded += io_result;
		else
		{
			free((void*)data);
			fclose(handle);
			return 0;
		}
	}
	fclose(handle);
	*file_size = size;
	*file_data = (void*)data;
	return 1;
}

int store_file(const char* file_name, size_t file_size, const void* file_data)
{
	FILE* handle = fopen(file_name, "wb");
	if (!handle)
		return 0;
	for (size_t stored = 0; stored != file_size;)
	{
		size_t io_result = fwrite((const void*)((uintptr_t)file_data + stored), 1, file_size - stored, handle);
		if (io_result)
			stored += io_result;
		else
		{
			fclose(handle);
			return 0;
		}
	}
	fclose(handle);
	return 1;
}

int validate_assembly(const char* assembly, size_t length, size_t* command_count)
{
	length = count_valid_characters(assembly, length);
	assembly_command_t command;
	size_t count = 0;
	size_t read_offset = 0;
	while (read_offset != length)
	{
		size_t line_length = get_line_length(assembly + read_offset, length - read_offset);
		size_t new_line_length = get_new_line_legth(assembly + read_offset + line_length, length - read_offset - line_length);
		decode_assembly_line(assembly + read_offset, line_length, &command);
		if (command.type == ASSEBLY_COMMAND_TYPE_ERROR)
			return 0;
		else if (command.type != ASSEBLY_COMMAND_TYPE_NONE)
			++count;
		read_offset += new_line_length + line_length;
	}
	*command_count = count;
	return 1;
}

int write_machine_code(uint8_t* code_buffer, uint8_t write_offset, const char* assembly, size_t assembly_length, uint8_t* used_size)
{
	uint8_t highest_used_address = write_offset;
	size_t command_count;
	if (!validate_assembly(assembly, assembly_length, &command_count))
		return 0;
	assembly_command_t* command_list = (assembly_command_t*)malloc(command_count * sizeof(assembly_command_t));
	if (!command_list)
		return 0;
	for (size_t read_assembly_offset = 0, command_index = 0; command_index != command_count;)
	{
		size_t assembly_line_length = get_line_length(assembly + read_assembly_offset, assembly_length - read_assembly_offset);
		size_t assembly_new_line_length = get_new_line_legth(assembly + read_assembly_offset + assembly_line_length, assembly_length - read_assembly_offset - assembly_line_length);
		decode_assembly_line(assembly + read_assembly_offset, assembly_line_length, command_list + command_index);
		if (command_list[command_index].type == ASSEBLY_COMMAND_TYPE_ERROR)
		{
			free(command_list);
			return 0;
		}
		else if (command_list[command_index].type != ASSEBLY_COMMAND_TYPE_NONE)
			++command_index;
		read_assembly_offset += assembly_line_length + assembly_new_line_length;
	}
	for (size_t command_index = 0; command_index != command_count; ++command_index)
	{
		if (command_list[command_index].type == ASSEBLY_COMMAND_TYPE_SET_CURSOR)
			write_offset = command_list[command_index].command.set_cursor.position;
		else if (command_list[command_index].type == ASSEBLY_COMMAND_TYPE_WRITE_RAW_DATA)
		{
			code_buffer[write_offset] = command_list[command_index].command.write_raw_data.data;
			++write_offset;
			if (write_offset > highest_used_address)
				highest_used_address = write_offset;
		}
		else if (command_list[command_index].type == ASSEBLY_COMMAND_TYPE_WRITE_INSTRUCTION)
		{
			if (command_list[command_index].command.write_instruction.instruction->intermediate)
			{
				code_buffer[write_offset] = 
					((command_list[command_index].command.write_instruction.instruction->machine_code_number & 0xF) << 4) |
					(command_list[command_index].command.write_instruction.intermediate & 0xF);
			}
			else
			{
				code_buffer[write_offset] =
					((command_list[command_index].command.write_instruction.instruction->machine_code_number & 0xF) << 4) |
					((command_list[command_index].command.write_instruction.left_register & 0x3) << 2) |
					((command_list[command_index].command.write_instruction.right_register & 0x3));
			}
			++write_offset;
			if (write_offset > highest_used_address)
				highest_used_address = write_offset;
		}
	}
	free(command_list);
	if (used_size)
		*used_size = highest_used_address;
	return 1;
}