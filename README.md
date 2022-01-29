# AVR_LoRaWAN

Important: This code is under development.
<br />
This is simple, not full implementation of LoRaWAN 1.0.3 protocol for AVR

# Config
This code is written for SX1276 and AVR ATMega328p @ 16MHz to use in [TTN](https://www.thethingsnetwork.org).
<br />
You may configure pins in `lora.h` file as well as provide device credentials in `lorawan_credentials.h` file.

# State
What is working?
- &#9745; Joining to TTN over OTAA (Over-The-Air Activation)
- &#9745; Unconfirmed uplink `n` bytes of data
- &#9745; Unconfirmed downlink of data
- &#9744; MAC commands:
	- &#9745; Device Status Command 
	- &#9744; Link ADR Command (partially)

# Goals
- Confirmed up- and down- link.
- Handle more MAC commands ( All if possible without own gateway. (Why are they so expensive though?) )