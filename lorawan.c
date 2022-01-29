/*
 * lorawan.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "lorawan.h"

const uint8_t DataRates[ DATARATE_LEN ][ 2 ] PROGMEM = {
        { SF12, BW125 },
        { SF11, BW125 },
        { SF10, BW125 },
        { SF9, BW125 },
        { SF8, BW125 },
        { SF7, BW125 },
        { SF8, BW250 }
};

const uint32_t Channels[ TTN_LORA_CHANNELS_LEN ] PROGMEM = {
        868100000,
        868300000,
        868500000,
        867100000,
        867300000,
        867500000,
        867700000,
        867900000
};

State_t state;

void (*lora_downlink_event_callback)( uint8_t fport, uint8_t * buf, uint8_t len );

void register_lorawan_downlink_callback( void (*callback)( uint8_t fport, uint8_t *buf, uint8_t len ) ) {
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

	state.Channels.main_channels[ 0 ] = 868100000UL;
	state.Channels.main_channels[ 1 ] = 868300000UL;
	state.Channels.main_channels[ 2 ] = 868500000UL;

	state.rx1.delay_sec = 5;

	uint8_t random_channel = random_u8() % 3;
	state.rx1.settings = (RadioSettings_t ) { SF7, BANDWIDTH_125_KHZ, CODING_RATE_4_5, state.Channels.main_channels[ random_channel ] };
	lora_set_settings( &state.rx1.settings );

	return INIT_SUCCESS;
}

void lorawan_encrypt_FRMPayload( uint8_t * key, uint8_t Dir, uint32_t FCnt, uint8_t * payload, uint8_t len ) {

	uint8_t pos = 0;
	uint8_t i = 1;

	uint8_t a_i[ 16 ] = { 0x01, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, i };
	memcpy( a_i + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
	memcpy( a_i + 10, &FCnt, 4 );

	uint8_t n = ( len + 15 ) / 16;

	for( i = 1; i <= n; i++ ) {
		uint8_t max = 16;

		if( i == n )
			max = ( ( len % 16 ) == 0 ) ? 16 : ( len % 16 );

		a_i[ 15 ] = i;

		uint8_t s_i[ 16 ];
		memcpy( s_i, a_i, 16 );

		aes_encrypt( key, s_i );

		for( uint8_t b = 0; b < max; b++ )
			payload[ pos + b ] = payload[ pos + b ] ^ s_i[ b ];
		pos += max;
	}
}

void lorawan_mic_uplink(uint8_t Dir, uint32_t FCnt, uint8_t * payload, uint8_t len, uint8_t * MIC) {

	uint8_t b_0[ 16 + len ];

	memcpy( b_0, (uint8_t[ ] ) { 0x49, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, len }, 16 );
	memcpy( b_0 + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
	memcpy( b_0 + 10, &FCnt, 4 );

	memcpy( b_0 + 16, payload, len );
	cmac_gen( state.NwkSKey, b_0, 16 + len, MIC );
}

