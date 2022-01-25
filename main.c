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

#include "AES/cmac.h"
#include "AES/qqq_aes.h"

#include "lorawan.h"

#include "MK_USART/mkuart.h"

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

	_delay_ms(15000);
	uint8_t cnt = 3;
	uint8_t buf[ 16 ] = { 'K', ':', ( cnt++ % 10 ) + 48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	lorawan_uplink( buf, 16 );
	uart_puts_P(PSTR("Uplink done.\r\n"));
	while( 1 ) {

	}

	return 0;
}
