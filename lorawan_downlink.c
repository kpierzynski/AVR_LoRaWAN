/*
 * lorawan_downlink.c
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#include "lorawan_downlink.h"

uint8_t lorawan_downlink( uint8_t delay_sec ) {
	lora_prepare_rx_single();

	dio0_flag = 0;
	dio1_flag = 0;

	uint8_t len;
	uint8_t packet[ 64 ];
	register uint8_t sec;

	for( sec = 0; sec < delay_sec; sec++ ) {
		while( TCNT1 < 15625 - 4 )
			;
		TCNT1 = 0;
	}

	if( lora_getd( packet, &len ) == GETD_SUCCESS ) {
		state.FCntDown++;
		uart_puthex( packet, len );

		uint8_t MHDR = packet[ 0 ];

		switch( ( MHDR >> 5 ) ){
			case JOIN_ACCEPT:
				if( lorawan_parse_join_accept( packet, len ) != PARSE_SUCCESS )
					return DOWNLINK_FAILED;

				lorawan_derive_keys();

				uart_puts( "NwkSKey: " );
				uart_puthex( state.NwkSKey, SKEY_LEN );
				uart_puts( "AppSKey: " );
				uart_puthex( state.AppSKey, SKEY_LEN );
				break;

			case UNCONFIRMED_DATA_DOWN:
				uart_puts( "UNCONFIRMED_DATA_DOWN\r\n" );
				uart_puthex( packet, len );

				lorawan_parse_mac_payload( packet, len );

				//if( lora_downlink_event_callback ) {
					//get pointer to frmpayload
					//check if any data
					//check if fport > 0
					//decrypt payload
					//run callback
					uint8_t FCtrl = packet[ MHDR_LEN + DEV_ADDR_LEN ];
					uint8_t FOptsLen = FCtrl & 0b1111;

					uint8_t FRMPayload_off = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + FOptsLen + FPORT_LEN;
					uint8_t FRMPayload_len = len - MIC_LEN - FRMPayload_off;

					uint8_t * FRMPayload = packet + FRMPayload_off;
					if( FRMPayload_len == 0 )
						break;

					const uint8_t Uplink = 0;
					uint8_t Dir = ( Uplink ) ? 0 : 1;

					uint8_t i = 1;

					uint8_t a_i[ 16 ];
					memcpy( a_i, (uint8_t[ ] ) { 0x01, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, state.FCntDown, 0x00, 0x00,
							        0x00, 0x00, 0x01 }, 16 );
					memcpy( a_i + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );

					uart_puts( "A_i: " );
					uart_puthex( a_i, 16 );

					aes_encrypt( state.AppSKey, a_i );

					for( uint8_t b = 0; b < 16; b++ ) {
						FRMPayload[ b ] = FRMPayload[ b ] ^ a_i[ b ];
					}
					uart_puts( "FRMPayload after encrypt: " );
					uart_puthex( FRMPayload, FRMPayload_len );

				//}
				break;

			default:
				uart_puts( "Unknown payload MHDR.\r\n" );
				return DOWNLINK_FAILED;
		}

	} else {
		uart_puts( "Timeout.\r\n" );
		return DOWNLINK_FAILED;
	}
	return DOWNLINK_SUCCESS;
}

