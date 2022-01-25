#ifndef __LORA_H_
#define __LORA_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//*************************************************************************************************************
// PINS CONFIG
#define SS (1 << PB2)
#define DDR_SS DDRB
#define PORT_SS PORTB

#define RST (1 << PB1)
#define DDR_RST DDRB
#define PORT_RST PORTB

#define SCK (1 << PB5)
#define DDR_SCK DDRB
#define PORT_SCK PORTB

#define MOSI (1 << PB3)
#define DDR_MOSI DDRB
#define PORT_MOSI PORTB

#define MISO (1 << PB4)
#define DDR_MISO DDRB
#define PORT_MISO PORTB

#define DIO0_INTERRUPT_vect INT0_vect
#define DIO1_INTERRUPT_vect INT1_vect
//*************************************************************************************************************

//*************************************************************************************************************
// DEFINITIONS
#define LORAWAN_SYNC_WORD 0x34
//*************************************************************************************************************

//*************************************************************************************************************
// DIO FLAGS
extern volatile uint8_t dio0_flag;
extern volatile uint8_t dio1_flag;
//*************************************************************************************************************

//*************************************************************************************************************
// CUSTOM TYPES
typedef struct {
	uint8_t sf;
	uint8_t bw;
	uint8_t cr;
	uint32_t freq;
} RadioSettings_t;
//*************************************************************************************************************

//*************************************************************************************************************
// ENUMS
enum { INIT_RADIO_SUCCESS = 1, INIT_RADIO_FAILED } INIT_RADIO_STATUS;
//*************************************************************************************************************

//*************************************************************************************************************
// FUNCTIONS
void register_lora_rx_event_callback( void (*callback)( uint8_t *buf, uint8_t len, uint8_t status ) );

void lora_interrupt_init();
uint8_t lora_init();

uint8_t lora_exchange( uint8_t addr, uint8_t val );
uint8_t lora_read_register( uint8_t reg );
void lora_write_register( uint8_t reg, uint8_t value );

void lora_sleep();
void lora_standby();
void lora_rx_single();
void lora_rx_continuous();

void lora_set_spreading_factor( uint8_t sf );
void lora_set_rx_crc( uint8_t value );
void lora_set_freq( uint32_t freq );
void lora_set_explicit_header();
void lora_set_tx_power( uint8_t db );
void lora_set_bandwidth( uint8_t mode );
void lora_set_coding_rate( uint8_t rate );
void lora_set_sync_word( uint8_t word );
void lora_set_invert( uint8_t inverted );
void lora_set_agc( uint8_t onoff );
void lora_set_low_data_rate_optimize( uint8_t onoff );
void lora_set_settings( RadioSettings_t * set );
int8_t lora_get_snr();
int16_t lora_get_last_packet_rssi( uint32_t freq );
uint8_t lora_set_ocp( uint8_t max_current );
uint8_t lora_random();

uint8_t lora_read_rx( uint8_t * buf, uint8_t buf_max_len );
uint8_t lora_get_packet_len();

void lora_prepare_rx_single();
void lora_receive();
void lora_putd( uint8_t *buf, uint8_t len );
//*************************************************************************************************************

#endif
