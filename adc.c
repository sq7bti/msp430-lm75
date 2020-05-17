#define __MSP430G2452__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"

unsigned int* adc = 0;
unsigned int adc_avg[4];
unsigned char adc_sel;

/**
* ADC interrupt routine. Pulls CPU out of sleep mode for the main loop.
**/
interrupt(ADC10_VECTOR) adc10_isr(void)
{

	ADC10CTL0 &= ~ENC;

	if(!adc_avg[adc_sel]) {
		adc_avg[adc_sel] = ADC10MEM << 6;
	} else {
		adc_avg[adc_sel] -= adc_avg[adc_sel] >> 6;
		adc_avg[adc_sel] += ADC10MEM;
	}

	switch(adc_sel) {
		// A = 65535 * V / Vref
		// 1.5 * A >> 16 = V
		// 0.00355 * C + 0.986 = V
		// 1.5 * A >> 16 = 0.00355 * C + 0.986
		// (1.5 * A >> 16 - 0.986) / 0.00355 = C
		// C = 1.5 * A >> 16 / 0.00355 - 277.75
		// C << 8 = 448 * A >> 8 - 71104

		case 0: adc[adc_sel] = ((unsigned long long)adc_avg[adc_sel] * 422 - 18169625) >> 8; break;

		// C = (A * 27069 - 18169625) >> 16
		//case 0: adc[adc_sel] = ((unsigned long long)adc_avg[0] * 27069 - 18169625) >> 8; break;

		// 1000 * V = 780 + (C - 25) * 2.8
		// 1000 * (1.5 * A>>16) = 780 + 2.8 * C - 70
		// 1500 * A>>16 = 710 + 2.8 * C
		// C = (1500 * A>>8 - 710<<8) / 2.8
		// C = (1500 * A>>8 - 181760) / 2.8
		// C = (535 * A>>8 - 64914)
		// C = (535 * A - 16618057) >> 8

		//case 1: adc[adc_sel] = (((535UL * ((unsigned long long)adc_avg[adc_sel])) >> 8) - 64914); break;
		//case 1: adc[adc_sel] = (535UL * (unsigned long long)adc_avg[adc_sel] - 16618057) >> 8; break;

		// CPU #1 R = 2.8883 * C + 725.36
		// CPU #2 R = 2.9283 * C + 737.83
		// CPU #3 R = 2.75 * C + 695.86

		// processor #3 with 1.102mA
		// R = 2.75 * C + 695.86
		// C = (R - 695.86) / 2.75

		// V = R * I = R * 1.102mA
		// R = 1000 * V / 1.102mA
		// V = 1.5 * A >> 16
		// R = 1500 * A >> 16 / 1.102
		// C = (1500 * A >> 16 / 1.102) - 695.86) / 2.75
		// C = A >> 16 * 494.967 - 253
		// C = (A * 494.967 - (253 << 16)) >> 16
		// C = (A * 494.967 - 16583229) >> 16
//		case 1: adc[adc_sel] = (((unsigned long long)adc_avg[adc_sel] * 495) - 16583229) >> 8; break;

		// processor #3 with 1.107mA
		// R = 2.75 * C + 695.86
		// C = (R - 695.86) / 2.75

		// V = R * I = R * 1.107mA
		// R = 1000 * V / 1.107mA
		// V = 1.5 * A >> 16
		// R = 1500 * A >> 16 / 1.107
		// C = (1500 * A >> 16 / 1.107) - 695.86) / 2.75
		// C = A >> 16 * 492.7322 - 253
		// C = (A * 492.7322 - (253 << 16)) >> 16
		// C = (A * 492.7322 - 16583229) >> 16
		case 1: adc[adc_sel] = (((unsigned long long)adc_avg[adc_sel] * 493) - 16583229) >> 8; break;

		// processor #1 with 1.237mA
		// R = 2.8883 * C + 725.36
		// C = (R - 725.36) / 2.8883

		// V = R * I = R * 1.237mA
		// R = 1000 * V / 1.237mA
		// V = 1.5 * A >> 16
		// R = 1500 * A >> 16 / 1.237
		// C = (1500 * A >> 16 / 1.237) - 725.36) / 2.8883

		// C = A >> 16 * 419.83 - 251
		// C = (A * 419.83 - (251 << 16)) >> 16
		// C = (A * 419.83 - 16458537.18796) >> 16
//		case 1: adc[adc_sel] = (((unsigned long long)adc_avg[adc_sel] * 420) - 16458537) >> 8; break;

		// C = (1653 * A >> 16) - 695.86) / 2.75
		// C = (6001 * A >> 8) - 253 << 8
		// C = (545 * A >> 8) - 64788
		//case 1: adc[adc_sel] = (600UL * (unsigned long long)adc_avg[adc_sel] - 16583229) >> 8; break;

		//case 3: ++adc[adc_sel]; break;
		default: adc[adc_sel] = (adc_avg[adc_sel] >> 1); break;
	}

	switch(adc_sel) {
		default:
			if(adc[1] > (50 << 8)) {
				if(adc[1] > (76 << 8)) {
					CCR1 = 512;
				} else {
					// for 32 degrees PWM should increase by 256
					CCR1 = 384 + ((adc[1] - (50 << 8)) >> 5);
				}
			} else {
				CCR1 = 0;
			}

			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_10;
			adc_sel = 0;
			break;
		case 0:
			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_0;
			adc_sel = 1;
			break;
		/*case 1:
			ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + ADC10IE;
			//ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_10;
			adc_sel = 0;
			break;
		case 2:
			ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_2;
			adc_sel = 3;
			break;*/
	}
	__delay_cycles (128);							// Delay to allow Ref to settle

	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
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
	adc_sel = 0;

	TACTL |= MC_1;
	TACTL |= TASSEL_1;
	TACTL |= TAIE;

	BCSCTL3 |= LFXT1S_2;

	adc = (unsigned int*)buffer;

	//Single_Measure_Temp();
	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
}

