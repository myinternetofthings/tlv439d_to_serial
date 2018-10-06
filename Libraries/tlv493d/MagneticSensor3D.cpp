/*
 *	Arduino library to control Infineon's Magnetic 3D Sensor TLV493D-A1B6
 *
 *	Have a look at the datasheet for more information. 
 */

#include "MagneticSensor3D.h"
#include "./util/RegMask.h"
#include "./util/BusInterface2.h"
#include "./util/TLV493D_conf.h"
#include <math.h>
#include <delay.h>
#include <stdio.h>

AccessMode_e mMode;
int16_t mXdata;
int16_t mYdata;
int16_t mZdata;
int16_t	mTempdata;

void Tlv493d_resetSensor(uint8_t adr);
void Tlv493d_setRegBits(uint8_t regMaskIndex, uint8_t data);
uint8_t Tlv493d_getRegBits(uint8_t regMaskIndex);
void Tlv493d_calcParity(void);
#define max(a,b) ((a) > (b) ? (a) : (b))

BusInterface_t mInterface;

void Tlv493d_begin(Tlv493d_Address_t slaveAddress, bool reset)
{
	mXdata = 0;
	mYdata = 0;
	mZdata = 0;
	mTempdata = 0;
	mInterface.adress = slaveAddress;

	initInterface(&mInterface, slaveAddress);
	delay(TLV493D_STARTUPDELAY);

	if(reset == true)
	{
		Tlv493d_resetSensor(mInterface.adress);
		delay(TLV493D_STARTUPDELAY);
	}

	// get all register data from sensor
	readOut(&mInterface);
//	printf("Address %02x, registers:\n", mInterface.adress);
//	for(int i = 0; i < TLV493D_BUSIF_READSIZE; i++) {
//		printf("%02x ", mInterface.regReadData[i]);
//	}
//	printf("\n");
	// copy factory settings to write registers
	Tlv493d_setRegBits(W_RES1, Tlv493d_getRegBits(R_RES1));
	Tlv493d_setRegBits(W_RES2, Tlv493d_getRegBits(R_RES2));
	Tlv493d_setRegBits(W_RES3, Tlv493d_getRegBits(R_RES3));
	// enable parity detection
	Tlv493d_setRegBits(W_PARITY_EN, 1);
	// config sensor to lowpower mode
	// also contains parity calculation and writeout to sensor
//	delay(1);
	Tlv493d_setAccessMode(TLV493D_DEFAULTMODE);
//	delay(Tlv493d_getMeasurementDelay());
//	Tlv493d_updateData(10, 10);
}


void Tlv493d_end(void)
{
	Tlv493d_disableInterrupt();
	Tlv493d_setAccessMode(POWERDOWNMODE);
}


void Tlv493d_setAccessMode(AccessMode_e mode)
{
	const AccessMode_t *modeConfig = &(accModes[mode]);
	Tlv493d_setRegBits(W_FAST, modeConfig->fast);
	Tlv493d_setRegBits(W_LOWPOWER, modeConfig->lp);
	Tlv493d_setRegBits(W_LP_PERIOD, modeConfig->lpPeriod);
	Tlv493d_calcParity();
	writeOut(&mInterface);
	mMode = mode;
}


void Tlv493d_enableInterrupt(void)
{
	Tlv493d_setRegBits(W_INT, 1);
	Tlv493d_calcParity();
	writeOut(&mInterface);
}


void Tlv493d_disableInterrupt(void)
{
	Tlv493d_setRegBits(W_INT, 0);
	Tlv493d_calcParity();
	writeOut(&mInterface);
}

void Tlv493d_enableTemp(void)
{
	Tlv493d_setRegBits(W_TEMP_NEN, 0);
	Tlv493d_calcParity();
	writeOut(&mInterface);
}


void Tlv493d_disableTemp(void)
{
	Tlv493d_setRegBits(W_TEMP_NEN, 1);
	Tlv493d_calcParity();
	writeOut(&mInterface);
}


