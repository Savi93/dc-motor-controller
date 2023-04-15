/*
 * Fast_PWM.c
 *
 * Created: 09/03/2022 20:28:33
 * Author : Savii
 */ 

#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>

#define TOP_VAL 1023.0

volatile uint8_t* MEM_PORTB = (uint8_t*) (0x25);
volatile uint8_t* MEM_DDRB = (uint8_t*) (0x24);
volatile uint8_t* MEM_PINB = (uint8_t*) (0x23);

volatile uint8_t count = 0;

inline uint16_t dutyToCount(uint8_t duty)
{
	uint16_t res = 0;
	res = ((TOP_VAL / 100.0)) * duty;
	return res;
}
inline void updateDuty(uint8_t duty)
{
	if(duty <= 100)
	{	
		OCR1AL = dutyToCount(duty);
		OCR1AH = dutyToCount(duty) >> 8;
	}
}

inline void startPWMTimer()
{ 
	TCNT1L = 0;
	TCNT1H = 0;
	TCCR1A = (1<<WGM10) | (1<<WGM11);
	TCCR1B = (1<<WGM12); //This operation and the previous selects FAST PWM with resolution 10bits (1023 is TOP val)
	TCCR1A |= (1<<COM1A1); //Clear on compare match, set at BOTTOM
	TCCR1B |= (1<<CS11); //CLK prescaler division by 8; 16M / 8 = 2M; PWM signal = 2M / 1024 = 1953,125Hz
	updateDuty(0); //Set PWM duty = 0
	TIMSK1 = (1<<TOIE1); //Enable timer overflow interrupt
}

inline void stopPWMTimer()
{
	TCCR1B &= ~(1<<CS11); //Prescaler = 0; stop timer
	updateDuty(0); //Reset PWM duty
	TCNT1L = 0; //Reset count value
	TCNT1H = 0; //Reset count value
}

inline void initButtonsINT()
{
	PCICR = (1<<PCIE0); //Enable Pin Change INT0 (Buttons are on PB0 and PB1)
	PCMSK0 = (1<<PCINT0) | (1<<PCINT1); //Un-mask INT on pin 0 and pin 1 
}

ISR(TIMER1_OVF_vect)
{
	updateDuty(count);
}

ISR(PCINT0_vect)
{
	if((*(MEM_PINB) & 1) == 0 && count < 100)
		count++;
	else if((*(MEM_PINB) & 2) == 0 && count > 0)
		count--;
}

int main()
{
	*(MEM_PORTB) |= 0b111; //Enable pull-up resistors on PB0, PB1, PB2
	
	initButtonsINT();
	startPWMTimer();
	sei();
	
    /* Replace with your application code */
    while (1) 
    {

    }
}

