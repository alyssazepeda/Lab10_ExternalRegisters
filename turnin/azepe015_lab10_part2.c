/*	Author: Alyssa Zepeda
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #2
 *	Exercise Description: https://youtu.be/SE1S3T4g2VM 
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#include "timer.h"
#endif

#define tasksSize 4
//struct
typedef struct task {
        int state;
        unsigned long period;
        unsigned long elapsedTime;
        int (*TickFct)(int);
} task;
task tasks[tasksSize];
//GCD of the periods
const unsigned long tasksPeriod = 100;
///////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////
//timer.h functions that needed to be added in main.c
void TimerISR() {
        unsigned char i;
        for (i = 0; i < tasksSize; ++i) {
                if ( tasks[i].elapsedTime >= tasks[i].period ) {
                        tasks[i].state = tasks[i].TickFct(tasks[i].state);
                        tasks[i].elapsedTime = 0;
                }
                tasks[i].elapsedTime += tasksPeriod;
        }
}
ISR(TIMER1_COMPA_vect) {
    _avr_timer_cntcurr--;
    if (_avr_timer_cntcurr == 0){
        TimerISR();
        _avr_timer_cntcurr = _avr_timer_M;
    }
}

////////////////////////////////////////////////////////////////////
//Display 1
unsigned char display1;
unsigned char i;
unsigned char arr1[] = {0x00, 0x81, 0xC3, 0xE7, 0xFF, 0xE7, 0xC3, 0x81};
enum D1_States{D1_start, D1};
int D1_Tick(int state) {
	switch(state) {
		case D1_start: state = D1; break;
		case D1: state = D1; break;
		default: state = D1_start; break;
	}
	switch(state) {
		case D1_start: display1 = 0x00; i = 0x00; break; case D1:
			if(i < 8) {
				display1 = arr1[i];
				i++;
			}
			else {
				i = 0;
				display1 = arr1[i];
			}
			break;
		default: break;
	}
	return state;
}

//Display 2
unsigned char display2;
unsigned char j;
unsigned char arr2[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
enum D2_States{D2_start, Dup, Ddown};
int D2_Tick(int state) {
	switch(state) {
		case D2_start: state = Dup; break;
		case Dup: state = (j < 7) ? Dup : Ddown; break;
		case Ddown: state = (j > 0) ? Ddown : Dup; break;
		default: state = D2_start; break;
	}
	switch(state) {
		case D2_start: display2 = 0x00; j = 0x00; break;
		case Dup: 
			//starts w j=0
			display2 = arr2[j];
			j++;
			//ends w j=7
			break;
		case Ddown:
			//starts w j=7
			display2 = arr2[j];
			j--;
			//ends w j=0
			break;
		default: break;
	}
	return state;
}

//Display 3
unsigned char display3;
enum D3_States{D3_start, D55, DAA};
int D3_Tick(int state) {
	switch(state) {
		case D3_start: state = D55; break;
		case D55: state = DAA; break;
		case DAA: state = D55; break;
		default: state = D3_start; break;
	}
	switch(state) {
		case D3_start: display3 = 0x00; break;
		case D55: display3 = 0x55; 
			  break;
		case DAA: display3 = 0xAA; 
			  break;
		default: break;
	}
	return state;
}

//Button Control
unsigned char go;
#define A0 ~PINA&0x01
#define A1 ~PINA&0x02

enum B_States{start, check, inc, dec, waitR, off, waitOn};
int B_Tick(int state) {
	switch(state) {	
		case start: state = check; break;
		case check: 
			if(!(A0) && !(A1)) {
				state = check;
			}
			else if(A0 && !(A1)) {
				state = inc;
			}
			else if(!(A0) && A1) {
				state = dec;
			}
			else { state = off;}
			break;	
		case inc: state = waitR; break;
		case dec: state = waitR; break;
		case waitR: 
			if(!(A0) && !(A1)) { state = check;}
			else if(A0 && A1) {state = off;}
			else {state = waitR;}
			break;
		case off:
		       //still pressing both buttons machine is off	
			if(A0 && A1) {
				state = off;
			} 
			else {state = waitOn;}
			break;
		case waitOn: 
			if(A0 && A1) {
				state = start;
			}
			else {state = waitOn;}
			break;
		default: state = start; break;
	}
	switch(state) {
		case start: go = 0x01; break;
		case check: 
			if(go == 0x01) {transmit_data(display1);}
			else if(go == 0x02) {transmit_data(display2);}
			else{transmit_data(display3);}
			break;
		case inc:
			i = 0;
		       	j = 0;	
			if(go == 0x01) {
				go = 0x02; 	
			}
			else if(go == 0x02) {
				go = 0x03; 
			}
			else {
				go = 0x01;
			}
			break;
		case dec:
			i = 0;
			j =0;
			if(go == 0x01) {
                                go = 0x03;
                        }
                        else if(go == 0x02) {
                                go = 0x01;
                        }
                        else {
                                go = 0x02;
                        }
			break;
		case waitR: 
			if(go == 0x01) {transmit_data(display1);}
                        else if(go == 0x02) {transmit_data(display2);}
                        else{transmit_data(display3);}
                        break;
		case off: 
		case waitOn:
			transmit_data(0);
			break;
		default: break;
	}
	return state;
}



int main(void){
 	DDRA = 0X00; PINA = 0X0F; //A IS INPUT
	DDRC = 0XFF; PINC = 0X00; //C IS OUTPUT
	go = 0x01;
	unsigned char c = 0;
	tasks[c].state = start;
	tasks[c].period = tasksPeriod;
	tasks[c].elapsedTime = tasks[c].period;
	tasks[c].TickFct = &B_Tick;
	c++;
	tasks[c].state = D1_start;
        tasks[c].period = 100;
        tasks[c].elapsedTime = tasks[c].period;
        tasks[c].TickFct = &D1_Tick;
	c++;
	tasks[c].state = D2_start;
        tasks[c].period = 100;
        tasks[c].elapsedTime = tasks[c].period;
        tasks[c].TickFct = &D2_Tick;
	c++;
	tasks[c].state = D3_start;
        tasks[c].period = 300;
        tasks[c].elapsedTime = tasks[c].period;
        tasks[c].TickFct = &D3_Tick;

	TimerSet(tasksPeriod);
	TimerOn();
	while (1){}
    return 1;
}
