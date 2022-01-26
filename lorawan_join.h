/*
 * lorawan_join.h
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#ifndef LORAWAN_JOIN_H_
#define LORAWAN_JOIN_H_

#include <avr/io.h>

#include "lora.h"
#include "lorawan.h"
#include "AES/cmac.h"
#include "AES/qqq_aes.h"

#include "lorawan_downlink.h"

uint8_t lorawan_join();
void lorawan_send_join_request();
uint8_t lorawan_parse_join_accept( uint8_t * data, uint8_t data_len );
void lorawan_derive_keys();

#endif /* LORAWAN_JOIN_H_ */
