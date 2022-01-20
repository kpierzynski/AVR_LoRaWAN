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

#include "MK_USART/mkuart.h"

int main() {

	_delay_ms( 5000 );

	DDRC |= ( 1 << PC5 );
	PORTC &= ~( 1 << PC5 );

	USART_Init( __UBRR );

	sei();
	uart_puts_P( PSTR( "Starting...\r\n" ) );

	uint8_t key[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
	uint8_t buf[16] = "0123456789ABCDEF";

	uint8_t cmac[16];
	cmac_gen( key, buf, 16, cmac );

	uart_puthex( buf, 16 );
	uart_puthex( cmac, 16 );

	while( 1 ) {

	}

	return 0;
}
