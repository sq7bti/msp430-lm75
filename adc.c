#define __MSP430G2452__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "adc.h"

unsigned int* adc = 0;
unsigned int adc_avg[4], adc_temp;
unsigned char adc_sel;
unsigned char* pwm_control;
unsigned int* threshold_control;
unsigned int keep_on = 0;

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

		case 0: adc_temp = ((unsigned long long)adc_avg[adc_sel] * 422 - 18169625) >> 8; break;

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

		// CPU #1 QAG9930A (pizza box) R = 2.8883 * C + 725.36
		// CPU #2 QAU0003A             R = 2.9283 * C + 737.83
		// CPU #3 QAQ9949A (desktop)   R = 2.75 * C + 695.86

		// processor #1 with 1.102mA
		// R = 2.9283 * C + 737.83
		// C = (R - 737.83) / 2.9283

		// V = R * I = R * 1.102mA
		// R = 1000 * V / 1.102mA
		// V = 1.5 * A >> 16
		// R = 1500 * A >> 16 / 1.102
		// C = (1500 * A >> 16 / 1.102) - 737.83) / 2.9283
		// C = A >> 16 * 464.82994 - 251.965304

		// C = (A * 464.82994 - (253 << 16)) >> 16
		// C = (A * 464.82994 - 16512798.1695) >> 16
//		case 1: adc_temp = (((unsigned long long)adc_avg[adc_sel] * 465) - 16512798) >> 8; break;

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
		case 1: adc_temp = ((((unsigned long long)adc_avg[adc_sel] * 495) - 16583229) >> 8) + threshold_control[1]; break;

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
//		case 1: adc_temp = (((unsigned long long)adc_avg[adc_sel] * 493) - 16583229) >> 8; break;

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
//		case 1: adc_temp = (((unsigned long long)adc_avg[adc_sel] * 420) - 16458537) >> 8; break;

		// processor #1 with 1.232mA
		// R = 2.8883 * C + 725.36
		// C = (R - 725.36) / 2.8883

		// V = R * I = R * 1.232mA
		// R = 1000 * V / 1.232mA
		// V = 1.5 * A >> 16
		// R = 1500 * A >> 16 / 1.232
		// C = (1500 * A >> 16 / 1.232) - 725.36) / 2.8883

		// C = A >> 16 * 421.54 - 251
		// C = (A * 421.54 - (251 << 16)) >> 16
		// C = (A * 421.54 - 16458537.18796) >> 16
//		case 1: adc_temp = (((unsigned long long)adc_avg[adc_sel] * 422) - 16458537) >> 8; break;

		default: adc_temp = (adc_avg[adc_sel] >> 1); break;
	}

	adc[adc_sel] = adc_temp;

// usable values 9 (full scale) to 1 (no propotional control)
#define PROPPWM 9
// PWM hystheresis range (range of temperature across which PWM changes from min to max) - expressed in log2(delta_temp)
// e.g. 2 -> 4C, 3 -> 8C, 4 -> 16C
#define PROPRANGE 2
	switch(adc_sel) {
		default:
			if(*pwm_control) {
				if(adc_temp > threshold_control[0]) {
					keep_on = 0x0FFF; //1 << 16;
					if(adc_temp > threshold_control[0] + (1 << (8 + PROPRANGE))) {
						CCR1 = 512;
					} else {
						// for 32 degrees PWM should increase by 256
						CCR1 = (512 - (1 << PROPPWM)) + ( (adc_temp - threshold_control[0] ) >> (8 + PROPRANGE - PROPPWM));
						//CCR1 = (512 - (1 << 7)) + ( (adc_temp - (*threshold_control) ) >> 5);
						//CCR1 = (512 - (1 << 6)) + ( (adc_temp - (*threshold_control) ) >> 6);
						//CCR1 = (512 - (1 << 5)) + ( (adc_temp - (*threshold_control) ) >> 7);
						//CCR1 = (512 - (1 << 4)) + ( (adc_temp - (*threshold_control) ) >> 8);
						//CCR1 = (512 - (1 << 3)) + ( (adc_temp - (*threshold_control) ) >> 9);
						//CCR1 = (512 - (1 << 2)) + ( (adc_temp - (*threshold_control) ) >> 10);
					}
				} else {
					if(keep_on) {
						--keep_on;
					} else {
						CCR1 = 0;
					}
				}
				adc[2] = (CCR1 < 511)?CCR1:511;
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
			break;
		case 3:
			ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;
			ADC10CTL1 = ADC10SSEL_3 + INCH_5;
			adc_sel = 3;
			break;*/
	}
	__delay_cycles (128);							// Delay to allow Ref to settle

	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
}

void Setup_ADC(unsigned int* buffer, unsigned char* ctrl, unsigned int* thr){

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

	BCSCTL3 |= LFXT1S_2;

	adc = buffer;
	pwm_control = ctrl;
	threshold_control = thr;

	ADC10CTL0 |= ENC + ADC10SC; 				// Enable and start conversion
}
