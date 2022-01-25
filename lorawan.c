/*
 * lorawan.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "lorawan.h"

static State_t state;

static uint8_t lorawan_mhdr( uint8_t mtype ) {
	return ( ( mtype & 0b111 ) << 5 );
}

static uint8_t lorawan_fctrl( uint8_t adr, uint8_t adrackreq, uint8_t ack, uint8_t foptslen ) {
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

uint8_t lorawan_join() {
	auto void lorawan_send_join_request();
	auto uint8_t lorawan_parse_join_accept( uint8_t * data, uint8_t data_len );
	auto void lorawan_derive_keys();

	void lorawan_send_join_request() {
		state.DevNonce = random_u16();
		uart_puts( "DevNonce: " );
		uart_putint( state.DevNonce, 16 );
		uart_putln();

		union {
			struct s {
				uint8_t MHDR;
				uint8_t AppEUI[ APP_EUI_LEN ];
				uint8_t DevEUI[ DEV_EUI_LEN ];
				uint16_t DevNonce;
				uint8_t MIC[ MIC_LEN ];
			} params;
			uint8_t buf[ sizeof(struct s) ];
		} JoinRequest;

		//JoinRequest.params.DevNonce = state.DevNonce;
		JoinRequest.params.DevNonce = ( state.DevNonce >> 8 ) & 0xFF;
		JoinRequest.params.DevNonce |= ( state.DevNonce << 8 );
		//Todo: ??? wtf is this

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

	uint8_t lorawan_parse_join_accept( uint8_t* data, uint8_t data_len ) {

		union {
			struct s {
				uint8_t MHDR;
				uint8_t AppNonce[ APP_NONCE_LEN ];
				uint8_t NetID[ NET_ID_LEN ];
				uint8_t DevAddr[ DEV_ADDR_LEN ];
				uint8_t DLSettings;
				uint8_t RXDelay;
				uint8_t CFList[ 16 ];
				uint8_t MIC[ MIC_LEN ];
			} params;

			uint8_t buf[ 33 ];
		} JoinAccept;
		JoinAccept.params.MHDR = data[ 0 ];

		if( data_len != 33 )
			return PARSE_FAILED;
		if( ( ( JoinAccept.params.MHDR >> 5 ) & 0b111 ) != JOIN_ACCEPT )
			return PARSE_FAILED;

		memcpy( JoinAccept.buf, data, data_len );

		aes_encrypt( state.AppKey, JoinAccept.buf + 1 );
		aes_encrypt( state.AppKey, JoinAccept.buf + 1 + 16 );

		memcpy( state.JoinAccept.AppNonce, JoinAccept.params.AppNonce, APP_NONCE_LEN );
		memcpy( state.JoinAccept.NetID, JoinAccept.params.NetID, NET_ID_LEN );
		memcpy( state.JoinAccept.DevAddr, JoinAccept.params.DevAddr, DEV_ADDR_LEN );

		uart_puts( "state.JoinAccept.AppNonce: " );
		uart_puthex( state.JoinAccept.AppNonce, APP_NONCE_LEN );

		uart_puts( "state.JoinAccept.NetID: " );
		uart_puthex( state.JoinAccept.NetID, NET_ID_LEN );

		uart_puts( "state.JoinAccept.DevAddr: " );
		uart_puthex( state.JoinAccept.DevAddr, DEV_ADDR_LEN );

		return PARSE_SUCCESS;
	}

	void lorawan_derive_keys() {
#define KEYDERICATION_PAD16_LEN 7
#define KEYDERICATION_PAD16 16-KEYDERICATION_PAD16_LEN

		union {
			struct s {
				uint8_t type;
				uint8_t AppNonce[ APP_NONCE_LEN ];
				uint8_t NetID[ NET_ID_LEN ];
				uint16_t DevNonce;
			} params;
			uint8_t buf[ sizeof(struct s) + KEYDERICATION_PAD16_LEN ];
		} KeyDerivation;

		memset( KeyDerivation.buf + KEYDERICATION_PAD16, 0x00, KEYDERICATION_PAD16_LEN );

		memcpy( KeyDerivation.params.AppNonce, state.JoinAccept.AppNonce, APP_NONCE_LEN );
		memcpy( KeyDerivation.params.NetID, state.JoinAccept.NetID, NET_ID_LEN );
		//KeyDerivation.params.DevNonce = state.DevNonce;
		//TODO: why i had to swap bytes here
		KeyDerivation.params.DevNonce = ( state.DevNonce >> 8 ) & 0xFF;
		KeyDerivation.params.DevNonce |= ( state.DevNonce << 8 );

		KeyDerivation.params.type = 0x01;
		memcpy( state.NwkSKey, KeyDerivation.buf, 16 );
		uart_puts( "Data for nwkskey derivation: " );
		uart_puthex( state.NwkSKey, 16 );
		aes_encrypt( state.AppKey, state.NwkSKey );

		KeyDerivation.params.type = 0x02;
		memcpy( state.AppSKey, KeyDerivation.buf, 16 );
		uart_puts( "Data for appskey derivation: " );
		uart_puthex( state.AppSKey, 16 );
		aes_encrypt( state.AppKey, state.AppSKey );
	}

	lorawan_send_join_request();	//Wait for TxDone Interrupt
	while( !dio0_flag )
		;

	TCNT1 = 0;
	dio0_flag = 0;
	dio1_flag = 0;

	lora_prepare_rx_single();

	while( TCNT1 )
		;
	while( TCNT1 < 12590 - 8 * 5 )
		;
//TODO: make this independent of F_CPU

	lora_rx_single();

//TODO: reset TCNT1 and implement soft timeout
	while( 1 ) {

		if( dio0_flag ) {
			dio0_flag = 0;

			uint8_t len = lora_read_register( REG_RX_NB_BYTES );
			uint8_t buf[ len ];

			lora_read_rx( buf, len );
			uart_puthex( buf, len );

			if( lorawan_parse_join_accept( buf, len ) != PARSE_SUCCESS )
				return JOIN_FAILED;

			lorawan_derive_keys();

			uart_puts( "NwkSKey: " );
			uart_puthex( state.NwkSKey, SKEY_LEN );
			uart_puts( "AppSKey: " );
			uart_puthex( state.AppSKey, SKEY_LEN );

			return JOIN_SUCCESS;
		} else if( dio1_flag ) {
			dio1_flag = 0;

			uart_puts( "lorawan_join(): dio1_flag set. RX TIMEOUT\r\n" );
			return JOIN_FAILED;
		}
	}
	return JOIN_SUCCESS;
}

void lorawan_uplink( uint8_t * msg, uint8_t msg_len ) {
	state.FCntUp++;
	lora_write_register( REG_IRQ_FLAGS, 0xFF );

	auto void lorawan_prepare_uplink();
	auto void lorawan_mic_uplink();
	auto void lorawan_encrypt_uplink();

	const uint8_t Uplink = 1;
	uint8_t Dir = ( Uplink ) ? 0 : 1;

	uint8_t payload_len = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN + msg_len + MIC_LEN;
	uint8_t payload[ payload_len ];
	memset(payload, 0x00, payload_len);

	void lorawan_mic_uplink() {
		uint8_t len = payload_len - MIC_LEN;

		uint8_t b_0[ 16 + len ];

		memcpy( b_0, (uint8_t[ ] ) { 0x49, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, state.FCntUp, 0x00, 0x00, 0x00, 0x00, len }, 16 );
		memcpy( b_0 + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
		//memcpy( b_0 + 10, &state.FCntUp, 4 );

		uart_puts("B_0: ");
		uart_puthex(b_0, 16 );

		memcpy( b_0 + 16, payload, len );
		cmac_gen( state.NwkSKey, b_0, 16 + len, payload + len );

		uart_puts("B_0 after cmac_gen: ");
		uart_puthex(b_0, 16 + len );

		uart_puts( "Payload after mic: " );
		uart_puthex( payload, payload_len );
	}

	void lorawan_encrypt_uplink() {
		uint8_t len = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN;
		uint8_t pos = 0;
		uint8_t i = 1;

		uint8_t a_i[ 16 ];
		memcpy( a_i, (uint8_t[ ] ) { 0x01, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, state.FCntUp, 0x00, 0x00, 0x00, 0x00, 0x01 }, 16 );
		memcpy( a_i + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
		//memcpy( a_i + 10, &state.FCntUp, 4 );

		uart_puts( "A_i: " );
		uart_puthex( a_i, 16 );
		//
		aes_encrypt( state.AppSKey, a_i );

		for( uint8_t b = 0; b < 16; b++ ) {
			payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN + b ] = payload[ MHDR_LEN
			        + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN + b ] ^ a_i[ b ];
		}
		uart_puts( "Payload after encrypt: " );
		uart_puthex( payload, payload_len );
		return;
		//

		uint8_t n = ( msg_len + 15 ) / 16;
		for( i = 1; i <= n; i++ ) {
			uint8_t max = 16;
			if( i == n )
				max = ( ( msg_len % 16 ) == 0 ) ? 16 : ( msg_len % 16 );

			uint8_t s_i[ 16 ];
			a_i[ 15 ] = i;
			memcpy( s_i, a_i, 16 );
			aes_encrypt( state.AppSKey, s_i );
			for( uint8_t b = 0; b < max; b++ )
				payload[ len + b + pos ] = payload[ len + b + pos ] ^ s_i[ b ];
			pos += max;
		}

	}

	void lorawan_prepare_uplink() {
		payload[ 0 ] = lorawan_mhdr( UNCONFIRMED_DATA_UP );
		memcpy( payload + MHDR_LEN, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
		payload[ MHDR_LEN + DEV_ADDR_LEN ] = lorawan_fctrl( 0, 0, 0, state.MACCommand.FOptsLen );
		payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN ] = state.FCntUp & 0xFF;
		payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + 1 ] = ( state.FCntUp >> 8 ) & 0xFF;
		if( state.MACCommand.FOptsLen )
			memcpy( payload + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN, state.MACCommand.FOpts, state.MACCommand.FOptsLen );
		payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen ] = 1;
		memcpy( payload + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN, msg, msg_len );

		uart_puts( "Payload after prepare: " );
		uart_puthex( payload, payload_len );
	}

	lorawan_prepare_uplink();
	lorawan_encrypt_uplink();
	lorawan_mic_uplink();

	uart_puts( "Payload: " );
	uart_puthex( payload, payload_len );
	uart_puts( "Payload_len: " );
	uart_putint( payload_len, 10 );
	uart_putln();

	dio0_flag = 0;
	lora_putd( payload, payload_len );
	while( !dio0_flag )
		;

}

