#include "lora.h"

#include "lora_mem.h"
#include "spi.h"

static uint8_t buf[MAX_PKT_LENGTH]; //TODO: is this still needed?

//*************************************************************************************************************
// DIO FLAGS
volatile uint8_t dio0_flag;
volatile uint8_t dio1_flag;
//*************************************************************************************************************

//*************************************************************************************************************
// CALLBACK
static void (*lora_rx_event_callback)( uint8_t *buf, uint8_t len, uint8_t status );

void register_lora_rx_event_callback( void (*callback)( uint8_t *buf, uint8_t len, uint8_t status ) ) {
	lora_rx_event_callback = callback;
}
//*************************************************************************************************************

//*************************************************************************************************************
// SETUP INTERRUPT REGISTERS FOR IRQ DIO
void lora_interrupt_init() {
	//Set INT0 to rising edge
	EICRA |= ( 1 << ISC01 ) | ( 1 << ISC00 );
	EIMSK |= ( 1 << INT0 );

	//Set INT1 to rising edge
	EICRA |= ( 1 << ISC11 ) | ( 1 << ISC10 );
	EIMSK |= ( 1 << INT1 );
}
//*************************************************************************************************************

//*************************************************************************************************************
// IRQ VECTS
ISR(DIO1_INTERRUPT_vect) {
	dio1_flag = 1;
}

ISR(DIO0_INTERRUPT_vect) {
	dio0_flag = 1;
}
//*************************************************************************************************************

//*************************************************************************************************************
// LORA_INIT
uint8_t lora_init() {
	spi_init();

	DDR_RST |= RST;

	PORT_RST |= RST;
	_delay_ms( 50 );
	PORT_RST &= ~RST;
	_delay_ms( 1 );
	PORT_RST |= RST;
	_delay_ms( 10 );

	PORT_SS |= SS;

	lora_interrupt_init();

	uint8_t version = lora_read_register( REG_VERSION );
	if( version != SX1276_VERSION )
		return INIT_RADIO_FAILED;

	lora_sleep();
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE );

	lora_set_spreading_factor( SF7 );
	lora_set_bandwidth( BANDWIDTH_125_KHZ );
	lora_set_coding_rate( CODING_RATE_4_5 );
	lora_set_freq( 868100000UL );

	lora_write_register( REG_PAYLOAD_MAX_LENGTH, MAX_PKT_LENGTH );

	lora_set_sync_word( LORAWAN_SYNC_WORD );

	lora_set_tx_power( 20 );

	lora_set_explicit_header();

	lora_standby();

	_delay_ms( 50 );

	return INIT_RADIO_SUCCESS;
}
//*************************************************************************************************************

//*************************************************************************************************************
// COMMUNICATION WITH SPI
uint8_t lora_exchange( uint8_t addr, uint8_t val ) {
	uint8_t data;

	PORT_SS &= ~SS;
	spi_tx( addr );
	data = spi_x( val );
	PORT_SS |= SS;

	return data;
}

uint8_t lora_read_register( uint8_t reg ) {
	return lora_exchange( reg & 0x7f, 0x00 );
}

void lora_write_register( uint8_t reg, uint8_t value ) {
	lora_exchange( reg | 0x80, value );
}
//*************************************************************************************************************

//*************************************************************************************************************
// MODULE MODES
inline void lora_sleep() {
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP );
}

inline void lora_standby() {
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY );
}

inline void lora_rx_single() {
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE );
}

inline void lora_rx_continuous() {
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS );
}
//*************************************************************************************************************

//*************************************************************************************************************
// SETTINGS FUNCTIONS

void lora_set_spreading_factor( uint8_t sf ) {
	lora_write_register( REG_MODEM_CONFIG_2, ( lora_read_register( REG_MODEM_CONFIG_2 ) & 0b00001111 ) | ( sf << 4 ) );
}

void lora_set_rx_crc( uint8_t value ) {
	lora_write_register( REG_MODEM_CONFIG_2, ( lora_read_register( REG_MODEM_CONFIG_2 ) & 0b11111011 ) | ( ( value ? 1 : 0 ) << 2 ) );
}

void lora_set_freq( uint32_t freq ) {
#define F_XOSC 32000000UL
	uint64_t f_Rf = ( (uint64_t )( freq ) << 19 ) / F_XOSC;

	lora_write_register( REG_FRF_MSB, ( f_Rf >> 16 ) & 0xFF );
	lora_write_register( REG_FRF_MID, ( f_Rf >> 8 ) & 0xFF );
	lora_write_register( REG_FRF_LSB, ( f_Rf >> 0 ) & 0xFF );
}

uint8_t lora_set_ocp( uint8_t max_current ) {
	uint8_t ocp_trim;

	if( max_current > 240 )
		return 0;

	if( max_current <= 120 ) {
		ocp_trim = ( max_current - 45 ) / 5;
	} else if( max_current <= 240 ) {
		ocp_trim = ( max_current + 30 ) / 10;
	} else {
		ocp_trim = 27;
	}

	lora_write_register( REG_OCP, ( 1 << 5 ) | ( ocp_trim & 0b00011111 ) );

	return 1;
}

void lora_set_explicit_header() {
	lora_write_register( REG_MODEM_CONFIG_1, lora_read_register( REG_MODEM_CONFIG_1 ) & 0b11111110 );
}

int16_t lora_get_last_packet_rssi( uint32_t freq ) {
	uint8_t rssi = lora_read_register( REG_PKT_RSSI_VALUE );
	return -( freq < RF_MID_BAND_THRESHOLD ? RSSI_OFFSET_LF_PORT : RSSI_OFFSET_HF_PORT ) + rssi;
}

