#include "fpu.h"

#define CPACR 		(*(volatile unsigned int *) 0xE000ED88)
#define	CPEN10_11	(0xF << 20)

void fpu_enable(void)
{
	CPACR |= CPEN10_11;
}
