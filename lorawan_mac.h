#ifndef __LORAWAN_MAC_CMD_H_
#define __LORAWAN_MAC_CMD_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "lorawan.h"
#include "lora.h"

#define LINKCHECK 0x02
#define LINKADDR 0x03
#define DUTYCYCLE 0x04
#define RXPARAMSETUP 0x05
#define DEVSTATUS 0x06
#define NEWCHANNEL 0x07
#define RXTIMINGSETUP 0x08
#define TXPARAMSETUP 0x09
#define DICHANNEL 0x0A
#define DEVICETIME 0x0D

#define BATTERY_EXTERNAL_POWER_SOURCE 0
#define BATTERY_UNABLE_TO_MEASURE 255

#define DR0 SF12_125
#define DR1 SF11_125
#define DR2 SF10_125
#define DR3 SF9_125
#define DR4 SF8_125
#define DR5 SF7_125
#define DR6 SF7_250

typedef uint8_t (*MACCommand_handler)(uint8_t *, uint8_t *);

void lorawan_parse_mac_payload(uint8_t * cmd, uint8_t cmd_len);

uint8_t RXParamSetupReq(uint8_t * payload, uint8_t * answer);
uint8_t DevStatusReq(uint8_t * payload, uint8_t * answer);
uint8_t LinkADRReq(uint8_t * payload, uint8_t * answer );

#endif
