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

	__eint();

	for (;;)
	{
		nop();
		WRITE_SR(GIE | CPUOFF);
	}
}