void lora_set_tx_power( uint8_t db ) {
	lora_set_ocp( 150 );

	if( db > 17 ) {
		if( db > 20 )
			db = 20;
		lora_write_register( REG_PA_DAC, ( 0x10 << 3 ) | 0x07 );
		lora_write_register( REG_PA_CONFIG, PA_BOOST | ( ( db - 2 ) - 3 ) );
	} else {
		if( db < 2 )
			db = 2;
		lora_write_register( REG_PA_DAC, ( 0x10 << 3 ) | 0x04 );
		lora_write_register( REG_PA_CONFIG, PA_BOOST | ( db - 2 ) );
	}
}

void lora_set_bandwidth( uint8_t mode ) {
	lora_write_register( REG_MODEM_CONFIG_1, ( lora_read_register( REG_MODEM_CONFIG_1 ) & 0b00001111 ) | ( mode << 4 ) );
}

void lora_set_coding_rate( uint8_t rate ) {
	lora_write_register( REG_MODEM_CONFIG_1, ( lora_read_register( REG_MODEM_CONFIG_1 ) & 0b11110001 ) | ( rate << 1 ) );
}

uint8_t lora_random() {
	return lora_read_register( REG_RSSI_WIDEBAND );
}

void lora_set_sync_word( uint8_t word ) {
	lora_write_register( REG_SYNC_WORD, word );
}

void lora_set_invert( uint8_t inverted ) {
	lora_write_register( REG_INVERTIQ, ( ( inverted ? 1 : 0 ) << 6 ) | 0x27 );
}

void lora_set_agc( uint8_t onoff ) {
	lora_write_register( REG_MODEM_CONFIG_3, ( lora_read_register( REG_MODEM_CONFIG_3 ) & 0b11111011 ) | ( ( onoff ? 1 : 0 ) << 2 ) );
}

void lora_set_low_data_rate_optimize( uint8_t onoff ) {
	lora_write_register( REG_MODEM_CONFIG_3, ( lora_read_register( REG_MODEM_CONFIG_3 ) & 0b11110111 ) | ( ( onoff ? 1 : 0 ) << 3 ) );
}

int8_t lora_get_snr() {
	return ( (int8_t )lora_read_register( REG_PKT_SNR_VALUE ) ) >> 2;
}

void lora_set_settings( RadioSettings_t * set ) {
	lora_set_spreading_factor( set->sf );
	lora_set_bandwidth( set->bw );
	lora_set_coding_rate( set->cr );

	lora_set_freq( set->freq );
}

//*************************************************************************************************************

//*************************************************************************************************************
// READ PACKET FROM FIFO
uint8_t lora_read_rx( uint8_t * buf, uint8_t buf_max_len ) {
	uint8_t len = lora_read_register( REG_RX_NB_BYTES );

	lora_write_register( REG_FIFO_ADDR_PTR, lora_read_register( REG_FIFO_RX_CURRENT_ADDR ) );

	uint8_t i;
	uint8_t loop = ( len > buf_max_len ) ? buf_max_len : len;
	for( i = 0; i < loop; i++ )
		buf[i] = lora_read_register( REG_FIFO );

	return len;
}

inline uint8_t lora_get_packet_len() {
	return lora_read_register( REG_RX_NB_BYTES );
}
//*************************************************************************************************************

//*************************************************************************************************************
// SEND AND RECEIVE
void lora_prepare_rx_single() {
	lora_standby();
	_delay_ms( 1 );

	lora_write_register( REG_IRQ_FLAGS, 0xFF );

	lora_write_register( REG_DIO_MAPPING_1, 0x00 );
	lora_set_invert( 1 );
	lora_set_rx_crc( 0 );

	lora_write_register( REG_LNA, 0x20 | 0x03 );
	lora_write_register( REG_SYMB_TIMEOUT_LSB, 0x08 );

	lora_write_register( REG_FIFO_ADDR_PTR, 0 );
	lora_write_register( REG_FIFO_RX_BASE_ADDR, 0 );
}

void lora_receive() {
	lora_standby();
	_delay_ms( 1 );

	lora_write_register( REG_IRQ_FLAGS, 0xFF );

	lora_write_register( REG_DIO_MAPPING_1, 0x00 );
	lora_set_invert( 1 );
	lora_set_rx_crc( 0 );

	lora_write_register( REG_FIFO_ADDR_PTR, 0 );
	lora_write_register( REG_FIFO_RX_BASE_ADDR, 0 );

	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS );
}

void lora_putd( uint8_t *buf, uint8_t len ) {
	if( len == 0 ) {
		return;
	}

	lora_standby();
	_delay_ms( 1 );

	lora_set_explicit_header();

	lora_set_rx_crc( 1 );

	lora_set_agc( 1 );
	lora_set_low_data_rate_optimize( 0 );

	lora_write_register( REG_HIGH_BW_OPTIMIZE_2, 0x3 );

	lora_write_register( REG_PA_RAMP, 0x08 );
	lora_write_register( REG_PA_CONFIG, 0x8C );

	lora_set_ocp( 150 );

	lora_set_sync_word( LORAWAN_SYNC_WORD );

	lora_write_register( REG_DIO_MAPPING_1, 0b01000000 );

	lora_write_register( REG_FIFO_TX_BASE_ADDR, 0 );
	lora_write_register( REG_FIFO_ADDR_PTR, 0 );

	lora_write_register( REG_PAYLOAD_LENGTH, len );
	for( uint8_t i = 0; i < len; i++ )
		lora_write_register( REG_FIFO, buf[i] );

	_delay_ms( 1 );

	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX );
}
//*************************************************************************************************************
