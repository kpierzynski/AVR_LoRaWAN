/*
 * lorawan_join.c
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#include "lorawan_join.h"

uint8_t lorawan_join() {

	lorawan_send_join_request();
	TCNT1 = 0;
	lorawan_downlink( 5 );

	return JOIN_SUCCESS;
}

void lorawan_send_join_request() {

	state.DevNonce = random_u16();

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

	JoinRequest.params.DevNonce = ( state.DevNonce >> 8 ) & 0xFF;
	JoinRequest.params.DevNonce |= ( state.DevNonce << 8 );
	//Todo: ??? wtf is this

	JoinRequest.params.MHDR = lorawan_mhdr( JOIN_REQUEST );
	memcpy( JoinRequest.params.DevEUI, state.DevEUI, DEV_EUI_LEN );
	memcpy( JoinRequest.params.AppEUI, state.AppEUI, APP_EUI_LEN );

	cmac_gen( state.AppKey, JoinRequest.buf, 19, JoinRequest.params.MIC );

	lora_putd( JoinRequest.buf, JOIN_REQUEST_LEN );
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

	//TODO: why i had to swap bytes here
	KeyDerivation.params.DevNonce = ( state.DevNonce >> 8 ) & 0xFF;
	KeyDerivation.params.DevNonce |= ( state.DevNonce << 8 );

	KeyDerivation.params.type = 0x01;
	memcpy( state.NwkSKey, KeyDerivation.buf, 16 );
	aes_encrypt( state.AppKey, state.NwkSKey );

	KeyDerivation.params.type = 0x02;
	memcpy( state.AppSKey, KeyDerivation.buf, 16 );
	aes_encrypt( state.AppKey, state.AppSKey );

	uart_puts("NWSKEY: ");
	uart_puthex(state.NwkSKey, 16);
	uart_puts("APPSKEY: ");
	uart_puthex(state.AppSKey, 16);

	uart_puts("DevNonce: ");
	uart_puthex( (uint8_t*)&state.DevNonce, 2 );
	uart_putln();

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
