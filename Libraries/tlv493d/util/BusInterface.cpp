#include <DAVE.h>
#include "BusInterface2.h"
#include <stdio.h>

void initInterface(BusInterface_t *interface, uint8_t adress)
{
	uint8_t i;
	interface->adress = adress;
	for(i = 0; i < TLV493D_BUSIF_READSIZE; i++) {
		interface->regReadData[i] = 0x00;;
	}
	for(i = 0; i < TLV493D_BUSIF_WRITESIZE; i++) {
		interface->regWriteData[i] = 0x00;;
	}
}

void readOut(BusInterface_t *interface)
{
	readOut(interface, TLV493D_BUSIF_READSIZE);
}

void readOut(BusInterface_t *interface, uint8_t count)
{
	if(count > TLV493D_BUSIF_READSIZE)
	{
		count = TLV493D_BUSIF_READSIZE;
	} else if(count == 0) {
		return;
	}

	I2C_MASTER_STATUS_t result = I2C_MASTER_Receive(&I2C_MASTER_0, true, interface->adress << 1, interface->regReadData, count, true, true);
	if(result == I2C_MASTER_STATUS_SUCCESS)
		while(I2C_MASTER_IsRxBusy(&I2C_MASTER_0));
	else
		printf("I2C Rx failed: %d\n", result);
//	for(int i = 0; i < count; i++)
//		printf("%02x ", interface->regReadData[i]);
//	printf("\n");
}

void writeOut(BusInterface_t *interface)
{
	writeOut(interface, TLV493D_BUSIF_WRITESIZE);
}

void writeOut(BusInterface_t *interface, uint8_t count)
{
	if(count > TLV493D_BUSIF_WRITESIZE)
	{
		count = TLV493D_BUSIF_WRITESIZE;
	}
	I2C_MASTER_STATUS_t result = I2C_MASTER_Transmit(&I2C_MASTER_0, true, interface->adress << 1, interface->regWriteData, count, true);
	if(result == I2C_MASTER_STATUS_SUCCESS)
		 while(I2C_MASTER_IsTxBusy(&I2C_MASTER_0));
	else
		printf("I2C Tx failed: %d\n", result);
}

