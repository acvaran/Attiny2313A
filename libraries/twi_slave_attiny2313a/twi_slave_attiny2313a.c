/*
 * twi_attiny2313a.c
 *
 * Created: 7.01.2021 00:37:13
 *  Author: ailev_000
 */ 

#include "twi_slave_attiny2313a.h"

uint8_t twi_read() {
	if(twi_available() != 0x00) {
		uint8_t data = receive_buffer[receive_read_index];
		receive_read_index++;
		if(receive_read_index == RECEIVE_BUFFER_LENGTH) {
			receive_read_index = 0;
		}
		return data;
	} else {
		return 0x00;
	}
}

uint8_t twi_available() {
	return (receive_write_index - receive_read_index);
}

void twi_write(uint8_t data) {
	transmit_buffer[transmit_write_index] = data;
	transmit_write_index++;
	if(transmit_write_index >= TRANSMIT_BUFFER_LENGTH) {
		transmit_write_index = 0;
	}
}

void twi_reset_registers() {
	USISR = (1 << USISIF)	|	// CLEAR START CONDITION FLAG
			(1 << USIOIF)	|	// CLEAR COUNTER OVERFLOW FLAG
			(0 << USIPF)	|	// DON'T CLEAR STOP CONDITION FLAG
			(1 << USIDC)	|	// CLEAR DATA COLLISION FLAG
			(0x00 << USICNT0);	// RESET COUNTER
		
	USICR = (1 << USISIE)	|	// ENABLE START CONDITION INTERRUPT
			(0 << USIOIE)	|	// DISABLE COUNTER OVERFLOW INTERRUPT
			(1 << USIWM1)	| (0 << USIWM0)	|	// TWO-WIRE MODE WITH COUNTER OVERFLOW HOLD
			(1 << USICS1)	| (0 << USICS0) |	// CLOCK SOURCE: EXTERNAL POSITIVE EDGE
			(0 << USICLK)	|	// CLOCK STROVE: EXTERNAL, BOTH EDGES
			(0 << USITC);		// NO CLOCK TOGGLE
}

void twi_reset() {
	SDA_INPUT();
	SDA_PULL_UP_LOW();
	SCL_INPUT();
	SCL_PULL_UP_LOW();
	
	SDA_OUTPUT();
	SDA_HIGH();
	SDA_INPUT();
	SDA_HIGH();
	SCL_OUTPUT();
	SCL_HIGH();	
	
	twi_reset_registers();
}

void twi_init(uint8_t addr) {
	slave_address = addr << 1;
	receive_read_index = 0;
	receive_write_index = 0;
	transmit_read_index = 0;
	transmit_write_index = 0;
	state = MODE_READ_ADDRESS;
	cli();
	twi_reset();
	sei();
}

ISR(USI_START_vect) {
	
	SDA_INPUT();
	
	while((TWI_PIN & SDA_PI) && (TWI_PIN & SCL_PI));
	
	if(TWI_PIN & SDA_PI) {
		twi_reset();
		return;
	}
	state = MODE_READ_ADDRESS;
	USIDR = 0xFF;
	USICR = (1 << USISIE)	|	// ENABLE START CONDITION INTERRUPT
			(1 << USIOIE)	|	// ENABLE COUNTER OVERFLOW INTERRUPT
			(1 << USIWM1)	| (1 << USIWM0)	|	// TWO-WIRE MODE WITH COUNTER OVERFLOW HOLD
			(1 << USICS1)	| (0 << USICS0)	|	// CLOCK SOURCE: EXTERNAL POSITIVE EDGE
			(0 << USICLK)	|	// CLOCK STROBE: EXTERNAL, BOTH EDGES
			(0 << USITC);		// NO CLOCK TOGGLE
	USISR = (1 << USISIF)	| // CLEAR START CONDITION INTERRUPT FLAG
			(1 << USIOIF)	| // CLEAR COUNTER OVERFLOW INTERRUPT FLAG
			(0 << USIPF)	| // DON'T CLEAR STOP CONDITION FLAG
			(1 << USIDC)	| // CLEAR DATA OUTPUT COLLISION
			(0x00 << USICNT0); // SET COUNTER TO 0
}

