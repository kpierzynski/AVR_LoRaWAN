#!/bin/python3
from wiringpi import digitalRead, digitalWrite, OUTPUT, INPUT, LOW, HIGH, pinMode, wiringPiSPIDataRW, wiringPiSetupGpio, wiringPiSPISetup, wiringPiSetup
from time import sleep
from random import randint as rand

def delay(ms):
	sleep(ms/1000)

SS = 6
DIO0 = 7
RST = 0

wiringPiSetup()
#wiringPiSetupGpio()

pinMode(SS, OUTPUT)
pinMode(DIO0, INPUT)
pinMode(RST, OUTPUT)

wiringPiSPISetup(0, 1000000)

LORAWAN_SYNC_WORD = 0x34
#Registers
REG_FIFO = 0x00
REG_OP_MODE = 0x01
REG_FRF_MSB = 0x06
REG_FRF_MID = 0x07
REG_FRF_LSB = 0x08
REG_PA_CONFIG = 0x09
REG_PA_RAMP = 0x0A
REG_OCP = 0x0b
REG_LNA = 0x0c
REG_FIFO_ADDR_PTR = 0x0d
REG_FIFO_TX_BASE_ADDR = 0x0e
REG_FIFO_RX_BASE_ADDR = 0x0f
REG_FIFO_RX_CURRENT_ADDR = 0x10
REG_IRQ_FLAGS = 0x12
REG_RX_NB_BYTES = 0x13
REG_MODEM_STAT = 0x18
REG_PKT_SNR_VALUE = 0x19
REG_PKT_RSSI_VALUE = 0x1a
REG_RSSI_VALUE = 0x1b
REG_MODEM_CONFIG_1 = 0x1d
REG_MODEM_CONFIG_2 = 0x1e
REG_SYMB_TIMEOUT_LSB = 0x1f
REG_PREAMBLE_MSB = 0x20
REG_PREAMBLE_LSB = 0x21
REG_PAYLOAD_LENGTH = 0x22
REG_PAYLOAD_MAX_LENGTH = 0x23
REG_MODEM_CONFIG_3 = 0x26
REG_FREQ_ERROR_MSB = 0x28
REG_FREQ_ERROR_MID = 0x29
REG_FREQ_ERROR_LSB = 0x2a
REG_RSSI_WIDEBAND = 0x2c
REG_DETECTION_OPTIMIZE = 0x31
REG_INVERTIQ = 0x33
REG_DETECTION_THRESHOLD = 0x37
REG_SYNC_WORD = 0x39
REG_INVERTIQ2 = 0x3b
REG_DIO_MAPPING_1 = 0x40
REG_VERSION = 0x42
REG_PA_DAC = 0x4d

#Modes
MODE_LONG_RANGE_MODE = 0x80
MODE_SLEEP = 0x00
MODE_STDBY = 0x01
MODE_TX = 0x03
MODE_RX_CONTINUOUS = 0b101
MODE_RX_SINGLE = 0b110

#PA = config
PA_BOOST = 0x80

#IRQ = masks
IRQ_TX_DONE_MASK = 0x08
IRQ_PAYLOAD_CRC_ERROR_MASK = 0x20
IRQ_RX_DONE_MASK = 0x40
IRQ_RX_TIMEOUT_MASK = 0x80

#RSSI = constants
RF_MID_BAND_THRESHOLD = 525E6
RSSI_OFFSET_HF_PORT = 157
RSSI_OFFSET_LF_PORT = 164

#Coding = rate
CODING_RATE_4_5 = 0b001
CODING_RATE_4_6 = 0b010
CODING_RATE_4_7 = 0b011
CODING_RATE_4_8 = 0b100

#Bandwidth
BANDWIDTH_7_8_KHZ = 0b0000
BANDWIDTH_10_4_KHZ = 0b0001
BANDWIDTH_15_6_KHZ = 0b0010
BANDWIDTH_20_8_KHZ = 0b0011
BANDWIDTH_31_25_KHZ = 0b0100
BANDWIDTH_41_7_KHZ = 0b0101
BANDWIDTH_62_5_KHZ = 0b0110
BANDWIDTH_125_KHZ = 0b0111
BANDWIDTH_250_KHZ = 0b1000
BANDWIDTH_500_KHZ = 0b1001

