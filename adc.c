#define __MSP430G2252__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"

unsigned int* adc = 0;
unsigned int adc_avg[4];
unsigned char adc_sel;

/**
* Reads ADC 'chan' once using an internal reference, 'ref' determines if the
*   2.5V or 1.5V reference is used.
**/
//void Single_Measure_Temp()
//{
//	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
//}

/**
* ADC interrupt routine. Pulls CPU out of sleep mode for the main loop.
**/
interrupt(ADC10_VECTOR) adc10_isr(void)
{

	ADC10CTL0 &= ~ENC;

//	adc[0] = 0x56;
//	adc[1] = 0x78;

	//adc[0] = 0x7f & ADC10MEM;	// Saves measured value.
	//adc[1] = 0; //0xff & (ADC10MEM >> 8);

//	adc[3] = adc[2];
//	adc[2] = adc[1];
//	adc[1] = adc[0];

	if(adc_avg[adc_sel]) {
		adc_avg[adc_sel] = (unsigned int)ADC10MEM << 6;
	} else {
		adc_avg[adc_sel] -= adc_avg[adc_sel];
		adc_avg[adc_sel] += ADC10MEM;
	}

	// C = A * 0.0014663 / 0.00355 - 277.75
	//adc[0] = ((unsigned long)adc_avg[0] * 27069 - 18169625) >> 8;

	// C = A * 0.00002288853284504463 / 0.00355 - 277.75
	// C = A * 0.00644747404085764225 - 277.75
	// C = (A * 422.54165874164644272676 - 18202393) >> 16

	// V = A * 
	switch(adc_sel) {
		case 0: adc[adc_sel] = ((unsigned long)adc_avg[adc_sel] * 422 - 18169625) >> 8; break;
		// 1000 * V = 780 + (C - 25) * 2.8
		// 1000 * (1.5 * A>>16) = 780 + 2.8 * C - 70
		// 1500 * A>>16 = 710 + 2.8 * C
		// C = (1500 * A>>8 - 710<<8) / 2.8
		// C = (1500 * A>>8 - 181760) / 2.8
		// C = (535 * A>>8 - 64914)
		// C = (535 * A - 16618057) >> 8

		//case 1: adc[adc_sel] = (((535UL * ((unsigned long long)adc_avg[adc_sel])) >> 8) - 64914); break;
		case 1: adc[adc_sel] = (535UL * (unsigned long long)adc_avg[adc_sel] - 16618057) >> 8; break;
		case 3: ++adc[adc_sel]; break;
		default: adc[adc_sel] = (adc_avg[adc_sel] >> 1); break; //((adc_avg[adc_sel] >> 0) + (adc_avg[adc_sel] >> 1)) >> 8; break;
	}

	switch(adc_sel) {
		default:
			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_10;
			adc_sel = 0;
			break;
		case 0:
			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			//ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_0;
			adc_sel = 1;
			break;
		case 1:
			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			//ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_10;
			adc_sel = 0;
			break;
		case 2:
			ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_2;
			adc_sel = 3;
			break;
	}
	__delay_cycles (128);							// Delay to allow Ref to settle

	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
	//Single_Measure_Temp();
}

void Setup_ADC(unsigned int* buffer){

	ADC10CTL0 &= ~ENC;							// Disable ADC
//	ADC10CTL0 = SREF0 + ADC10SHT_2 + ADC10ON + REFON + REF2_5V;
//	ADC10CTL0 = SREF_1 + ADC10SHT_3 + REFON + ADC10ON + ref + ADC10IE;	// Use reference,
//	ADC10CTL0 = SREF0 + ADC10SHT_2 + ADC10ON + REFON + REF2_5V + ADC10IE;	// Use reference,
	ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;		// Use reference,
										//   16 clock ticks, internal reference on
										//   ADC On, enable ADC interrupt, Internal  = 'ref'
	ADC10CTL1 = ADC10SSEL_3 + INCH_10;					// Set 'chan', SMCLK
	__delay_cycles (128);							// Delay to allow Ref to settle
	adc_sel = 3;

	TACTL |= MC_1;
	TACTL |= TASSEL_1;
	TACTL |= TAIE;

	BCSCTL3 |= LFXT1S_2;

	adc = (unsigned int*)buffer;

	//Single_Measure_Temp();
	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
}