uint16_t Tlv493d_getMeasurementDelay(void)
{
	return accModes[mMode].measurementTime;
}


uint8_t Tlv493d_updateData(uint8_t xyzBits, uint8_t temperatureBits)
{
	uint8_t nBytesToRead = 0;
	// in POWERDOWNMODE, sensor has to be switched on for one measurement
	uint8_t powerdown = 0;
	if(mMode == POWERDOWNMODE) 
	{
		printf("Writing mastercontrolmode\n");
		Tlv493d_setAccessMode(MASTERCONTROLLEDMODE);
		delay(Tlv493d_getMeasurementDelay());
		powerdown = 1;
	}
#ifdef TLV493D_ACCELERATE_READOUT
	// just read the most important results in FASTMODE, if this behavior is desired
	nBytesToRead = xyzBits > 0 && xyzBits <= 8
			? max(nBytesToRead, 3) : max(nBytesToRead, 6);
	nBytesToRead = temperatureBits > 0 && temperatureBits <= 4
			? max(nBytesToRead, 4) : max(nBytesToRead, 7);
//	if(mMode == FASTMODE)
//	{
//		readOut(&mInterface, TLV493D_FAST_READOUT);
//	}
//	else
//	{
//		readOut(&mInterface, TLV493D_MEASUREMENT_READOUT);
//	}
#else
	nBytesToRead = TLV493D_MEASUREMENT_READOUT;
#endif
	readOut(&mInterface, nBytesToRead);
	// construct results from registers
	mXdata = Tlv493d_getRegBits(R_BX1) << 8 | Tlv493d_getRegBits(R_BX2) << 4;
	mXdata >>= 4;
	mYdata = Tlv493d_getRegBits(R_BY1) << 8 | Tlv493d_getRegBits(R_BY2) << 4;
	mYdata >>= 4;
	mZdata = Tlv493d_getRegBits(R_BZ1) << 8 | Tlv493d_getRegBits(R_BZ2) << 4;
	mZdata >>= 4;
	mTempdata = Tlv493d_getRegBits(R_TEMP1) << 12 | Tlv493d_getRegBits(R_TEMP2) << 4;
	mTempdata >>= 4;

#ifdef TLV493D_ACCELERATE_READOUT
	// clear out bits that were not actually read
	if(xyzBits <= 8) {
		mXdata &= 0xFFF0;
		mYdata &= 0xFFF0;
		mZdata &= 0xFFF0;
	}
	if(temperatureBits <= 4) {
		mTempdata &= 0xFF00;
	}
#endif

	// switch sensor back to POWERDOWNMODE, if it was in POWERDOWNMODE before
	if(powerdown)
	{
		Tlv493d_setAccessMode(POWERDOWNMODE);
	}
	// return 0x10 if data was not read at all
	if(nBytesToRead == 0)
		return 0x10;

	// if the return value is 0, all results are from the same frame
	// otherwise some results may be outdated
	if(Tlv493d_getRegBits(R_CHANNEL)==0)
	{
		return Tlv493d_getRegBits(R_POWERDOWNFLAG) ? 0 : 0x04;
	}
	else
	{
		return Tlv493d_getRegBits(R_CHANNEL);
	}
}


float Tlv493d_getX(void)
{
	return (float)mXdata * TLV493D_B_MULT;
}


float Tlv493d_getY(void)
{
	return (float)mYdata * TLV493D_B_MULT;
}


float Tlv493d_getZ(void)
{
	return (float)mZdata * TLV493D_B_MULT;
}


float Tlv493d_getTemp(void)
{
	return (float)(mTempdata-TLV493D_TEMP_OFFSET) * TLV493D_TEMP_MULT;
}


