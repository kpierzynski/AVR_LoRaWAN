/*
 * main.c
 *
 *  Created on: 20 sty 2022
 *      Author: kpier
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "AES/cmac.h"
#include "AES/qqq_aes.h"

#include "lorawan.h"
#include "lorawan_join.h"
#include "lorawan_downlink.h"
#include "lorawan_uplink.h"

#include "MK_USART/mkuart.h"

void parse_uart( char * buf ) {
	if( !strncasecmp( "AT+RST?", buf, 7 ) ) {
		cli();
		// disable interrupts
		wdt_enable( 0 );  	// set  watchdog
		while( 1 )
			;           // wait for RESET
	}
}

int main() {

	// WAIT AFTER BOOTLOADER FLASH
	_delay_ms( 5000 );
	// ===========================

	// DEBUG LED
	DDRC |= ( 1 << PC5 );
	PORTC &= ~( 1 << PC5 );
	// =========

	// INIT UART
	USART_Init( __UBRR );
	register_uart_str_rx_event_callback( parse_uart );
	sei();
	// =========

	if( lorawan_init() != JOIN_SUCCESS ) {
		uart_puts_P( PSTR( "Failed to init lorawan module.\r\n" ) );
		while( 1 )
			;
	}

	uart_puts_P( PSTR( "Trying to join.\r\n" ) );
	while( 1 ) {
		if( lorawan_join() == JOIN_SUCCESS ) {
			uart_puts_P( PSTR( "Join success.\r\n" ) );
			break;
		}
		_delay_ms( 10000 );
		uart_puts_P( PSTR( "Join failed. Trying to join.\r\n" ) );
	}

	_delay_ms( 5000 );
	uint8_t cnt = 3;
	uint8_t buf[ 17 ] = { 'K', ':', ( cnt++ % 10 ) + 48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x69 };
	lorawan_uplink( buf, 17 );
	_delay_ms(5000);
	lorawan_uplink( buf, 17 );
	uart_puts_P( PSTR( "Uplink done.\r\n" ) );

	char uart_buf[ 32 ];
	while( 1 ) {
		UART_RX_STR_EVENT( uart_buf );
	}

	return 0;
}
