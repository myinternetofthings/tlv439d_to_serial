/*
 * main.c
 *
 *  Created on: 2018 Apr 25 21:53:45
 *  Author: Michal Milkowski
 */
#include <DAVE.h>                 //Declarations from DAVE Code Generation (includes SFR declaration)
#include "tlv493d/MagneticSensor3D.h"
#include <stdio.h>
#include <delay.h>
#include <gui_link.h>

/** global 1ms tick */
volatile uint32_t tick;
/** buffer for single command received */
char cmdBuffer[128];
uint8_t cmdBufferIdx = 0;
bool cmdReceived = false;
uint8_t uartRxByte;
/** configuration structure updated by command interpreter */
GUI_TLV493D_CONF_t guiCommandConf = {
		.version = "0.1.0",
		.specialCommand = NONE,
		.readIntervalMs = 500,
		.readXYZ = XYZ_NONE,
		.readTemperature = TEMP_NONE
};
/** main program state variables */
/** temperature conversion is enabled on chip */
bool temperatureEnabled = false;
/** last data readout status */
bool dataValid = false;

void sysTimer(void *unused) {
	tick++;
	DIGITAL_IO_ToggleOutput(&LED2);
}

/**
 * Read requested data from sensor
 */
void readData() {
	uint8_t xyzBits, temperatureBits, result;
	switch(guiCommandConf.readXYZ) {
	case XYZ_8BIT:
		xyzBits = 8;
		break;
	case XYZ_10BIT:
		xyzBits = 10;
		break;
	default:
		xyzBits = 0;
	}

	switch(guiCommandConf.readTemperature) {
	case TEMP_4BIT:
		temperatureBits = 4;
		break;
	case TEMP_10BIT:
		temperatureBits = 10;
		break;
	default:
		xyzBits = 0;
	}

	result = Tlv493d_updateData(xyzBits, temperatureBits);
	dataValid = result == 0;
//	printf("updateData = %02x\n", result);
}

void sendMeasurements() {
	if(guiCommandConf.readXYZ == XYZ_NONE && guiCommandConf.readTemperature == TEMP_NONE)
		return;

	float x = Tlv493d_getX();
	float y = Tlv493d_getY();
	float z = Tlv493d_getZ();
	float t = Tlv493d_getTemp();
	guiStartFrame();
	guiSendStatus(dataValid);
	if(guiCommandConf.readXYZ != XYZ_NONE)
		guiSendFieldComponents(x, y, z);
	if(guiCommandConf.readTemperature != TEMP_NONE)
		guiSendTemperature(t);
	guiEndFrame();
}

/**

 * @brief main() - Application entry point
 *
 * <b>Details of function</b><br>
 * This routine is the application entry point. It is invoked by the device startup code. It is responsible for
 * invoking the APP initialization dispatcher routine - DAVE_Init() and hosting the place-holder for user application
 * code.
 */

int main(void)
{
	/* Initialization of DAVE APPs  */
  if(DAVE_Init() != DAVE_STATUS_SUCCESS)
  {
    /* Placeholder for error handler code. The while loop below can be replaced with an user error handler. */
    XMC_DEBUG("DAVE APPs initialization failed\n");
    while(1U){}
  }

  if(UART_Receive(&UART_0, &uartRxByte, 1) != UART_STATUS_SUCCESS) {
  	XMC_DEBUG("UART Rx setup failed\n");
  	while(1){}
  }

  PWM_SetDutyCycle(&MyLed, 20);
  uint32_t tid = SYSTIMER_CreateTimer(1000, SYSTIMER_MODE_PERIODIC, sysTimer, NULL);
  XMC_ASSERT("Timer create failed", tid != 0);
  uint32_t result = SYSTIMER_StartTimer(tid);
  XMC_ASSERT("Timer start failed", result == SYSTIMER_STATUS_SUCCESS);

//  printf("Before Tlv begin\n");
  Tlv493d_begin(TLV493D_ADDRESS2, false);
//  printf("After Tlv begin\n");
  // not using interrupt mode for now
	Tlv493d_disableInterrupt();
  // enable temperature conversion
  if(guiCommandConf.readTemperature != TEMP_NONE) {
  	Tlv493d_enableTemp();
  	temperatureEnabled = true;
  }
  // main loop
  while(1U)
  {
  	// wait for another conversion
  	delay(guiCommandConf.readIntervalMs);
  	// read out data
  	readData();

  	// send current readings
  	sendMeasurements();

  	// command interperter
  	if(cmdReceived) {
  		cmdReceived = false;
  		guiProcessCommand(cmdBuffer, &guiCommandConf);
  	  if(UART_Receive(&UART_0, &uartRxByte, 1) != UART_STATUS_SUCCESS) {
  	  	XMC_DEBUG("UART Rx setup failed in loop\n");
  	  }
  	}

  	// execute special commands
  	if(guiCommandConf.specialCommand == RESET) {
  		printf("Not implemented\n");
  	} else if(guiCommandConf.specialCommand == READ_REGISTERS) {
  		uint8_t *registers = Tlv493d_getRegisters();
			for(int i = 0; i < 10; i++)
				printf("%02x, ", registers[i]);
			printf("\n");
  	}
		guiCommandConf.specialCommand = NONE;

  	// enable or disable temperature measurement on chip
  	if(temperatureEnabled && guiCommandConf.readTemperature == TEMP_NONE) {
  		Tlv493d_disableTemp();
  		temperatureEnabled = false;
  	} else if(!temperatureEnabled && guiCommandConf.readTemperature != TEMP_NONE) {
  		Tlv493d_enableTemp();
  		temperatureEnabled = true;
  	}

  }
}

void uartRxCallback(void) {
	// command complete when end of line received
	if(uartRxByte == '\n') {
		if(cmdBufferIdx > 0) {
			// command is saved to buffer, and processed in another thread
			cmdBuffer[cmdBufferIdx] = 0;
			cmdReceived = true;
		}
		// reset buffer after processing
		cmdBufferIdx = 0;
	} else if(uartRxByte >= 32 && uartRxByte <= 126){
		// save only printable characters to buffer, ignore others
		// last byte reserved for terminating 0
		if(cmdBufferIdx < sizeof(cmdBuffer) - 1) {
			cmdBuffer[cmdBufferIdx++] = uartRxByte;
		} else {
			// reset buffer on overflow
			cmdBufferIdx = 0;
		}
	}
	// command buffer has to be processed in main thread first
	if(cmdReceived == false)
		UART_Receive(&UART_0, &uartRxByte, 1);
}

/* This "wires" the putchar/printf functions to transmit to UART_0 */
int _write(int file, uint8_t *buf, int nbytes)
{
    if(UART_Transmit(&UART_0, buf, nbytes) == UART_STATUS_SUCCESS) {
       while(UART_0.runtime->tx_busy) {}
    }
    return nbytes;
}
