/*
 * delay.c

 *
 *  Created on: May 15, 2018
 *      Author: Test
 */

#include <inttypes.h>
#include "delay.h"

extern volatile uint32_t tick;

void delay(uint32_t ms) {
	uint32_t start = tick;
	while(tick - start < ms);
}
