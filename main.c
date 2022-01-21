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

	while( 1 ) {

	}

	return 0;
}
