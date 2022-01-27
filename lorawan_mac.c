#include "lorawan_mac.h"

#include "MK_USART/mkuart.h"

const uint8_t MACCommands_len[ ][ 2 ] PROGMEM = { { 0, 0x00 }, //0x00
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

MACCommand_handler handlers[ 14 ] = { 0, 0, 0, LinkADRReq, 0, 0, DevStatusReq, 0, 0, 0, 0, 0, 0, 0 };

//TODO: BE AWARE HERE. THIS MAC SHOULD BE ADDED FOR ALL UPLINKS UNTIL NODE RECEIVE DOWNLINK!!!
uint8_t RXTimingSetupReq( uint8_t * payload, uint8_t * answer ) {

	uint8_t Settings = payload[ 0 ];
	uint8_t Del = Settings & 0b1111;

	uint8_t Delay = ( Del == 0 ) ? 1 : Del;

	state.rx1.delay_sec = Delay;

	return 0;
}

//TODO: BE AWARE HERE. THIS MAC SHOULD BE ADDED FOR ALL UPLINKS UNTIL NODE RECEIVE DOWNLINK!!!
uint8_t DlChannelReq( uint8_t * payload, uint8_t * answer ) {
	uint8_t ChIndex = payload[ 0 ];
	uint32_t Frequency = ( (uint32_t )payload[ 3 ] << 16 ) | ( (uint32_t )payload[ 2 ] << 8 ) | ( (uint32_t )payload[ 1 ] );
	Frequency *= 100;

	answer[ 0 ] = 0;
	return 1;
}

uint8_t NewChannelReq( uint8_t * payload, uint8_t * answer ) {

	uint8_t ChIndex = payload[ 0 ];
	uint32_t Frequency = ( (uint32_t )payload[ 3 ] << 16 ) | ( (uint32_t )payload[ 2 ] << 8 ) | ( (uint32_t )payload[ 1 ] );
	Frequency *= 100;

	uint8_t DrRange = payload[ 4 ];
	uint8_t MaxDR = ( DrRange >> 4 );
	uint8_t MinDR = ( DrRange & 0b1111 );

	answer[ 0 ] = 0;

	return 1;
}

uint8_t DutyCycleReq( uint8_t * payload, uint8_t * answer ) {

	uint8_t DutyCyclePL = payload[ 0 ];
	uint8_t MaxDCycle = DutyCyclePL & 0b1111;

	float aggregated = 1 / ( 1 << MaxDCycle );
	return 0;
}

uint8_t LinkADRReq( uint8_t * payload, uint8_t * answer ) {

	answer[0] = 0b111;

	uint8_t DataRate_TXPower = payload[ 0 ];
	uint16_t ChMask = ( (uint16_t )payload[ 1 ] << 8 ) | payload[ 2 ];
	uint8_t Redundancy = payload[ 3 ];

	uint8_t DataRate = (DataRate_TXPower >> 4);
	uint8_t TXPower = (DataRate_TXPower & 0b1111);

	uint8_t ChMaskCntl = (Redundancy >> 4) & 0b111;
	uint8_t NbTrans = (Redundancy & 0b1111 );

	state.rx1.settings.sf = DataRates[DataRate][0];
	state.rx1.settings.bw = DataRates[DataRate][1];

	if( TXPower < 8 ) lora_set_tx_power( MAX_EIRP - 2*TXPower );
	else answer[0] &= ~(1<<2);

	uart_puts("TXPower: ");
	uart_putint(TXPower, 10);
	uart_puts("\r\nDataRate: ");
	uart_putint(DataRate, 10);
	uart_puts("\r\nChMastCntl: ");
	uart_putint(ChMaskCntl, 10);
	uart_putln();
	uart_puts("LinkADRReq payload: ");
	uart_puthex(payload, 4);
	uart_putln();

	return 1;
}

uint8_t RXParamSetupReq( uint8_t * payload, uint8_t * answer ) {

	uint8_t DLSettings = payload[ 0 ];
	uint32_t Frequency = ( (uint32_t )payload[ 3 ] << 16 ) | ( (uint32_t )payload[ 2 ] << 8 ) | ( (uint32_t )payload[ 1 ] );
	Frequency *= 100;

	uint8_t RX1DRoffset = ( DLSettings >> 4 ) & 0b111;
	uint8_t RX2DataRate = ( DLSettings ) & 0b1111;

	answer[ 0 ] = 0b111; //Temporary write that this device is dumb.

	return 1;
}

uint8_t DevStatusReq( uint8_t * payload, uint8_t * answer ) {
	int8_t snr = lora_get_snr();

	uint8_t Battery = BATTERY_EXTERNAL_POWER_SOURCE;
	uint8_t Margin = ( ( ( snr < 0 ) ? 1 : 0 ) << 6 ) | ( snr & 0b11111 );

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
				uint8_t bytes = handlers[ CID ]( fopts + wsk + 1, answer + cnt + 1 );
				cnt += ( bytes + 1 );
			}
		}

		wsk += pgm_read_byte(&(MACCommands_len[CID][1])) + 1;
	}
	*answer_len = cnt;
	return 1;
}
