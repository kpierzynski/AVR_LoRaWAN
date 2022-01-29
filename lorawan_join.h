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

#define JOIN_ACCEPT_DELAY1 5

uint8_t lorawan_join();
void lorawan_join_send_request();
uint8_t lorawan_join_parse_accept( uint8_t * data, uint8_t data_len );
void lorawan_join_derive_keys();

#endif /* LORAWAN_JOIN_H_ */
