/*
 * cmac.c
 *
 *  Created on: 20 sty 2022
 *      Author: kpier
 */

#include "cmac.h"
#include "qqq_aes.h"

static void cmac_padding( uint8_t * input, uint8_t input_len, uint8_t * output ) {
	uint8_t i = 0;

	for( i = 0; i < 16; i++ ) {
		if( i < input_len )
			output[i] = input[i];
		else if( i == input_len )
			output[i] = 0x80;
		else
			output[i] = 0x00;
	}
}

static void cmac_left_shift( uint8_t * input, uint8_t * output ) {
	uint8_t i;
	uint8_t overflow = 0;

	for( i = 16; i > 0; i-- ) {
		output[i - 1] = ( input[i - 1] << 1 );
		output[i - 1] |= overflow;
		overflow = ( input[i - 1] & 0x80 ) ? 1 : 0;
	}
}

static void cmac_xor( uint8_t * a, uint8_t * b, uint8_t * output ) {
	uint8_t i;

	for( i = 0; i < 16; i++ ) {
		output[i] = a[i] ^ b[i];
	}
}

static void cmac_generate_subkey( uint8_t * key, uint8_t * K1, uint8_t * K2 ) {
	uint8_t L[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t const_Rb[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87 };

	aes_encrypt( key, L );

	if( ( L[0] & 0x80 ) == 0 ) {
		cmac_left_shift( L, K1 );
	} else {
		cmac_left_shift( L, K1 );
		cmac_xor( K1, const_Rb, K1 );
	}

	if( ( K1[0] & 0x80 ) == 0 ) {
		cmac_left_shift( K1, K2 );
	} else {
		cmac_left_shift( K1, K2 );
		cmac_xor( K2, const_Rb, K2 );
	}
}

void cmac_gen( uint8_t * key, uint8_t * msg, uint8_t msg_len, uint8_t * cmac ) {
	uint8_t K1[16], K2[16], M_last[16], tmp[16], X[16], Y[16];
	uint8_t flag, i;

	//Step 2:
	uint8_t n = ( msg_len + ( 16 - 1 ) ) / 16;

	//Step 1:
	cmac_generate_subkey( key, K1, K2 );

	//Step 3:
	if( n == 0 ) {
		n = 1;
		flag = 0;
	} else {
		if( ( msg_len % 16 ) == 0 )
			flag = 1;
		else
			flag = 0;
	}

	//Step 4:
	if( flag ) {
		cmac_xor( msg + ( 16 * ( n - 1 ) ), K1, M_last );
	} else {
		cmac_padding( msg + ( 16 * ( n - 1 ) ), msg_len % 16, tmp );
		cmac_xor( tmp, K2, M_last );
	}

	//Step 5:
	memset( X, 0, 16 );

	//Step 6:
	for( i = 1; i < n - 1; i++ ) {
		cmac_xor( X, msg + ( 16 * i ), Y );
		aes_encrypt( key, Y );
		memcpy( X, Y, 16 );
	}
	cmac_xor( M_last, X, Y );
	aes_encrypt( key, Y );
	memcpy( cmac, Y, 16 );
}
