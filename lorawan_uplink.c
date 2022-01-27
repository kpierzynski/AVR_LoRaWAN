/*
 * lorawan_uplink.c
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#include "lorawan_uplink.h"


void lorawan_uplink( uint8_t * msg, uint8_t msg_len ) {
	auto void lorawan_prepare_uplink();
	auto void lorawan_mic_uplink();
	auto void lorawan_encrypt_uplink();

	const uint8_t Uplink = 1;
	uint8_t Dir = ( Uplink ) ? 0 : 1;

	uint8_t payload_len = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN + msg_len + MIC_LEN;
	uint8_t payload[ payload_len ];
	memset( payload, 0x00, payload_len );

	void lorawan_mic_uplink() {
		uint8_t len = payload_len - MIC_LEN;

		uint8_t b_0[ 16 + len ];

		memcpy( b_0, (uint8_t[ ] ) { 0x49, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, state.FCntUp, 0x00, 0x00, 0x00, 0x00, len }, 16 );
		memcpy( b_0 + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
		//memcpy( b_0 + 10, &state.FCntUp, 4 );

		memcpy( b_0 + 16, payload, len );
		cmac_gen( state.NwkSKey, b_0, 16 + len, payload + len );
	}

	void lorawan_encrypt_uplink() {
		uint8_t len = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN;
		uint8_t pos = 0;
		uint8_t i = 1;

		uint8_t a_i[ 16 ];
		memcpy( a_i, (uint8_t[ ] ) { 0x01, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, state.FCntUp, 0x00, 0x00, 0x00, 0x00, 0x01 }, 16 );
		memcpy( a_i + 6, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
		//memcpy( a_i + 10, &state.FCntUp, 4 );

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
	}

	lorawan_prepare_uplink();
	lorawan_encrypt_uplink();
	lorawan_mic_uplink();

	dio0_flag = 0;
	uart_puts("Payload: ");
	uart_puthex(payload, payload_len);

	uart_puts("Mac answer: ");
	uart_puthex(state.MACCommand.FOpts,state.MACCommand.FOptsLen );

	lora_putd( payload, payload_len );
	TCNT1 = 0;
	state.FCntUp++;
	lorawan_downlink(state.rx1.delay_sec);

}

