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
#include "lorawan_join.h"
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


#define TIMER1_1SEC (F_CPU/1024)
#define TIMER_256uS (F_CPU/1000000/4)

#define TIMER_RESET TCNT1 = 0


#define DEV_NONCE_LEN 2

#define EUI_LEN 8
#define APP_EUI_LEN EUI_LEN
#define DEV_EUI_LEN EUI_LEN

#define CMAC_LEN 16
#define MIC_LEN 4

#define SKEY_LEN 16
#define APPSKEY_LEN SKEY_LEN
#define NWKSKEY_LEN SKEY_LEN

#define APP_KEY_LEN 16

#define JOIN_REQUEST_LEN (MHDR_LEN + APP_EUI_LEN + DEV_EUI_LEN + DEV_NONCE_LEN + MIC_LEN)
#define JOIN_REQUEST_MIC_OFFSET (MHDR_LEN + APP_EUI_LEN + DEV_EUI_LEN + DEV_NONCE_LEN)
#define JOIN_REQUEST_DEV_NONCE_OFFSET (MHDR_LEN + APP_EUI_LEN + DEV_EUI_LEN)
#define JOIN_REQUEST_DEV_EUI_OFFSET (MHDR_LEN + APP_EUI_LEN)
#define JOIN_REQUEST_APP_EUI_OFFSET (MHDR_LEN)

#define AES_BLOCK_LEN 16
#define TYPE_LEN 1
#define KEY_DERIVATION_LEN (TYPE_LEN + APP_NONCE_LEN  + NET_ID_LEN + DEV_NONCE_LEN + 7)
#define KEY_DERIVATION_APP_NONCE_OFFSET (TYPE_LEN)
#define KEY_DERIVATION_NET_ID_OFFSET (TYPE_LEN + APP_NONCE_LEN)
#define KEY_DERIVATION_DEV_NONCE_OFFSET (TYPE_LEN + APP_NONCE_LEN + NET_ID_LEN)

#define APP_NONCE_LEN 3
#define NET_ID_LEN 3
#define DEV_ADDR_LEN 4

#define MHDR_LEN 1
#define FCTRL_LEN 1
#define FPORT_LEN 1
#define FCNT_LEN 2

#define DATARATE_LEN 7

#define MAX_EIRP 16
#define SX1276_MAX_TX_POWER 20

#define TTN_LORA_CHANNELS_LEN 8

enum {
	JOIN_SUCCESS = 1, JOIN_FAILED
} JOIN_STATUS;
enum {
	INIT_SUCCESS = 1, INIT_FAILED
} INIT_STATUS;

enum {
	PARSE_SUCCESS, PARSE_FAILED
} PARSE_STATUS;

enum {
	DOWNLINK_SUCCESS, DOWNLINK_FAILED, DOWNLINK_NO_DATA
} DOWNLINK_STATS;

enum {
	UPLINK_DIRECTION = 0, DOWNLINK_DIRECTION = 1
} DIRECTION;

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
		RadioSettings_t settings;
		uint8_t delay_sec;
} Window_t;

typedef struct {
		uint32_t main_channels[3];
		uint32_t channels[16];
		uint8_t len;
} Channels_t;

typedef struct {
	uint16_t DevNonce;
	uint32_t FCntUp;
	uint32_t FCntDown;

	uint8_t AppEUI[ APP_EUI_LEN ];
	uint8_t DevEUI[ DEV_EUI_LEN ];

	uint8_t AppKey[ APP_KEY_LEN ];
	uint8_t AppSKey[ APPSKEY_LEN ];
	uint8_t NwkSKey[ NWKSKEY_LEN ];

	JoinAccept_t JoinAccept;
	MACCommand_t MACCommand;

	Window_t rx1;
	Channels_t Channels;

} State_t;

extern State_t state;

extern const uint8_t DataRates[DATARATE_LEN][2] PROGMEM;

void (*lora_downlink_event_callback)( uint8_t fport, uint8_t * buf, uint8_t len );

uint8_t lorawan_fctrl( uint8_t adr, uint8_t adrackreq, uint8_t ack, uint8_t foptslen );
uint8_t lorawan_mhdr( uint8_t mtype );
void lorawan_encrypt_FRMPayload( uint8_t * key, uint8_t Dir, uint32_t FCnt, uint8_t * payload, uint8_t len );
void lorawan_mic_uplink(uint8_t Dir, uint32_t FCnt, uint8_t * payload, uint8_t len, uint8_t * MIC);

uint8_t lorawan_init();

void register_lorawan_downlink_callback( void (*callback)( uint8_t fport, uint8_t *buf, uint8_t len ) );

#endif /* LORAWAN_H_ */
