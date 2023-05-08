/*
 * Fast_PWM.c
 *
 * Created: 09/03/2022 20:28:33
 * Author : Savii
 */ 

#define F_CPU 16000000U

#include <avr/io.h>
#include <avr/interrupt.h>
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
	//Same as performing / -> (TOP_VAL / 100.0) -> (1023 / 100.0); avoid usage of division
	(*res).FULLREG = (10.23) * duty;
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
				OCR1AL = res.LOWREG;
				OCR1AH = res.HIGHREG;
				break;
			case LEFT:
				OCR1BL = res.LOWREG;
				OCR1BH = res.HIGHREG;
				break;
			default:
				break;
		}
	}
}

inline void startPWMTimer()
{ 
	updateDuty(count); //Set PWM duty = 0
	TCNT1L = 0x00;
	TCNT1H = 0x00;
	TCCR1A = (direct == RIGHT) ? (1<<COM1A1) : (1<<COM1B1); //Clear on compare match, set at BOTTOM
	TCCR1B |= (1<<CS10) | (1<<CS11); //CLK prescaler division by 64; 16M / 64 = 250k; PWM signal = 250k / 1024 = 244Hz
	TCCR1A |= (1<<WGM10) | (1<<WGM11);
	TCCR1B |= (1<<WGM12); //This operation and the previous selects FAST PWM with resolution 10bits (1023 is TOP val)
	TIMSK1 = (1<<TOIE1); //Enable timer overflow interrupt
}

static inline void stopPWMTimer()
{	
	updateDuty(count);
	updateDuty(count);
	//Delay empirically estimated to give enough time to update the compare value .... ??
	_delay_ms(20);
	TCNT1L = 0x00;
	TCNT1H = 0x00;
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TIMSK1 = 0x00;
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
		direct = (direct == RIGHT) ? LEFT : RIGHT;
		startPWMTimer();
	}
}

int main()
{
	//Array of function pointers used to call the two functions.
	void (*fncPoint[2]) () = {&initButtonsINT, &startPWMTimer};
	
	*(MEM_DDRB) |= 0b01111000; //PB3 and PB4 enabled as output; PB5 and PB6 used as control pins for the motor
	*(MEM_PORTB) |= 0b00000111; //Enable pull-up resistors on PB0, PB1, PB2
	DDRD |= 0xFF; //Enable PORTD as output
	
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
		//Hundreds-related display; set to "1" in case of count == 100, OE PB7 is active low
		PORTD = 0b01100000;
		PORTD |= (count == 100) ? 0x01 : 0x00;
		_delay_ms(20);
		//Tens-related display; set to value 100 / 10, OE PB6 is active low
		PORTD = 0b10100000;
		PORTD |= (count < 100) ? ((uint16_t)count * 205) >> 11 : 0; //Divide by 10 
		_delay_ms(20);
		//Units-related display; set to value 100 modulo 10, OE PB5 is active low
		PORTD = 0b11000000;
		PORTD |= count % 10;
		_delay_ms(20);
		
		//Update of rotation signalization LEDs
		*(MEM_PORTB) &= 0b01100111;
		*(MEM_PORTB) |= (direct == RIGHT) ?  0b00010000 : 0b00001000;
    }
}

