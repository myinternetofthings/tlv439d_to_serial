/*
 * gui_link.h
 *
 *  Created on: Sep 13, 2018
 *      Author: Test
 */

#ifndef GUI_LINK_H_
#define GUI_LINK_H_

typedef enum GUI_TLV493D_SPECIAL_COMMAND {
	NONE,
	READ_REGISTERS,
	RESET,
} GUI_TLV493D_SPECIAL_COMMAND_t;

typedef enum GUI_TLV493D_READ_XYZ {
	XYZ_NONE,
	XYZ_8BIT,
	XYZ_10BIT,
	XYZ_MAX
} GUI_TLV493D_READ_XYZ_t;

typedef enum GUI_TLV493D_READ_TEMP {
	TEMP_NONE,
	TEMP_4BIT,
	TEMP_10BIT,
	TEMP_MAX
} GUI_TLV493D_READ_TEMP_t;

typedef struct GUI_TLV493D_CONF
{
	char *const version;
	GUI_TLV493D_SPECIAL_COMMAND_t specialCommand;   		/**< execute special command */
	GUI_TLV493D_READ_XYZ_t readXYZ;    /**< read xyz field components */
	GUI_TLV493D_READ_TEMP_t readTemperature; /**< read temperature */
	uint16_t readIntervalMs; /**< interval between samples [ms] */
} GUI_TLV493D_CONF_t;

void guiProcessCommand(char *command, GUI_TLV493D_CONF_t *state);
void guiStartFrame();
void guiEndFrame();
void guiSendFieldComponents(float x, float y, float z);
void guiSendStatus(bool dataValid);
void guiSendTemperature(float t);

#endif /* GUI_LINK_H_ */
