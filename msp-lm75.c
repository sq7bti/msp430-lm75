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
//#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"
#include "i2c_usi.h"
//#include "i2c_slave.h"

unsigned char display_buffer[4] = { 0x00, 0x00, 0x00, 0x00 };
unsigned int adc_buffer[4]; // = { 1 << 8, 2 << 8, 3 << 8, 4 << 8 };
unsigned char config = 0; //DISPLAY_CURR_3M + DISPLAY_CURR_6M + DISPLAY_CURR_12M;

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

	Setup_ADC(adc_buffer);

	Setup_I2C((unsigned char*)&display_buffer, (unsigned char*)&adc_buffer, (unsigned char*)&config);

	P2DIR |= BIT6;             // P2.6 to output
	//P2OUT |= BIT6;             // P2.6 to output
	P2SEL |= BIT6;             // P2.6 to TA0.1
	P2SEL &= ~BIT7;             // P2.6 to TA0.1
	P2SEL2 &= ~(BIT6|BIT7);             // P2.6 to TA0.1

	CCR0 = 512-1;             // PWM Period
	CCTL1 |= OUTMOD_7; // + OUT;          // CCR1 reset/set
	CCR1 = 256;                // CCR1 PWM duty cycle
	CCTL2 |= OUTMOD_7;
	TACTL = TASSEL_2 + MC_1;   // SMCLK, up mode

	__eint();

	for (;;)
	{
		nop();
		WRITE_SR(GIE | CPUOFF);
		// here implement PWM control
		// if adc_buffer > low_range
	}
}
