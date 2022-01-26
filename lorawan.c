/*
 * lorawan.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "lorawan.h"

State_t state;

void (*lora_downlink_event_callback)( uint8_t * buf, uint8_t len );

void register_lorawan_downlink_callback( void (*callback)( uint8_t *buf, uint8_t len ) ) {
	lora_downlink_event_callback = callback;
}

uint8_t lorawan_mhdr( uint8_t mtype ) {
	return ( ( mtype & 0b111 ) << 5 );
}

uint8_t lorawan_fctrl( uint8_t adr, uint8_t adrackreq, uint8_t ack, uint8_t foptslen ) {
	return ( ( adr ? 1 : 0 ) << 7 ) | ( ( adrackreq ? 1 : 0 ) << 6 ) | ( ( ack ? 1 : 0 ) << 5 ) | ( foptslen & 0b1111 );
}

uint8_t lorawan_init() {

	if( lora_init() != INIT_RADIO_SUCCESS ) {
		return INIT_FAILED;
	}

	memcpy( state.DevEUI, (uint8_t[ ] ) { DEV_EUI }, DEV_EUI_LEN );
	memcpy( state.AppEUI, (uint8_t[ ] ) { APP_EUI }, APP_EUI_LEN );
	memcpy( state.AppKey, (uint8_t[ ] ) { APP_KEY }, APP_KEY_LEN );

	//Init Timer1 (16MHz with 1024 presc gives 1tick = 64uS
	TCCR1B |= ( 1 << CS12 ) | ( 1 << CS10 );
	TCNT1 = 0;

	return INIT_SUCCESS;
}
