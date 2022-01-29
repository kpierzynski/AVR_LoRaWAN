/*
 * lorawan_uplink.c
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#include "lorawan_uplink.h"

void lorawan_uplink( uint8_t * msg, uint8_t msg_len ) {

	uint8_t payload_len = MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN + msg_len + MIC_LEN;
	uint8_t payload[ payload_len ];
	memset( payload, 0x00, payload_len );

	payload[ 0 ] = lorawan_mhdr( UNCONFIRMED_DATA_UP );
	memcpy( payload + MHDR_LEN, state.JoinAccept.DevAddr, DEV_ADDR_LEN );
	payload[ MHDR_LEN + DEV_ADDR_LEN ] = lorawan_fctrl( 0, 0, 0, state.MACCommand.FOptsLen );
	payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN ] = state.FCntUp & 0xFF;
	payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + 1 ] = ( state.FCntUp >> 8 ) & 0xFF;

	if( state.MACCommand.FOptsLen )
		memcpy( payload + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN, state.MACCommand.FOpts, state.MACCommand.FOptsLen );
	payload[ MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen ] = 1;
	memcpy( payload + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN + state.MACCommand.FOptsLen + FPORT_LEN, msg, msg_len );

	lorawan_encrypt_FRMPayload( state.AppSKey, UPLINK_DIRECTION, state.FCntUp, payload + MHDR_LEN + DEV_ADDR_LEN + FCTRL_LEN + FCNT_LEN
	        + state.MACCommand.FOptsLen + FPORT_LEN, msg_len );
	lorawan_mic_uplink( UPLINK_DIRECTION, state.FCntUp, payload, payload_len - MIC_LEN, payload + payload_len - MIC_LEN );

	dio0_flag = 0;
	uart_puts( "Payload: " );
	uart_puthex( payload, payload_len );

	uart_puts( "Mac answer: " );
	uart_puthex( state.MACCommand.FOpts, state.MACCommand.FOptsLen );

	lora_putd( payload, payload_len );
	TCNT1 = 0;
	state.FCntUp++;
	lorawan_downlink( state.rx1.delay_sec );
}

