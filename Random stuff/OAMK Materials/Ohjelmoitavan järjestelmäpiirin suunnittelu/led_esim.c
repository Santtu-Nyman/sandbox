#include "stdio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"

int main()
{
	printf("Hello from Nios II!\n");
	int count = 0;
	int delay;
	while(1)
	{
		// IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, count & 0x01);
		// IORD(base, offset)
		// IOWR(base, offset, value)
		IOWR(0x00020810, 0x00, count & 0x0ff);
		delay = 0;
		while (delay < 2000000)
		{
			delay++;
		}
		count++;
	}
	return 0;
}