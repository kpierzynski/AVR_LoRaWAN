/*
 * cmac.h
 *
 *  Created on: 20 sty 2022
 *      Author: kpier
 */

#ifndef AES_CMAC_H_
#define AES_CMAC_H_

#include <stdint.h>
#include <string.h>

//This function using key, compute mic (note: mic (4byte), not cmac (16bytes)) for given message of msg_len length;
void cmac_gen( uint8_t * key, uint8_t * msg, uint8_t msg_len, uint8_t * mic );

#endif /* AES_CMAC_H_ */