ISR(USI_OVERFLOW_vect) {
	uint8_t counter_value = 0x00;
again:
	switch(state) 
	{
		case MODE_READ_ADDRESS:
		{
			uint8_t data = USIDR;
			uint8_t address = data & 0xFE;
			uint8_t direction = data & 0x01;
			if(address == slave_address) { // IF THE RECEIVED ADDRESS IS EQUAL TO THE SLAVE'S ADDRESS
				if(direction == 0x00) { // SLAVE READS FROM THE BUS
					state = MODE_RECEIVE_ACK; // CHANGE STATE TO MODE_RECEIVE_ACK
				} else { // SLAVE WRITES TO BUS
					state = MODE_TRANSMIT; // CHANGE STATE TO MODE_TRANSMIT
				}
				USIDR = 0x00; // CLEAR USI DATA REGISTER TO SEND ACK BIT
				counter_value = 0x0E; // SET COUNTER VALUE TO 14 TO OVERFLOW AFTER ONE BIT
				SDA_OUTPUT(); // SET SDA PIN DIRECTION AS OUTPUT
			} else if (address == 0x00) { // IF THE MASTER BROADCASTS A MESSAGE
				state = MODE_RECEIVE_ACK; // CHANGE STATE TO MODE_RECEIVE_ACK
				USIDR = 0x00; // SET USI DATA REGISTER TO SEND ACK BIT
				counter_value = 0x0E; // SET COUNTER VALUE TO 14 TO OVERFLOW AFTER ONE BIT
				SDA_OUTPUT(); // SET SDA PIN AS OUTPUT
			} else { // IF THE RECEIVED ADDRESS DOES NOT MATCH WITH THE SLAVE'S ADDRESS AND THE MESSAGE IS NOT BROADCASTED BY THE MASTER
				USIDR = 0x00; // CLEAR USI DATA REGISTER
				counter_value = 0x00; // RESET COUNTER
				twi_reset_registers(); // RESET THE TWI
			}
		} break;
		case MODE_TRANSMIT_ACK:
		{
			if(USIDR == 0x00) {
				state = MODE_TRANSMIT;
				goto again;
			} else {
				USIDR = 0x00;
				counter_value = 0x00;
				twi_reset_registers();
			}
		} break;
		case MODE_RECEIVE_ACK:
		{
			state = MODE_RECEIVE;
			counter_value = 0x00;
			SDA_INPUT();
		} break;
		case MODE_TRANSMIT:
		{
			state = MODE_REQUEST_ACK;
			counter_value = 0x00;
			SDA_OUTPUT();
			if(transmit_write_index == transmit_read_index) { // IF THE WRITE INDEX IS SMALLER OR EQUAL THAN READ INDEX
				USIDR = 0x00; // SEND EMPTY DATA PACKET
			} else { // IF THERE IS DATA IN BUFFER THAT HASN'T BEEN SENT
				USIDR = transmit_buffer[transmit_read_index]; // SEND THE CORRESPONDING DATA
				transmit_read_index++; // INCREMENT TRANSMIT READ INDEX
				if(transmit_read_index >= TRANSMIT_BUFFER_LENGTH) { // IF TRANSMIT READ INDEX OVERFLOWS THE BUFFER LENGTH
					transmit_read_index = 0; // RESET THE TRANSMIT READ INDEX
				}
			}
		} break;
		case MODE_RECEIVE:
		{
			receive_buffer[receive_write_index] = USIDR;
			receive_write_index++;
			if(receive_write_index >= RECEIVE_BUFFER_LENGTH) {
				receive_write_index = 0;
			}
			state = MODE_RECEIVE_ACK;	// SET STATE TO SEND ACK BIT
			USIDR = 0x00; // CLEAR USI DATA REGISTER TO SEND ACK BIT
			counter_value = 0x0E; // SET COUNOTER VALUE TO 14 TO OVERFLOW AFTER ONE BIT
			SDA_OUTPUT();
		} break;
		case MODE_REQUEST_ACK:
		{
			state = MODE_TRANSMIT_ACK;
			USIDR = 0x00;
			counter_value = 0x0E;
			SDA_INPUT();
		} break;
	}
	
	USISR = (0 << USISIF)	|				// DON'T CLEAR START CONDITION FLAG
			(1 << USIOIF)	|				// CLEAR OVERFLOW FLAG
			(0 << USIPF)	|				// DON'T CLEAR STOP CONDITION FLAG
			(1 << USIDC)	|				// CLEAR DATA OUTPUT COLLISION FLAG
			(counter_value << USICNT0);		// SET USI COUNTER TO DESIRED VALUE
}