#Spreading = factor
SF6 = 6
SF7 = 7
SF8 = 8
SF9 = 9
SF10 = 10
SF11 = 11
SF12 = 12

MAX_PKT_LENGTH = 255

APP_EUI = b"\x00\x00\x00\x00\x00\x00\x00\x00"
DEV_EUI = b"\xB8\xBA\x04\xD0\x7E\xD5\xB3\x70"
APP_KEY = b"\x36\x2F\xD7\xA4\xA1\x5E\x81\x42\x25\xA1\x39\x77\xC9\xFB\xE4\x98"

def lora_read_register(addr):
	buf = [addr & 0x7F, 0x00]
	digitalWrite(SS, LOW)
	recv = wiringPiSPIDataRW( 0, bytes(buf) )
	digitalWrite(SS, HIGH)
	return recv[1][1]

def lora_write_register(addr, value):
	buf = [addr | 0x80, value]
	digitalWrite(SS, LOW)
	wiringPiSPIDataRW(0, bytes(buf))
	digitalWrite(SS, HIGH)

def lora_sleep_mode():
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP )

def lora_standby_mode():
	lora_write_register( REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY)

def lora_set_freq(freq):
	F_XOSC = 32000000
	f_Rf = (freq << 19) // F_XOSC

	lora_write_register(REG_FRF_MSB, (f_Rf >> 16) & 0xFF)
	lora_write_register(REG_FRF_MID, (f_Rf >> 8) & 0xFF)
	lora_write_register(REG_FRF_LSB, (f_Rf >> 0) & 0xFF)

def lora_set_spreading_factor(sf):
	lora_write_register(REG_MODEM_CONFIG_2, (lora_read_register(REG_MODEM_CONFIG_2) & 0b00001111) | (sf << 4));

def lora_set_bandwidth( mode ):
	lora_write_register(REG_MODEM_CONFIG_1, (lora_read_register(REG_MODEM_CONFIG_1) & 0b00001111) | (mode << 4));

def lora_set_coding_rate( rate ):
	lora_write_register(REG_MODEM_CONFIG_1, (lora_read_register(REG_MODEM_CONFIG_1) & 0b11110001) | (rate << 1));

def lora_set_sync_word( word ):
	lora_write_register(REG_SYNC_WORD, word);

def lora_set_tx_power(db):
	if db > 17:
		if db > 20:
			db = 20
		lora_write_register(REG_PA_DAC, (0x10 << 3) | 0x07)
		lora_write_register(REG_PA_CONFIG, PA_BOOST | ((db - 2) - 3))
	else:
		if db < 2:
			db = 2
		lora_write_register(REG_PA_DAC, (0x10 << 3) | 0x04)
		lora_write_register(REG_PA_CONFIG, PA_BOOST | (db - 2))

def lora_set_ocp(ma):
	if ma > 240:
		ma = 240

	if ma <= 120:
		ocp_trim = (ma - 45) // 5
	elif ma <= 240:
		ocp_trim = (ma + 30) // 10
	else:
		ocp_trim = 27

	lora_write_register(REG_OCP, (1 << 5) | (ocp_trim & 0b00011111))



def lora_init():

	digitalWrite(RST, HIGH)
	delay(50)
	digitalWrite(RST, LOW)
	delay(2)
	digitalWrite(RST, HIGH)
	delay(10)

	if lora_read_register(REG_VERSION) != 0x12:
		raise ValueError("Cannot init radio.")

	lora_sleep_mode()

	lora_set_freq(868100000)
	lora_set_spreading_factor(SF7)
	lora_set_bandwidth(BANDWIDTH_125_KHZ)
	lora_set_coding_rate(CODING_RATE_4_5)

	lora_set_sync_word( LORAWAN_SYNC_WORD )

	lora_set_ocp(200)
	lora_set_tx_power(20)

	lora_standby_mode()

	delay(100)


