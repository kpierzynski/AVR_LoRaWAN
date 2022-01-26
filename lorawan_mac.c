#include "lorawan_mac.h"

#include "MK_USART/mkuart.h"

const uint8_t MACCommands_len[ ][ 2 ] PROGMEM = {
		{ 0, 0x00 }, //0x00
        { 0, 0x00 }, //0x01
        { 1, 2 }, //LINKCHECK
        { 1, 4 }, //LINADDR
        { 1, 1 }, //DUTYCYCLE
        { 1, 4 }, //RXPARAMSETUP
        { 1, 0 }, //DEVSTATUS
        { 1, 5 }, //NEWCHANNEL
        { 1, 1 }, //RXTIMINGSETUP
        { 0, 0x00 }, //0x09
        { 1, 4 }, //DICHANNEL
        { 0, 0x00 }, //0x0B
        { 0, 0x00 }, //0x0C
        { 1, 5 }, //DEVICETIME
        };

MACCommand_handler handlers[ 14 ] = { 0, 0, LinkADRReq, 0, 0, RXParamSetupReq, DevStatusReq, 0, 0, 0, 0, 0, 0, 0 };

uint8_t LinkADRReq( uint8_t * payload, uint8_t * answer ) {

	uint8_t DataRate_TXPower = payload[ 0 ];
	uint16_t ChMask = ( (uint16_t )payload[ 1 ] << 8 ) | payload[ 2 ];
	uint8_t Redundancy = payload[ 3 ];

	answer[ 0 ] = 0;
	return 1;
}

uint8_t RXParamSetupReq( uint8_t * payload, uint8_t * answer ) {
	uart_puts( "RXParamSetupReq\r\n" );

	uint8_t DLSettings = payload[ 0 ];
	uint32_t Frequency = ( (uint32_t )payload[ 3 ] << 16 ) | ( (uint32_t )payload[ 2 ] << 8 ) | ( (uint32_t )payload[ 1 ] );
	Frequency *= 100;

	uint8_t RX1DRoffset = ( DLSettings >> 4 ) & 0b111;
	uint8_t RX2DataRate = ( DLSettings ) & 0b1111;

	uart_putln();
	uart_puts_P( PSTR( "Frequency: " ) );
	uart_puthex( payload + 1, 3 );
	uart_putln();
	uart_puts_P( PSTR( "DLSettings: " ) );
	uart_putint( DLSettings, 16 );
	uart_putln();

	answer[ 0 ] = 0b111; //Temporary write that this device is dumb.

	return 1;
}

uint8_t DevStatusReq( uint8_t * payload, uint8_t * answer ) {
	uart_puts( "DevStatusReq\r\n" );
	int8_t snr = lora_get_snr();

	uint8_t Battery = BATTERY_EXTERNAL_POWER_SOURCE;
	uint8_t Margin = ( ( ( snr < 0 ) ? 1 : 0 ) << 6 ) | ( snr & 0b11111 );

	uart_puts( "SNR: " );
	uart_putint( snr, 10 );
	uart_puts( "Margin: " );
	uart_putint( Margin, 10 );

	answer[ 0 ] = Battery;
	answer[ 1 ] = Margin;

	return 2;
}

uint8_t lorawan_mac_carrige( uint8_t * fopts, uint8_t foptslen, uint8_t * answer, uint8_t * answer_len ) {

	uint8_t cnt = 0;
	uint8_t wsk = 0;

	while( wsk != foptslen ) {
		uint8_t CID = *( fopts + wsk );
		if( pgm_read_byte(&(MACCommands_len[CID][0])) != 1 )
			return 0;
		else {
			answer[ cnt ] = CID;
			if( handlers[ CID ] ) {
				uint8_t bytes = handlers[ CID ]( fopts + cnt, answer + cnt + 1 );
				cnt += ( bytes + 1 );
			}
		}

		wsk += pgm_read_byte(&(MACCommands_len[CID][1])) + 1;
	}
	*answer_len = cnt;
	return 1;
}

void lorawan_parse_mac_payload( uint8_t * cmd, uint8_t cmd_len ) {
	uart_puts( "lorawan_parse_mac_payload\r\n" );
	union {
		struct s {
			uint8_t MHDR;
			uint8_t DevAddr[ DEV_ADDR_LEN ];
			uint8_t FCtrl;
			uint16_t FCntDown;
		} params;
		uint8_t buf[ sizeof(struct s) ];
	} MACCommand;

	const uint8_t offset = DEV_ADDR_LEN + 1 + 1 + 2;

	memcpy( MACCommand.buf, cmd, offset );

	if( ( ( MACCommand.params.MHDR >> 5 ) & 0b111 ) != UNCONFIRMED_DATA_DOWN )
		return;

	if( memcmp( MACCommand.params.DevAddr, state.JoinAccept.DevAddr, DEV_ADDR_LEN ) )
		return;

	uint8_t * FOpts = cmd + offset;
	uint8_t FOptsLen = ( MACCommand.params.FCtrl & 0b1111 );

	if( FOptsLen == 0 )
		return;
	//uint8_t answer[16];
	//uint8_t answer_len = 0;
	lorawan_mac_carrige( FOpts, FOptsLen, state.MACCommand.FOpts, &state.MACCommand.FOptsLen );

	uart_puts( "\r\nAnswer for MAC commands: " );
	uart_puthex( state.MACCommand.FOpts, state.MACCommand.FOptsLen );
	uart_puts( "answer_len: " );
	uart_putint( state.MACCommand.FOptsLen, 10 );
	uart_putln();
	//lorawan_send_mac(answer, answer_len);
}
