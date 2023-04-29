/*
 * Fast_PWM.c
 *
 * Created: 09/03/2022 20:28:33
 * Author : Savii
 */ 

#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define TOP_VAL 1023.0
#define TOGGLE(X) X = (X ^ 0x01)
#define LEFT 1
#define RIGHT 0

volatile uint8_t* MEM_PORTB = (uint8_t*) (0x25);
volatile uint8_t* MEM_DDRB = (uint8_t*) (0x24);
volatile uint8_t* MEM_PINB = (uint8_t*) (0x23);

volatile uint8_t count = 0;
volatile uint8_t direct = RIGHT;

//Union created to avoid shift (>>) operations
typedef union 
{
	uint16_t FULLREG;
	
	struct 
	{
		uint8_t LOWREG;	
		uint8_t HIGHREG;
	};
} DUTYTOCOUNTREG;

inline void dutyToCount(uint8_t duty, DUTYTOCOUNTREG* res)
{
	(*res).FULLREG = ((TOP_VAL / 100.0)) * duty;
}

inline void updateDuty(uint8_t duty)
{
	DUTYTOCOUNTREG res;
	
	dutyToCount(duty, &res);
	
	if(duty <= 100)
	{
		switch(direct)
		{
			case RIGHT:
				OCR1AL = (duty > 0) ? res.LOWREG : 0;
				OCR1AH = (duty > 0) ? res.HIGHREG : 0;
				break;
			case LEFT:
				OCR1BL = (duty > 0) ? res.LOWREG : 0;
				OCR1BH = (duty > 0) ? res.HIGHREG : 0;
				break;
			default:
				break;
		}
	}
}

inline void startPWMTimer()
{ 
	TCNT1L = 0x00;
	TCNT1H = 0x00;
	TCCR1A = (direct == RIGHT) ? (1<<COM1A1) : (1<<COM1B1); //Clear on compare match, set at BOTTOM
	TCCR1B |= (1<<CS11); //CLK prescaler division by 8; 16M / 8 = 2M; PWM signal = 2M / 1024 = 1953,125Hz
	TCCR1A |= (1<<WGM10) | (1<<WGM11);
	TCCR1B |= (1<<WGM12); //This operation and the previous selects FAST PWM with resolution 10bits (1023 is TOP val)
	updateDuty(0); //Set PWM duty = 0
	TIMSK1 = (1<<TOIE1); //Enable timer overflow interrupt
}

inline void stopPWMTimer()
{
	TCNT1H = 0x00; //Reset count value
	TCNT1L = 0x00; //Reset count value
	updateDuty(0); //Reset PWM duty
	_delay_ms(1); //Delay needed to permit the reaching of the clear on compare match operation
	TCCR1B &= 0x00; //Prescaler = 0; stop timer
	TCCR1A &= 0x00;
	TIMSK1 &= 0x00; //Disable timer interrupts
}

inline void initButtonsINT()
{
	PCICR = (1<<PCIE0); //Enable Pin Change INT0 (Buttons are on PB0, PB1 and PB2)
	PCMSK0 = (1<<PCINT0) | (1<<PCINT1) | (1<<PCINT2); //Unmasking INT on pin 0, pin 1 and pin 2
}

ISR(TIMER1_OVF_vect)
{
	updateDuty(count);
}

ISR(PCINT0_vect)
{
	//Increment pushbutton (PB0)
	if((*(MEM_PINB) & 0x01) == 0 && count < 100)
		count++;
	//Decrement pushbutton (PB1)
	else if((*(MEM_PINB) & 0x02) == 0 && count > 0)
		count--;
	//Invert pushbutton (PB2)
	else if((*(MEM_PINB) & 0x04) == 0)
	{
		count = 0;
		
		stopPWMTimer();
		direct = TOGGLE(direct);
		startPWMTimer();
	}
	
	*(MEM_PORTB) &= 0b01100111;
	*(MEM_PORTB) |= (direct == RIGHT) ?  0b00010000 : 0b00001000;
}

int main()
{
	//Array of function pointers used to call the two functions.
	void (*fncPoint[2]) () = {&initButtonsINT, &startPWMTimer};
	
	*(MEM_PORTB) |= 0b00000111; //Enable pull-up resistors on PB0, PB1, PB2
	*(MEM_DDRB) |= 0b00011000; //PB3 and PB4 enabled as output
	
	fncPoint[0]();
	fncPoint[1]();
	
	//Previous is the same as:
	//initButtonsINT();
	//startPWMTimer();
	
	*(MEM_PORTB) |= (direct == RIGHT) ? 0b00010000 : 0b00001000; //Initialize first rotation signalization LED
	
	sei();
	
    /* Replace with your application code */
    while(1) 
    {
		
    }
}