float Tlv493d_getAmount(void)
{
	// sqrt(x^2 + y^2 + z^2)
	return TLV493D_B_MULT * sqrt(pow((float)mXdata, 2) + pow((float)mYdata, 2) + pow((float)mZdata, 2));
}


float Tlv493d_getAzimuth(void)
{
	// arctan(y/x)
	return atan2((float)mYdata, (float)mXdata);
}


float Tlv493d_getPolar(void)
{
	// arctan(z/(sqrt(x^2+y^2)))
	return atan2((float)mZdata, sqrt(pow((float)mXdata, 2) + pow((float)mYdata, 2)));
}

uint8_t *Tlv493d_getRegisters(void)
{
	return mInterface.regReadData;
}

/* internal function called by begin()
 * The sensor has a special reset sequence which allows to change its i2c address by setting SDA to high or low during a reset. 
 * As some i2c peripherals may not cope with this, the simplest way is to use for this very few bytes bitbanging on the SCL/SDA lines.
 * Furthermore, as the uC may be stopped during a i2c transmission, a special recovery sequence allows to bring the bus back to
 * an operating state.
 */
void Tlv493d_resetSensor(uint8_t adr)     // Recovery & Reset - this can be handled by any uC as it uses bitbanging
{
	uint8_t resetBuffer[1];
	if (adr == TLV493D_ADDRESS1) {
		// if the sensor shall be initialized with i2c address 0x1F
		resetBuffer[0] = 0x00;
	} else {
		// if the sensor shall be initialized with address 0x5E
		resetBuffer[0] = 0xFF;
	}
	I2C_MASTER_STATUS_t result = I2C_MASTER_Transmit(&I2C_MASTER_0, true, 0x00, resetBuffer, sizeof(resetBuffer), true);
	if(result == I2C_MASTER_STATUS_SUCCESS)
		 while(I2C_MASTER_IsTxBusy(&I2C_MASTER_0));
//
//	I2C_MASTER_SendStart(&I2C_MASTER_0, 0, XMC_I2C_CH_CMD_WRITE);
//
//	if (adr == TLV493D_ADDRESS1) {
//		// if the sensor shall be initialized with i2c address 0x1F
//		I2C_MASTER_TransmitByte(&I2C_MASTER_0, 0x00);
//	} else {
//		// if the sensor shall be initialized with address 0x5E
//		I2C_MASTER_TransmitByte(&I2C_MASTER_0, 0xFF);
//	}
//
//	I2C_MASTER_SendStop(&I2C_MASTER_0);
}

void Tlv493d_setRegBits(uint8_t regMaskIndex, uint8_t data)
{
	if(regMaskIndex < TLV493D_NUM_OF_REGMASKS)
	{
		setToRegs(&(regMasks[regMaskIndex]), mInterface.regWriteData, data);
	}
}

uint8_t Tlv493d_getRegBits(uint8_t regMaskIndex)
{
	if(regMaskIndex < TLV493D_NUM_OF_REGMASKS)
	{
		const RegMask_t *mask = &(regMasks[regMaskIndex]);
		if(mask->rw == REGMASK_READ)
		{
			return getFromRegs(mask, mInterface.regReadData);
		}
		else
		{
			return getFromRegs(mask, mInterface.regWriteData);
		}
	}
	return 0;
}

void Tlv493d_calcParity(void)
{
	uint8_t i;
	uint8_t y = 0x00;
	// set parity bit to 1
	// algorithm will calculate an even parity and replace this bit, 
	// so parity becomes odd
	Tlv493d_setRegBits(W_PARITY, 1);
	// combine array to one byte first
	for(i = 0; i < TLV493D_BUSIF_WRITESIZE; i++)
	{
		y ^= mInterface.regWriteData[i];
	}
	// combine all bits of this byte
	y = y ^ (y >> 1);
	y = y ^ (y >> 2);
	y = y ^ (y >> 4);
	// parity is in the LSB of y
	Tlv493d_setRegBits(W_PARITY, y&0x01);
}
