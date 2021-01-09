/*
 * twi_attiny2313a.h
 *
 * Created: 7.01.2021 00:37:27
 *  Author: Ahmet Can Varan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef TWI_SLAVE_ATTINY2313A_H_
#define TWI_SLAVE_ATTINY2313A_H_

#define TWI_DDR								DDRB
#define TWI_PORT							PORTB
#define TWI_PIN								PINB
#define SDA_PO								PORTB5
#define SDA_PI								PINB5
#define SCL_PO								PORTB7
#define SCL_PI								PINB7

#define SDA_OUTPUT()						(DDRB |= (1 << SDA_PO))
#define SDA_INPUT()							(DDRB &= ~(1 << SDA_PI))
#define SDA_PULL_UP_LOW()					(PORTB &= ~(1 << SDA_PO)) // WHEN CONFIGURED AS INPUT DISABLES THE PULL UP
#define SDA_HIGH()							(PORTB |= (1 << SDA_PO))
#define SCL_OUTPUT()						(DDRB |= (1 << SCL_PO))
#define SCL_INPUT()							(DDRB &= ~(1 << SCL_PI))
#define SCL_PULL_UP_LOW()					(PORTB &= ~(1 << SCL_PO)) // WHEN CONFIGURED AS INPUT DISABLES THE PULL UP
#define SCL_HIGH()							(PORTB |= (1 << SCL_PO))

#define RECEIVE_BUFFER_LENGTH				40
#define TRANSMIT_BUFFER_LENGTH				40

#define MODE_READ_ADDRESS					0x00
#define MODE_TRANSMIT_ACK					0x01
#define MODE_RECEIVE_ACK					0x02
#define MODE_TRANSMIT						0x03
#define MODE_RECEIVE						0x04
#define MODE_REQUEST_ACK					0x05

uint8_t receive_buffer[RECEIVE_BUFFER_LENGTH];
uint8_t transmit_buffer[TRANSMIT_BUFFER_LENGTH];
uint8_t receive_write_index;						// INDEX FOR RECEIVE BUFFER WRITING
uint8_t receive_read_index;							// INDEX FOR RECEIVE BUFFER READING
uint8_t transmit_write_index;						// INDEX FOR TRANSMIT BUFFER WRITING
uint8_t transmit_read_index;						// INDEX FOR TRANSMIT BUFFER READING
uint8_t slave_address;
uint8_t state;

void write(uint8_t data);

uint8_t twi_available(void);
uint8_t twi_read(void);
void twi_write(uint8_t data);

void twi_reset_registers(void);
void twi_reset(void);
void twi_init(uint8_t addr);


#endif /* TWI_ATTİNY2313A_H_ */