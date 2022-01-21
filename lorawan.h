/*
 * lorawan.h
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#ifndef LORAWAN_H_
#define LORAWAN_H_

#include "lora.h"
#include "random.h"

enum { JOIN_SUCCESS = 1, JOIN_FAILED } JOIN_STATUS;

typedef struct {
	uint16_t DevNonce;
} State_t;

#endif /* LORAWAN_H_ */
