/*
 * lorawan.h
 *
 *  Created on: 21 sty 2022
 *      Author: kpier
 */

#ifndef LORAWAN_H_
#define LORAWAN_H_

#include <string.h>

#include "lora.h"
#include "lora_mem.h"
#include "lorawan_credentials.h"
#include "random.h"
#include "AES/cmac.h"
#include "AES/qqq_aes.h"

#include "MK_USART/mkuart.h"

#define JOIN_REQUEST 0b000
#define JOIN_ACCEPT 0b001
#define UNCONFIRMED_DATA_UP 0b010
#define UNCONFIRMED_DATA_DOWN 0b011
#define CONFIRMED_DATA_UP 0b100
#define CONFIRMED_DATA_DOWN 0b101

#define EUI_LEN 8
#define APP_EUI_LEN EUI_LEN
#define DEV_EUI_LEN EUI_LEN

#define CMAC_LEN 16
#define MIC_LEN 4

#define SKEY_LEN 16
#define APPSKEY_LEN SKEY_LEN
#define NWKSKEY_LEN SKEY_LEN

#define APP_KEY_LEN 16

#define JOIN_REQUEST_LEN 23

#define APP_NONCE_LEN 3
#define NET_ID_LEN 3
#define DEV_ADDR_LEN 4

#define MHDR_LEN 1
#define FCTRL_LEN 1
#define FPORT_LEN 1
#define FCNT_LEN 2

enum {
	JOIN_SUCCESS = 1, JOIN_FAILED
} JOIN_STATUS;
enum {
	INIT_SUCCESS = 1, INIT_FAILED
} INIT_STATUS;

enum {
	PARSE_SUCCESS, PARSE_FAILED
} PARSE_STATUS;

typedef struct {
	uint8_t AppNonce[ APP_NONCE_LEN ];
	uint8_t NetID[ NET_ID_LEN ];
	uint8_t DevAddr[ DEV_ADDR_LEN ];
} JoinAccept_t;

typedef struct {
	uint8_t FOptsLen;
	uint8_t FOpts[ 16 ];
} MACCommand_t;

typedef struct {
	uint16_t DevNonce;
	uint32_t FCntUp;

	uint8_t AppEUI[ APP_EUI_LEN ];
	uint8_t DevEUI[ DEV_EUI_LEN ];

	uint8_t AppKey[ APP_KEY_LEN ];
	uint8_t AppSKey[ APPSKEY_LEN ];
	uint8_t NwkSKey[ NWKSKEY_LEN ];
	JoinAccept_t JoinAccept;
	MACCommand_t MACCommand;
} State_t;

uint8_t lorawan_init();
uint8_t lorawan_join();
void lorawan_uplink( uint8_t * msg, uint8_t msg_len );

#endif /* LORAWAN_H_ */
