/*	Author: Alyssa Zepeda
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #10  Exercise #3
 *	Exercise Description:  https://youtu.be/tupRsjrQ31Q
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
const unsigned long tasksPeriod = 50;
///////////////////////////////////////////////////////////////
void transmit_data(unsigned char data, unsigned char reg) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
	 if(reg == 1) { PORTC = 0x08;}//C3
	 else {PORTC = 0x20;}//C5
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  //C1
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    if(reg == 1) {PORTC |= 0x04;}//C2
    else {PORTC |= 0x10;}//C4
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
unsigned char goR1;
unsigned char goR2;
unsigned char flag1;
unsigned char flag2;
#define A0 ~PINA&0x01
#define A1 ~PINA&0x02
#define A2 ~PINA&0x04
#define A3 ~PINA&0x08

enum B_States{start, check, incR1, decR1, incR2, decR2, waitR, off1, off2};
int B_Tick(int state) {
	switch(state) {	
		case start: state = check; break;
		case check: 
			if(A0 && !(A1)) { //R1 up
				state = incR1;
			}
			else if(!(A0) && A1) { //R1 down
				state = decR1;
			}
			else if(A2 && !(A3)) { //R2 up
				state = incR2;
			}
			else if(!(A2) && A3) { //R2 down
				state = decR2;
			}
			else if(A0 && A1) { //if both, R1 off
				state = off1;
				//to turn off, flag is lowered
				//to turn on, flag is raised
				flag1 = !flag1;
			}
			else if(A2 && A3) { //if both, R2 off
				state = off2;
				flag2 = !flag2;
			}
			else {state = check;}
			break;	
		case incR1: state = (A0 && A1) ? off1 : waitR; break;
		case decR1: state = (A0 && A1) ? off1 : waitR; break;
		case incR2: state = (A2 && A3) ? off2 : waitR; break;
		case decR2: state = (A2 && A3) ? off2 : waitR; break;
		case waitR: 
			if(!(A0) && !(A1) && !(A2) && !(A3)) { state = check;}
			else if(A0 && A1) {state = off1; flag1 = !flag1;}
			else if(A2 && A3) {state = off2; flag2 = !flag2;}
			else {state = waitR;}
			break;
		case off1:
		       //still pressing both buttons machine is off	
			if(A0 && A1) {state = off1;}
			else {state = check;}
			break;
		case off2:
			if(A2 && A3) {state = off2;}
			else {state = check;}
			break;
		default: state = start; break;
	}
	switch(state) {
		case start: goR1 = 0x00; goR2 = 0x00; 
			    flag1 = 0x00; flag2 = 0x00; 
			    break;
		case check: 
		case waitR:
			//R1
			if(goR1 == 0x00) {transmit_data(0, 1);}
			else if(goR1 == 0x01) {transmit_data(display1, 1);}
			else if(goR1 == 0x02) {transmit_data(display2, 1);}
			else if(goR1 == 0x03) {transmit_data(display3, 1);}
			//R2
			if(goR2 == 0x00) {transmit_data(0, 2);}
			else if(goR2 == 0x01) {transmit_data(display1, 2);}
                        else if(goR2 == 0x02) {transmit_data(display2, 2);}
                        else if(goR2 == 0x03) {transmit_data(display3, 2);}
			break;
		case incR1:
			if(goR1 == 0x01) {j = 0; goR1 = 0x02;}
			else if(goR1 == 0x02) {goR1 = 0x03;}
			else if(goR1 == 0x03) {i = 0; goR1 = 0x01;}
			break;
		case decR1:
			if(goR1 == 0x01) {goR1 = 0x03;}
                        else if(goR1 == 0x02) {i = 0; goR1 = 0x01;}
                        else if(goR1 == 0x03) {j = 0; goR1 = 0x02;}
			break;
		case incR2:
                        if(goR2 == 0x01) {j = 0; goR2 = 0x02;}
                        else if(goR2 == 0x02) {goR2 = 0x03;}
                        else if(goR2 == 0x03) {i = 0; goR2 = 0x01;}
                        break;
                case decR2:
                        if(goR2 == 0x01) {goR2 = 0x03;}
                        else if(goR2 == 0x02) {i = 0; goR2 = 0x01;}
                        else if(goR2 == 0x03) {j = 0; goR2 = 0x02;}
                        break;
		case off1: 
			if(goR1 > 0 && !flag1) {goR1 = 0x00;}
			else if (goR1 == 0x00 && flag1) {goR1 = 0x01;}
			//R2 still goes in this state
                        if(goR2 == 0x00) {transmit_data(0, 2);}
                        else if(goR2 == 0x01) {transmit_data(display1, 2);}
                        else if(goR2 == 0x02) {transmit_data(display2, 2);}
                        else if(goR2 == 0x03) {transmit_data(display3, 2);}
		       	break;
		case off2: 
			if(goR2 > 0 && !flag2) {goR2 = 0x00;}
			else if (goR2 == 0x00 && flag2) {goR2 = 0x01;}
			//R1 still goes in this state
			if(goR1 == 0x00) {transmit_data(0, 1);}
                        else if(goR1 == 0x01) {transmit_data(display1, 1);}
                        else if(goR1 == 0x02) {transmit_data(display2, 1);}
                        else if(goR1 == 0x03) {transmit_data(display3, 1);}
			break;	
		default: break;
	}
	return state;
}



int main(void){
 	DDRA = 0X00; PINA = 0X0F; //A IS INPUT
	DDRC = 0XFF; PINC = 0X00; //C IS OUTPUT
	//goR1 = 0x00;
	//goR2 = 0x00;
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
