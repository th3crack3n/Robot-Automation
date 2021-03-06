/**
 * util.c: utility functions for the Atmel platform
 * 
 * For an overview of how timer based interrupts work, see
 * page 111 and 133-137 of the Atmel Mega128 User Guide
 *
 * @author Zhao Zhang & Chad Nelson
 * @date 07/06/2011
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "util.h"

// Global used for interrupt driven delay functions
volatile unsigned int timer2_tick;
void timer2_start(char unit);
void timer2_stop();


// Blocks for a specified number of milliseconds
void wait_ms(unsigned int time_val) {
	//Seting OC value for time requested
	OCR2=250; 				//Clock is 16 MHz. At a prescaler of 64, 250 timer ticks = 1ms.
	timer2_tick=0;
	timer2_start(0);

	//Waiting for time
	while(timer2_tick < time_val);

	timer2_stop();
}


// Start timer2
void timer2_start(char unit) {
	timer2_tick=0;
	if ( unit == 0 ) { 		//unit = 0 is for slow 
        TCCR2=0b00001011;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 64
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
	}
	if (unit == 1) { 		//unit = 1 is for fast
        TCCR2=0b00001001;	//WGM:CTC, COM:OC2 disconnected,pre_scaler = 1
        TIMSK|=0b10000000;	//Enabling O.C. Interrupt for Timer2
	}
	sei();
}


// Stop timer2
void timer2_stop() {
	TIMSK&=~0b10000000;		//Disabling O.C. Interrupt for Timer2
	TCCR2&=0b01111111;		//Clearing O.C. settings
}


// Interrupt handler (runs every 1 ms)
ISR (TIMER2_COMP_vect) {
	timer2_tick++;
}




/// Initialize PORTC to accept push buttons as input
void init_push_buttons(void) {
	DDRC &= 0xC0;  //Setting PC0-PC5 to input
	PORTC |= 0x3F; //Setting pins' pull up resistors
}

/// Return the position of button being pushed
/**
 * Return the position of button being pushed.
 * @return the position of the button being pushed.  A 1 is the rightmost button.  0 indicates no button being pressed
 */
uint8_t read_push_buttons(void) {
	int shift;

	for (shift = 5; shift > -1; shift--) {
		if (((PINC >> shift) & 0x01) == 0)
			break;
	}

	return shift + 1;
}




/// Initialize PORTC for input from the shaft encoder
void shaft_encoder_init(void) {
	DDRC &= 0x3F;	//Setting PC6-PC7 to input
	PORTC |= 0xC0;	//Setting pins' pull-up resistors
}

/// Read the shaft encoder
/**
 * Reads the two switches of the shaft encoder and compares the values
 * to the previous read.  This function should be called very frequently
 * for the best results.
 *
 * @return a value indicating the shaft encoder has moved:
 * 0 = no rotation (switches did not change)
 * 1 = CW rotation
 * -1 = CCW rotation
 */
int8_t read_shaft_encoder(void) {
	// Static variable to store the old value
	static int8_t old_value = 0x03;
	// Function variables
	int8_t new_value = PINC >> 6;
	int8_t rotation = 0;

	if (old_value == 0x03) {
		if (new_value == 0x01)
			rotation = 1;
		if (new_value == 0x02)
			rotation = -1;
	}

	old_value = new_value;

	return rotation;
}



/// Initialize PORTE to control the stepper motor
void stepper_init(void) {
	DDRE |= 0xF0;  	//Setting PE4-PE7 to output
	PORTE &= 0x8F;  //Initial postion (0b1000) PE4-PE7
	wait_ms(2);
	PORTE &= 0x0F;  //Clear PE4-PE7
}

/// Turn the Stepper Motor
/**
 * Turn the stepper motor a given number of steps. 
 *
 * @param num_steps A value between 1 and 200 steps (1.8 to 360 degrees)
 * @param direction Indication of direction: 1 for CW and -1 for CCW 
 */
void  move_stepper_motor_by_step(int num_steps, int direction) {
	int i=0;
	char full_byte;
	static int coil_position=0b0001;
		
	if(direction==1) {
	// If Clockwise
		for(i=0;i<num_steps;i++) {
			if (coil_position==0b1000) {
				coil_position = 0b0001;
			} else {
				coil_position = coil_position << 1;
			}
			PORTE &= 0x0F;
			full_byte=coil_position << 4;
			PORTE |= full_byte;
			wait_ms(2);
		}   
	} else if (direction == -1) {
	// Else if counterclockwise
		for(i=0;i<num_steps;i++) {
			if (coil_position==0b0001) {
				coil_position = 0b1000;
			} else {
				coil_position = coil_position >> 1;
			}
			PORTE &= 0x0F;
			full_byte=coil_position << 4;
			PORTE |= full_byte;
			wait_ms(2);
		}   
	}

	PORTE &= 0x0F; //Clear PE4-PE7 so that stepper motor may move freely
}