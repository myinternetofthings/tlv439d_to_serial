/*
 * gui_link.c
 *
 *  Created on: Sep 13, 2018
 *      Author: Test
 */
#include <DAVE.h>                 //Declarations from DAVE Code Generation (includes SFR declaration)
#include <gui_link.h>
#include "tlv493d/MagneticSensor3D.h"
#include <stdio.h>
#include <ctype.h>

#define CMD_RESET "reset"
#define CMD_READ_REGISTERS "registers"
#define CMD_SET_UPDATE_INTERVAL "set interval"
#define CMD_SET_XYZ_READ_MODE "set xyzmode"
#define CMD_SET_TEMP_READ_MODE "set tmode"
#define CMD_GET_VERSION "version"

#define ERROR_INVALID_COMMAND "error: invalid command"
#define ERROR_INVALID_ARGUMENT "error: invalid argument"

void guiProcessCommand(char *command, GUI_TLV493D_CONF_t *state) {
//	printf("Received command: %s\n", command);
	if(strncasecmp(CMD_RESET, command, sizeof(CMD_RESET) - 1) == 0) {
		state->specialCommand = RESET;
	} else if(strncasecmp(CMD_READ_REGISTERS, command, sizeof(CMD_READ_REGISTERS) - 1) == 0) {
		state->specialCommand = READ_REGISTERS;
	} else if(strncasecmp(CMD_GET_VERSION, command, sizeof(CMD_GET_VERSION) - 1) == 0) {
		printf("%s\n", state->version);
	} else if(strncasecmp(CMD_SET_UPDATE_INTERVAL, command, sizeof(CMD_SET_UPDATE_INTERVAL) - 1) == 0) {
		uint16_t v;
		if(sscanf(&command[sizeof(CMD_SET_UPDATE_INTERVAL)], "%hu", &v) == 1) {
			if(v > 0) {
				state->readIntervalMs = v;
			} else {
				printf(ERROR_INVALID_ARGUMENT "\n");
			}
		}
	} else if(strncasecmp(CMD_SET_XYZ_READ_MODE, command, sizeof(CMD_SET_XYZ_READ_MODE) - 1) == 0) {
		// find first non white space character
		command += sizeof(CMD_SET_XYZ_READ_MODE);
		while(*command != 0) {
			if(isspace((int)*command) == false) {
				char v = *command - '0';
				// validate enum range
				if(v >= 0 && v < XYZ_MAX) {
					state->readXYZ = v;
				} else {
					printf(ERROR_INVALID_ARGUMENT "\n");
				}
				break;
			}
			command++;
		}
	} else if(strncasecmp(CMD_SET_TEMP_READ_MODE, command, sizeof(CMD_SET_TEMP_READ_MODE) - 1) == 0) {
		// find first non white space character
		command += sizeof(CMD_SET_TEMP_READ_MODE);
		while(*command != 0) {
			if(isspace((int)*command) == false) {
				char v = *command - '0';
				// validate enum range
				if(v >= 0 && v < TEMP_MAX) {
					state->readTemperature = v;
				} else {
					printf(ERROR_INVALID_ARGUMENT "\n");
				}
				break;
			}
			command++;
		}
	} else {
		printf(ERROR_INVALID_COMMAND "\n");
	}
}

void guiStartFrame() {

}

void guiEndFrame() {
	printf("\n");
}

void guiSendFieldComponents(float x, float y, float z) {
	printf("x=%+.2f, y=%+.2f, z=%+.2f, ", x, y, z);
}

void guiSendStatus(bool dataValid) {
	printf("%c, ", dataValid ? 'v' : 'x');
}

void guiSendTemperature(float t) {
	printf("t=%+.2f, ", t);
}
