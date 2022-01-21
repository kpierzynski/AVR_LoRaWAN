/*
 * random.c
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#include "random.h"

uint16_t random_u16() {
	return ( (uint16_t)random_u8() << 8 ) | random_u8();
}

uint8_t random_u8() {
	uint8_t V = 0b00010110;
	const uint8_t N = 20;

	for( int i = 0; i < N; i++ ) {
		ADCSRA |= ( 1 << ADEN );

		if( i % 2 == 0 )
			ADMUX = 0b01000011;
		else
			ADMUX = 0b01001111;
		ADCSRA |= 1 << ADSC;
		uint8_t low, high;

		while( ( ADCSRA >> ADSC ) % 2 )
			;
		low = ADCL;
		high = ADCH;
		V ^= low;
		V ^= high;

		uint8_t last = V % 2;
		V >>= 1;
		V |= last << 7;

		ADCSRA = 0;

		ADCSRA = 0b10000000 | ( ( V % 4 ) << 1 );
	}
	return V;
}

