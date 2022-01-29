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
		while( TCNT1 < TIMER1_1SEC - TIMER_256uS )
			;
		TCNT1 = 0;
	}

	if( lora_getd( packet, &len ) == GETD_SUCCESS ) {
		uart_puts( "Packet: " );
		uart_puthex( packet, len );

		uint8_t MHDR = packet[ 0 ];

		switch( ( MHDR >> 5 ) ) {
			case JOIN_ACCEPT:
				if( lorawan_join_parse_accept( packet, len ) != PARSE_SUCCESS )
					return DOWNLINK_FAILED;

				lorawan_join_derive_keys();
				break;

			case UNCONFIRMED_DATA_DOWN:
				lorawan_downlink_unconfirmed(packet, len);
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

uint8_t lorawan_downlink_unconfirmed( uint8_t * packet, uint8_t len ) {

	uint8_t FCtrl = packet[ MHDR_LEN + DEV_ADDR_LEN ];
	uint8_t FOptsLen = FCtrl & 0b1111;

	uint8_t * FOpts = packet + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN;

	lorawan_mac_carrige( FOpts, FOptsLen, state.MACCommand.FOpts, &state.MACCommand.FOptsLen );

	//Is callback is not defined, no point to preparing and decrypting data.
	if( lora_downlink_event_callback ) {

		//No FPORT nor FRMPayload present if true. FPORT is not present, if FRMPayload is empty.
		if( MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + FOptsLen + MIC_LEN == len ) {
			return DOWNLINK_NO_DATA;
		}

		uint8_t FRMPayload_off = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + FOptsLen + FPORT_LEN;
		uint8_t FRMPayload_len = len - MIC_LEN - FRMPayload_off;

		//Check if payload length is not zero, just in case.
		if( FRMPayload_len == 0 )
			return DOWNLINK_NO_DATA;

		uint8_t * FRMPayload = packet + FRMPayload_off;
		uint8_t FCntDown = packet[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN ];
		uint8_t FPort = packet[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + FOptsLen ];

		lorawan_encrypt_FRMPayload( state.AppSKey, DOWNLINK_DIRECTION, FCntDown, FRMPayload, FRMPayload_len );

		lora_downlink_event_callback( FPort, FRMPayload, FRMPayload_len );
	}
	return 1;
}

