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

	//Init Timer1 (16MHz with 1024 presc gives 1tick = 64uS
	TCCR1B |= ( 1 << CS12 ) | ( 1 << CS10 );

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

		uart_puts( "JoinRequest.buf: " );
		uart_puthex( JoinRequest.buf, JOIN_REQUEST_LEN );
		uart_puts( "state.AppKey: " );
		uart_puthex( state.AppKey, 16 );
		lora_putd( JoinRequest.buf, JOIN_REQUEST_LEN );
	}

	lorawan_send_join_request();
	//Wait for TxDone Interrupt
	while( !dio0_flag )
		;
	TCNT1 = 0;
	dio0_flag = 0;
	dio1_flag = 0;

	lora_config_single_rx();
	while( TCNT1 )
		;
	while( TCNT1 < 12590 - 8 )
		;
	//TODO: make this independent of F_CPU

	lora_rx_single();

	//TODO: reset TCNT1 and implement soft timeout
	while( 1 ) {

		if( dio0_flag ) {
			dio0_flag = 0;

			uint8_t len = lora_get_packet_len();
			uint8_t buf[len];

			lora_read_rx( buf, len );
			uart_puthex( buf, len );

			return JOIN_SUCCESS;
		} else if( dio1_flag ) {
			dio1_flag = 0;

			uart_puts( "lorawan_join(): dio1_flag set. RX TIMEOUT\r\n" );
			return JOIN_FAILED;
		}

	}

	return JOIN_SUCCESS;
}

