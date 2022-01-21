/*
 * lorawan.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "lorawan.h"

#define EUI_LEN 8
#define APP_EUI_LEN EUI_LEN
#define DEV_EUI_LEN EUI_LEN

#define CMAC_LEN 16
#define MIC_LEN 4

State_t state;

uint8_t lorawan_join() {
	state.DevNonce = random_u16();

	union {
		struct s {
			uint8_t MHDR;
			uint8_t AppEUI[APP_EUI_LEN];
			uint8_t DevEUI[DEV_EUI_LEN];
			uint16_t DevNonce;
			uint8_t MIC[MIC_LEN];
		} params;
		uint8_t buf[sizeof(struct s)];
	} JoinRequest;

	return JOIN_SUCCESS;
}

