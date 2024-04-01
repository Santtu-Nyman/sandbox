#include <stdlib.h>
#include <stdio.h>

void create_vhdl_increment_circuit(const char* file_name, int signal_length)
{
	FILE* file_handle = fopen(file_name, "w");
	fprintf(file_handle,
		"library ieee;\n"
		"use ieee.std_logic_1164.all;\n\n"
		"entity increment_%i is\n"
		"	port (D : in std_logic_vector(%i downto 0); Q : out std_logic_vector(%i downto 0));\n"
		"end increment_%i;\n\n"
		"architecture increment_%i_arch of increment_%i is\n",
		signal_length, signal_length - 1, signal_length - 1, signal_length, signal_length, signal_length);
	if (signal_length > 3)
	{
		fprintf(file_handle,
			"	signal and_vector : std_logic_vector(%i downto 0);\n"
			"begin\n"
			"	Q(0) <= not D(0);\n"
			"	and_vector(0) <= D(1) and D(0);\n"
			"	Q(1) <= D(1) xor D(0);\n",
			signal_length - 3);
		for (int bit_index = 2; bit_index != signal_length - 1; ++bit_index)
		{
			fprintf(file_handle,
				"	and_vector(%i) <= D(%i) and and_vector(%i);\n"
				"	Q(%i) <= D(%i) xor and_vector(%i);\n",
				bit_index - 1, bit_index, bit_index - 2, bit_index, bit_index, bit_index - 2);
		}
		fprintf(file_handle, "	Q(%i) <= D(%i) xor and_vector(%i);\n", signal_length - 1, signal_length - 1, signal_length - 3);
	}
	else
	{
		fprintf(file_handle,
			"begin\n"
			"	Q(0) <= not D(0);\n");
		if (signal_length > 1)
			fprintf(file_handle, "	Q(1) <= D(1) xor D(0);");
		if (signal_length > 2)
			fprintf(file_handle, "	Q(2) <= D(2) xor (D(1) and D(0));");
	}
	fprintf(file_handle, "end increment_%i_arch;\n", signal_length);
	fclose(file_handle);
}

int main(int argc, char** argv)
{
	create_vhdl_increment_circuit(argv[1], atoi(argv[2]));
	return 0;
}