/*	Author: Alyssa Zepeda
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #1
 *	Exercise Description: https://youtu.be/sZxvS8usevQ
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void transmit_data(unsigned char data) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
   	 PORTC = 0x08;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    PORTC |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
}

//start, off, off release, on press, on release, off press
enum states {start, check, inc, dec, wait1, wait2} state;
unsigned char tempC = 0x00;

int TickFct() {
	switch(state) { //transitions
		case start:
			tempC= 0x00;
			state = check;
		       	break;
		case check:
			if((~PINA & 0x03) == 0x01) {
				state = inc;
			}
			else if((~PINA & 0x03) == 0x02) {
				state = dec;
			}
			else {
				state = check;
			}
			break;
		case inc:
			state = wait1;
			break;
		case dec:
			state = wait2;
			break;
		case wait1:
			state = ((~PINA & 0x03) == 0x01) ? wait1 : check;
			break;
		case wait2:
			state = ((~PINA & 0x03) == 0x02) ? wait2 : check;
			break;
		default:
			state = start;
			break;	
	}
	switch(state) { //actions
		case start:
			break;
		case check:
			if((~PINA & 0x03) == 0x03) {
				tempC = 0;
			}
			transmit_data(tempC);
			break;
		case inc:
			if(tempC < 0xFF) {
				tempC++;

			}
			transmit_data(tempC);
			break;
		case dec:
			if(tempC > 0x00) {
				tempC--;
			}
			transmit_data(tempC);
			break;
		case wait1:
		case wait2:
			if((~PINA & 0x03) == 0x03) {
				tempC = 0;
			}
			transmit_data(tempC);
			break;
		default: break;
	}
}


int main(void){
	state = start;
 	DDRA = 0X00; PINA = 0X0F; //A IS INPUT
	DDRC = 0X0F; PINC = 0X00; //C IS OUTPUT
	while (1) {
		TickFct();
    	}
    return 1;
}