def lora_putd( msg ):
	lora_sleep_mode()
	delay(50)
	lora_standby_mode()
	delay(50)

	lora_write_register(REG_MODEM_CONFIG_1, 0x72)
	lora_write_register(REG_MODEM_CONFIG_2, 0x74)
	lora_write_register(REG_MODEM_CONFIG_3, 0x4)
	lora_write_register(0x3A, 0x3)
	lora_write_register(REG_FRF_MSB, 0xD9)
	lora_write_register(REG_FRF_MID, 0x06)
	lora_write_register(REG_FRF_LSB, 0x66)

	lora_write_register(REG_PA_RAMP, 0x08)
	lora_write_register(REG_PA_CONFIG, 0x8C)
	lora_write_register(REG_OCP, 0x2B)
	lora_write_register(REG_SYNC_WORD, 0x34)

	lora_write_register(REG_DIO_MAPPING_1, 0b01000000)

	lora_write_register(REG_FIFO_TX_BASE_ADDR, 0)
	lora_write_register(REG_FIFO_ADDR_PTR, 0)

	lora_write_register(REG_PAYLOAD_LENGTH, len(msg))

	for x in range(0, len(msg)):
		lora_write_register(REG_FIFO, msg[x])

	lora_write_register(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

from Crypto.Hash import CMAC
from Crypto.Cipher import AES
from Crypto.Cipher import AES

def mic(key,msg):

	c = CMAC.new(key, ciphermod=AES)
	c.update(msg)

	return c.digest()[:4]

def lorawan_parse_join_accept(data):
	global AppNonce
	global NetID
	global DevAddr

	if data[0] != 0x20:
		raise ValueError("MHDR is not JOIN ACCEPT")

	msg = [ 0 for x in range(0,33)]
	msg[0] = 0x20

	c = AES.new(APP_KEY, AES.MODE_ECB)
	msg[1:17] = c.encrypt(data[1:17])
	c = AES.new(APP_KEY, AES.MODE_ECB)
	msg[17:33] = c.encrypt(data[17:33])

	AppNonce = bytes(msg[1:4])
	NetID = bytes(msg[4:7])
	DevAddr = bytes(msg[7:11])

	print("DevAddr:", end=' ')
	print(DevAddr.hex())

def lorawan_derive_keys():
	global NwkSKey
	global AppSKey

	nwk = [0 for x in range(0,16)]
	app = [0 for x in range(0,16)]

	nwk[0] = 0x01
	app[0] = 0x02

	print("DevNonce:", end=' ')
	print(hex(DevNonce))

	print("AppNonce:", end=' ')
	print(AppNonce.hex())

	print("NetID:", end=' ')
	print(NetID.hex())

	nwk[1:4] = app[1:4] = AppNonce
	nwk[4:7] = app[4:7] = NetID
	nwk[7:9] = app[7:9] = DevNonce.to_bytes(2, byteorder='big')

	print("Nwk data:", end=' ')
	print(bytes(nwk).hex())
	print("App data:", end=' ')
	print(bytes(app).hex())

	c = AES.new(APP_KEY, AES.MODE_ECB)
	NwkSKey = c.encrypt(bytes(nwk))

	c = AES.new(APP_KEY, AES.MODE_ECB)
	AppSKey = c.encrypt(bytes(app))

def lorawan_encrypt_payload(msg):
	Uplink = True

	Dir = 0 if Uplink else 1

	a_1 = [0x01, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 1 ]
	a_1[6:10] = DevAddr
	a_1[10:14] = FCnt.to_bytes(4, byteorder='little')

	c = AES.new(AppSKey, AES.MODE_ECB)
	s_1 = c.encrypt(bytes(a_1))

	result = [ msg[x] ^ s_1[x] for x in range(0,16)]

	print("RESULT:", end=' ')
	print(bytes(result).hex())
	return bytes(result)

def lorawan_mic(msg):
	Uplink = True
	Dir = 0 if Uplink else 1

	b_0 = [0x49, 0x00, 0x00, 0x00, 0x00, Dir, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, len(msg)]
	b_0[6:10] = DevAddr
	b_0[10:14] = FCnt.to_bytes(4, byteorder='little')

	b_0 += msg

	print("B_0:", end=' ')
	print(bytes(b_0).hex())
	return mic(NwkSKey, bytes(b_0))

def lorawan_send_message(msg):
	global FCnt
	MHDR = 0b01000000

	print("Fcnt: " + str(FCnt))
	print("DevAddr len: " + str(len(DevAddr)))

	print("Fcnt as bytes:", end=' ')
	print(FCnt.to_bytes(2, byteorder='little'))

	Payload = MHDR.to_bytes(1, byteorder='big') + DevAddr + b"\x00" +  FCnt.to_bytes(2, byteorder='little') + (1).to_bytes(1, byteorder='big')
	print("Payload plain")
	print(Payload.hex())

	Payload += lorawan_encrypt_payload(msg)
	print("Payload withenc")
	print(Payload.hex())

	Payload += lorawan_mic(Payload)

	print("Payload:", end=' ')
	print(Payload.hex())
	print("Payload len:", end=' ')
	print(len(Payload))
	lora_putd(Payload)
	while not digitalRead(DIO0):
		pass

	FCnt += 1

	lora_rx()

	while not digitalRead(DIO0):
		pass

	packet_len = lora_read_register(REG_RX_NB_BYTES)

	lora_write_register(REG_FIFO_ADDR_PTR, lora_read_register(REG_FIFO_RX_CURRENT_ADDR))

	packet = []
	for x in range(0, packet_len):
		packet.append( lora_read_register(REG_FIFO) )
	packet = bytes(packet)

	print(packet.hex())


def lora_rx():
	lora_sleep_mode()
	delay(50)
	lora_standby_mode()
	delay(50)

	lora_write_register(REG_IRQ_FLAGS, 0xFF)
	lora_write_register(REG_DIO_MAPPING_1, 0x00)
	lora_write_register(REG_INVERTIQ, 0x67)

	lora_write_register(REG_MODEM_CONFIG_2, 0x70)
	lora_write_register(REG_SYMB_TIMEOUT_LSB, 0x2F)
	lora_write_register(REG_FIFO_ADDR_PTR, 0)
	lora_write_register(REG_FIFO_RX_BASE_ADDR, 0)
	lora_write_register(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);



def lorawan_send_join_request():
	global DevNonce

	DevNonce = rand(0,0xFFFF)
	MHDR = 0

	JoinRequest = MHDR.to_bytes(1, byteorder='big') + APP_EUI + DEV_EUI + DevNonce.to_bytes(2, byteorder='big')

	print(JoinRequest.hex())
	print(len(JoinRequest))

	JoinRequest += mic(APP_KEY, JoinRequest)

	print(JoinRequest.hex())
	print(len(JoinRequest))

	lora_putd(JoinRequest)

	while not digitalRead(DIO0):
		pass

	lora_rx()

	while not digitalRead(DIO0):
		pass

	packet_len = lora_read_register(REG_RX_NB_BYTES)

	lora_write_register(REG_FIFO_ADDR_PTR, lora_read_register(REG_FIFO_RX_CURRENT_ADDR))

	packet = []
	for x in range(0, packet_len):
		packet.append( lora_read_register(REG_FIFO) )
	packet = bytes(packet)

	print(packet.hex())
	lorawan_parse_join_accept(packet)
	lorawan_derive_keys()

	print(DevAddr.hex())
	print(NwkSKey.hex())
	print(AppSKey.hex())

DevNonce = 0
DevAddr = 0
AppNonce = 0
NetID = 0
NwkSKey = 0
AppSKey = 0
FCnt = 1

lora_init()
delay(10000)
print()
lorawan_send_join_request()
delay(10000)
print()
lorawan_send_message(b'\x69\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
