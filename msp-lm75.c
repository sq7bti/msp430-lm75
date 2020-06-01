//******************************************************************************
//
//                  Slave                      Master
//               MSP430G2452
//             -----------------          -----------------
//            |                 |        |                 |
//            |         SDA/P1.7|------->|SDA              |
//            |         SCL/P1.6|<-------|SCL              |
//            |                 |        |                 |
//            |                 |         -----------------
//            |                 |
//            |                 |         -----------------
//            |                 |        |                 |
//            |             P1.0|------->|  THERM          |
//            |                 |        |                 |
//             -----------------          -----------------
//
//******************************************************************************

#define __MSP430G2452__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"
#include "i2c_usi.h"
//#include "i2c_slave.h"

unsigned char display_buffer[4] = { 0x00, 0x00, 0x00, 0x00 };
unsigned int adc_buffer[4]; // = { 1 << 8, 2 << 8, 3 << 8, 4 << 8 };
unsigned char config = 0; //DISPLAY_CURR_3M + DISPLAY_CURR_6M + DISPLAY_CURR_12M;
unsigned char control = 0;
unsigned int threshold = (50 << 8);
unsigned int i;

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog
	if (CALBC1_1MHZ ==0xFF || CALDCO_1MHZ == 0xFF)
	{
		while(1);                               // If calibration constants erased
		// do not load, trap CPU!!
	}

	BCSCTL1 = CALBC1_16MHZ;                    // Set DCO
	DCOCTL = CALDCO_16MHZ;

	Setup_I2C((unsigned char*)&display_buffer, (unsigned char*)&adc_buffer, (unsigned char*)&config);

	Setup_ADC(adc_buffer, &control, &threshold);

	P2DIR |= BIT6;             // P2.6 to output
	//P2OUT |= BIT6;             // P2.6 to output
	P2SEL |= BIT6;             // P2.6 to TA0.1
	P2SEL &= ~BIT7;             // P2.6 to TA0.1
	P2SEL2 &= ~(BIT6|BIT7);             // P2.6 to TA0.1

	CCR0 = 512-1;             // PWM Period ~31.25kHz
	CCTL1 |= OUTMOD_7; // + OUT;          // CCR1 reset/set
	CCR1 = 0;                // CCR1 PWM duty cycle
	CCTL2 |= OUTMOD_7;
	TACTL = TASSEL_2 | MC_1;   // SMCLK, up mode, interrupt enabled

	__eint();		// enable interrupts

	while(++CCR1 < 512) __delay_cycles(4000);

	for (i=0; i < 3*1024;i++) __delay_cycles(1000);

	while(--CCR1 > 2) __delay_cycles(1000);

	for (i=0; i < 1024;i++) __delay_cycles(2000);

	control = 1;

	TACTL |= TAIE;
	for (;;)
	{
		nop();
		WRITE_SR(GIE | CPUOFF);
		// here implement PWM control
		// if adc_buffer > low_range
	}
}

//ISR(TIMER0_A0,timer0_a3_isr)
interrupt(TIMER0_A1_VECTOR) timer0_a0_isr(void)
{
	++adc_buffer[2];
	TACTL &= ~TAIFG;
};
