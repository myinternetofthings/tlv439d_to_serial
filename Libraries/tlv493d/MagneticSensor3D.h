/*
 *	Arduino library to control Infineon's Magnetic 3D Sensor TLV493D-A1B6
 *
 *	Have a look at the datasheet for more information. 
 */


#ifndef MAGNETICSENSOR3D_H_INCLUDED
#define MAGNETICSENSOR3D_H_INCLUDED

#include <DAVE.h>                 //Declarations from DAVE Code Generation (includes SFR declaration)
#include "./util/BusInterface.h"
#include "./util/TLV493D.h"

typedef enum Tlv493d_Address
{
	TLV493D_ADDRESS1	=	0x1F,
	TLV493D_ADDRESS2	=	0x5E
}Tlv493d_Address_t;


/*
 * TLV493D_ACCELERATE_READOUT lets the controller just read out the first 3 bytes when in fast mode. 
 * This makes the readout faster (half of usual transfer duration), but there is no way to get 
 * temperature, current channel or high precision (only 8 instead of 12 bits for x, y, z)
 * It is necessary for slow I2C busses to read the last result before the new measurement is completed. 
 * It only takes effect in FASTMODE, not in other modes. 
 *
 * Feel free to undefine this and increase your I2C bus speed if you need to. 
 */
//#define TLV493D_ACCELERATE_READOUT

#ifdef __cplusplus
extern "C" {
#endif

void Tlv493d_begin(Tlv493d_Address_t slaveAddress, bool reset);
void Tlv493d_end(void);
	
	// sensor configuration
	/* sets the data access mode for TLE493D
	 * Tle493d is initially in POWERDOWNMODE
	 * use POWERDOWNMODE for rare and infrequent measurements 
	 * 	Tle493d will automatically switch to MASTERCONTROLLEDMODE for one measurement if on a readout
	 *	measurements are quite slow in this mode. The power consumption is very low between measurements. 
	 * use MASTERCONTROLLEDMODE for low measurement frequencies where results do not have to be up-to-date
	 *	In this mode a new measurement starts directly after the last result has been read out. 
	 * use LOWPOWERMODE and ULTRALOWPOWERMODE for continuous measurements
	 *	each readout returns the latest measurement results
	 * use FASTMODE for for continuous measurements on high frequencies
	 *	measurement time might be higher than the time necessary for I2C-readouts in this mode. 
	 */
	typedef enum AccessMode_et
	{
		POWERDOWNMODE = 0,
		FASTMODE,
		LOWPOWERMODE,
		ULTRALOWPOWERMODE,
		MASTERCONTROLLEDMODE,
	}AccessMode_e;
	void Tlv493d_setAccessMode(AccessMode_e mode);
	// interrupt is disabled by default
	// it is recommended for FASTMODE, LOWPOWERMODE and ULTRALOWPOWERMODE
	// the interrupt is indicated with a short(1.5 us) low pulse on SCL
	// you need to capture and react(read the new results) to it by yourself
	void Tlv493d_enableInterrupt(void);
	void Tlv493d_disableInterrupt(void);
	// temperature measurement is enabled by default
	// it can be disabled to reduce power consumption
	void Tlv493d_enableTemp(void);
	void Tlv493d_disableTemp(void);
	
	// returns the recommended time between two readouts for the sensor's current configuration
	uint16_t Tlv493d_getMeasurementDelay(void);
	// read measurement results from sensor
	// @returns 0 if data is valid
	// CH register if conversion was incomplete (0-3),
	// 0x04 when power down flag was not set
	// 0x10 when no registers were read
	uint8_t Tlv493d_updateData(uint8_t xyzBits, uint8_t temperatureBits);
	
	// fieldvector in Cartesian coordinates
	float Tlv493d_getX(void);
	float Tlv493d_getY(void);
	float Tlv493d_getZ(void);
	
	// fieldvector in spherical coordinates
	float Tlv493d_getAmount(void);
	float Tlv493d_getAzimuth(void);
	float Tlv493d_getPolar(void);
	
	// temperature
	float Tlv493d_getTemp(void);

	// raw registers
	uint8_t *Tlv493d_getRegisters(void);

#ifdef __cplusplus
}
#endif
#endif		/* MAGNETICSENSOR3D_H_INCLUDED */
