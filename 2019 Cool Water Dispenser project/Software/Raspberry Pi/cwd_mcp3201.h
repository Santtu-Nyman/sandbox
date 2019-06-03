/*
	MCP3201 Library version 1.0.0 2019-03-02 by Santtu Nyman.
	git https://github.com/AP-Elektronica-ICT/ip2019-coolwater
	
	Description
		Just one function to read output of MCP3201 12 bit ADC.
		
	Version history
		Version 1.0.0 2019-03-02
			First and hopefully last version.
*/

#ifndef CWD_MCP3201_H
#define CWD_MCP3201_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

int cwd_mcp3201_read(uint8_t spi_cs, uint8_t spi_clk, uint8_t spi_dout);
/*
	Description
		Function reads 12 bit value from mcp3201 ADC.
	Parameters
		spi_cs
			SPI chip select pin number.
		spi_clk
			SPI clock pin number.
		spi_dout
			SPI data out pin number.
	Return
		Function returns the 12 bit value read form the ADC.
		All other bits of the return value are zeroes except 12 low bits.
*/

#ifdef __cplusplus
}
#endif

#endif