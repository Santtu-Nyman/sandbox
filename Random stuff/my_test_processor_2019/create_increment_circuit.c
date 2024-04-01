#include <stdlib.h>
#include <stdio.h>

void create_vhdl_increment_circuit(const char* file_name, int signal_length, const char* source_signal_name, const char* destination_signal_name)
{
	FILE* file_handle = fopen(file_name, "w");
	for (int destination_bit_index = signal_length; --destination_bit_index;)
	{
		fprintf(file_handle, "%s(%i) <= %s(%i) xor\n\t(", destination_signal_name, destination_bit_index, source_signal_name, destination_bit_index);
		for (int source_bit_index = 0; source_bit_index != destination_bit_index; ++source_bit_index)
			fprintf(file_handle, "%s(%i)%s", source_signal_name, source_bit_index, (source_bit_index != (destination_bit_index - 1) ? (source_bit_index && source_bit_index % 8 == 7 ? " and\n\t" : " and ") : ");\n"));
	}
	fprintf(file_handle, "%s(0) <= not %s(0);\n", destination_signal_name, source_signal_name);
	fclose(file_handle);
}

int main(int argc, char** argv)
{
	create_vhdl_increment_circuit(argv[1], atoi(argv[2]), argv[3], argv[4]);
	return 0;
}