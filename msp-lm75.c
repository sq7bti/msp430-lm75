//******************************************************************************//
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
//******************************************************************************//

#define __MSP430G2452__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"
#include "i2c_usi.h"

//unsigned int adc_data[4] = { 0, 0, 12, 0 };
// adc_data[0] = 0x48 - msp temp b[0,1]
// adc_data[1] = 0x49 - 060 temp b[2,3]
// adc_data[2] = 0x4a - PWM      b[4,5]
// adc_data[3] = 0x4b - tacho    b[6] = max, b[7] = current

union adc_data_t {
        unsigned int w[4];
	unsigned char b[8];
} adc_data;

unsigned char i2c_config = 0;
unsigned char control = 0;
unsigned int threshold[2] = { (57 << 8), (0 << 8) };
unsigned int i;
unsigned int tacho;
unsigned interval = 0;

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog
	if (CALBC1_1MHZ == 0xFF || CALDCO_1MHZ == 0xFF)
	{
		while(1);                               // If calibration constants erased
		// do not load, trap CPU!!
	}

	BCSCTL1 = CALBC1_16MHZ;                    // Set DCO
	DCOCTL  = CALDCO_16MHZ;

	P1OUT |=  0x02;                            // P1.1 set, else reset
	P1REN |=  0x02;                            // P1.1 pullup
	P1IE  |=  0x02;                             // P1.1 interrupt enabled
	P1IES |=  0x02;                            // P1.1 Hi/lo edge
	P1IFG &= ~0x02;                           // P1.1 IFG cleared

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

	Setup_ADC(adc_data.w, &control, (unsigned int*)&threshold);

	Setup_I2C(adc_data.w, (unsigned int*)&threshold, (unsigned char*)&i2c_config);

	P1IFG &= ~0x02;                           // P1.1 IFG cleared

	__eint();		// enable interrupts

	TACTL &= ~TAIFG;
//	TACTL |= TAIE;

	for (i=0; i < 3; i++)
		__delay_cycles(0xFFFF);

	while(++CCR1 < 512) __delay_cycles(1 << 9);

	for (i=0; i < 5; i++)
		__delay_cycles(0xFFFF);

	TACTL &= ~TAIFG;
repeat:
	tacho = 0;

	// 1/4 second delay = __delay_cycles(4000000);
	for (i=0; i < 350; i++)
		__delay_cycles(1000);

	if(tacho > 255)
		goto repeat;

	adc_data.b[7] = (0xFF & tacho); // store the max speed tacho for PWM=100%, in development setup it is a fan with approx 120Hz pulses at full speed, idle is approx 35Hz

	while(--CCR1 > 2) __delay_cycles(1 << 9);

	for (i=0; i < 3; i++)
		__delay_cycles(0xFFFF);

	control = 1;

	TACTL |= TAIE;

	for (;;)
	{
		nop();
		WRITE_SR(GIE | CPUOFF);
		// here implement PWM control
		// if adc_data > low_range
	}
}

//ISR(TIMER0_A0,timer0_a3_isr)
interrupt(TIMER0_A1_VECTOR) timer0_a0_isr(void)
{
	if(interval) {
		--interval;
	} else {
		adc_data.b[6] = tacho;
		tacho = 0;
		interval = 8500;
	}
	TACTL &= ~TAIFG;
};


// Port 1 interrupt service routine
interrupt(PORT1_VECTOR) Port_1 (void)
{
	++tacho;
	P1IFG &= ~0x02;                           // P1.1 IFG cleared
}
