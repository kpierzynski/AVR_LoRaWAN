/*
 * lorawan.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "lorawan.h"

State_t state;

uint8_t lorawan_mhdr( uint8_t mtype ) {
	return ( ( mtype & 0b111 ) << 5 );
}

uint8_t lorawan_init() {

	if( lora_init() != INIT_RADIO_SUCCESS ) {
		return INIT_FAILED;
	}

	memcpy( state.DevEUI, (uint8_t[] ) { DEV_EUI }, DEV_EUI_LEN );
	memcpy( state.AppEUI, (uint8_t[] ) { APP_EUI }, APP_EUI_LEN );
	memcpy( state.AppKey, (uint8_t[] ) { APP_KEY }, APP_KEY_LEN );

	return INIT_SUCCESS;
}

uint8_t lorawan_join() {
	auto void lorawan_send_join_request();

	void lorawan_send_join_request() {
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

		JoinRequest.params.DevNonce = state.DevNonce;
		JoinRequest.params.MHDR = lorawan_mhdr( JOIN_REQUEST );
		memcpy( JoinRequest.params.DevEUI, state.DevEUI, DEV_EUI_LEN );
		memcpy( JoinRequest.params.AppEUI, state.AppEUI, APP_EUI_LEN );

		cmac_gen( state.AppKey, JoinRequest.buf, 19, JoinRequest.params.MIC );

		lora_putd( JoinRequest.buf, JOIN_REQUEST_LEN );
	}

	return JOIN_SUCCESS;
}

