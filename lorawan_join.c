/*
 * lorawan_join.c
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#include "lorawan_join.h"

uint8_t lorawan_join() {

	lorawan_join_send_request();

	TIMER_RESET;

	if( lorawan_downlink( JOIN_ACCEPT_DELAY1 ) != DOWNLINK_SUCCESS )
		return JOIN_FAILED;

	return JOIN_SUCCESS;
}

void lorawan_join_send_request() {

	state.DevNonce = random_u16();

	uint8_t JoinRequest[ JOIN_REQUEST_LEN ];

	JoinRequest[0] = lorawan_mhdr( JOIN_REQUEST );
	memcpy( JoinRequest + JOIN_REQUEST_APP_EUI_OFFSET, state.AppEUI, APP_EUI_LEN );
	memcpy( JoinRequest + JOIN_REQUEST_DEV_EUI_OFFSET, state.DevEUI, DEV_EUI_LEN );
	memcpy( JoinRequest + JOIN_REQUEST_DEV_NONCE_OFFSET, &state.DevNonce, DEV_NONCE_LEN);

	cmac_gen( state.AppKey, JoinRequest, 19, JoinRequest + JOIN_REQUEST_MIC_OFFSET );

	uart_puts("JOIN_REQUEST: ");
	uart_puthex(JoinRequest, JOIN_REQUEST_LEN);
	lora_putd( JoinRequest, JOIN_REQUEST_LEN );
}

void lorawan_join_derive_keys() {

	uint8_t KeyDerivation2[ KEY_DERIVATION_LEN ] = {0};
	memcpy( KeyDerivation2 + KEY_DERIVATION_APP_NONCE_OFFSET, state.JoinAccept.AppNonce, APP_NONCE_LEN );
	memcpy( KeyDerivation2 + KEY_DERIVATION_NET_ID_OFFSET, state.JoinAccept.NetID, NET_ID_LEN );
	memcpy( KeyDerivation2 + KEY_DERIVATION_DEV_NONCE_OFFSET, &state.DevNonce, DEV_NONCE_LEN);

	KeyDerivation2[0] = 0x01;
	memcpy( state.NwkSKey, KeyDerivation2, AES_BLOCK_LEN );
	aes_encrypt( state.AppKey, state.NwkSKey );

	KeyDerivation2[0] = 0x02;
	memcpy( state.AppSKey, KeyDerivation2, AES_BLOCK_LEN );
	aes_encrypt( state.AppKey, state.AppSKey );

	uart_puts( "NWSKEY: " );
	uart_puthex( state.NwkSKey, 16 );
	uart_puts( "APPSKEY: " );
	uart_puthex( state.AppSKey, 16 );

	uart_puts( "DevNonce: " );
	uart_puthex( (uint8_t* )&state.DevNonce, 2 );
	uart_putln();
}

uint8_t lorawan_join_parse_accept( uint8_t* data, uint8_t data_len ) {

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

	if( JoinAccept.params.CFList[ 15 ] == 0 ) {
		uint8_t * list = JoinAccept.params.CFList;
		for( uint8_t i = 0; i < 5; i++ ) {
			uint32_t Frequency = ( (uint32_t )list[ 3 * i + 2 ] << 16 ) | ( (uint32_t )list[ 3 * i + 1 ] << 8 ) | ( list[ 3 * i ] );
			if( Frequency == 0 )
				continue;

			Frequency *= 100;
			state.Channels.channels[ state.Channels.len++ ] = Frequency;
		}
	}

	memcpy( state.JoinAccept.AppNonce, JoinAccept.params.AppNonce, APP_NONCE_LEN );
	memcpy( state.JoinAccept.NetID, JoinAccept.params.NetID, NET_ID_LEN );
	memcpy( state.JoinAccept.DevAddr, JoinAccept.params.DevAddr, DEV_ADDR_LEN );

	return PARSE_SUCCESS;
}
