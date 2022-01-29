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
		wdt_enable( 0 );// set  watchdog
		while( 1 )
			;           // wait for RESET
	}
}

void parse_lorawan( uint8_t fport, uint8_t * buf, uint8_t len ) {
	uart_puts_P( PSTR( "\r\n\r\nPARSE_LORAWAN: " ) );
	uart_puthex( buf, len );
	uart_puts_P( PSTR( "On fport: " ) );
	uart_putint( fport, 10 );
	uart_putln();
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
	register_lorawan_downlink_callback( parse_lorawan );

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
	uint16_t cnt = 0, led = 0;
	char uart_buf[ 32 ];
	while( 1 ) {
		if( cnt == 18000 ) {
			uint8_t buf[ 3 ] = { 'K', ':', ( cnt % 10 ) + 48 };
			lorawan_uplink( buf, 3 );
			cnt = 0;
		}
		if( led == 100 ) {
			PORTC ^= ( 1 << PC5 );
			led = 0;
		}
		_delay_ms( 10 );
		cnt++;
		led++;
		UART_RX_STR_EVENT( uart_buf );
	}

	return 0;
}
