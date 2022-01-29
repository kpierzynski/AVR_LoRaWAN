/*
 * lorawan_downlink.h
 *
 *  Created on: 26 sty 2022
 *      Author: kpier
 */

#ifndef LORAWAN_DOWNLINK_H_
#define LORAWAN_DOWNLINK_H_

#include <avr/io.h>

#include "lora.h"
#include "lorawan.h"
#include "lorawan_mac.h"

uint8_t lorawan_downlink( uint8_t delay_sec );
uint8_t lorawan_downlink_unconfirmed( uint8_t * packet, uint8_t len );

#endif /* LORAWAN_DOWNLINK_H_ */
