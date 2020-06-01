
#define __MSP430G2452__ 1
#include <msp430.h>

//depreciated: #include <signal.h>
#include <legacymsp430.h>

#include <isr_compat.h>

#include "i2c_usi.h"

//unsigned char MST_Data[4] = { 0x00, 0x00, 0x00, 0x00 };
unsigned char* MST_Data = 0;
unsigned char* SLV_Data = 0;
unsigned char* CFG = 0;

unsigned int I2C_State, Bytecount, cmd, SLV_Sel;     // State variables

#define SCL (P1IN & BIT6)
#define I2C_IDLE              0x00
#define I2C_RX_ADDRESS        0x01
#define I2C_PROCESS_ADDRESS   0x02
#define I2C_RX_CMD            0x03
#define I2C_RX_DATA           0x04
#define I2C_CMD_ACK           0x05
#define I2C_RX_ACK            0x06
#define I2C_TX_DATA           0x07
#define I2C_RECEIVE_DATA      0x08
#define I2C_CHECK_TXDATA      0x09

//******************************************************************************//
// USI interrupt service routine
// Rx bytes from master:
// State        2       ->          4          ->      6      ->     8      [ ->      6      ->     8      ]
//       I2C_RX_ADDRESS -> I2C_PROCESS_ADDRESS -> I2C_RX_DATA -> I2C_RX_ACK [ -> I2C_RX_DATA -> I2C_RX_ACK ]
// Tx bytes to Master: State 2->4->10->12->14
//******************************************************************************//
interrupt(USI_VECTOR) usi_i2c_txrx(void)
{
	if (USICTL1 & USISTTIFG) {                 // Start entry?
		I2C_State = I2C_RX_ADDRESS;                          // Enter 1st state on start
	}

	switch(I2C_State) {

	case I2C_RX_ADDRESS: // RX Address 0x02

		USICTL1 != USIIE;
		// USICNT = (USICNT & 0xE0) + 0x08; // Bit counter = 8, RX address
		// // USISCLREL | USI16B | USIIFGCC
		USICNT = (USICNT & 0xE0) | 0x08;               // Bit counter = 8, RX address

		USICTL1 &= ~USISTTIFG;        // Clear start flag
		I2C_State = I2C_PROCESS_ADDRESS; // Go to next state: check address
		Bytecount = 0;                // Reset counter for next TX/RX
		cmd = 0xff;                // Reset counter for next TX/RX
		break;

	case I2C_PROCESS_ADDRESS: // Process Address and send (N)Ack 0x04

		if ( (USISRL & 0xFC) == (I2C_Addr & 0xFC))       // Address match?
		{
			I2C_State = (USISRL & 0x01) ? I2C_TX_DATA : I2C_RX_CMD;
			SLV_Sel = (0x02 & USISRL) >> 1;
			USISRL = 0x00;                // Send Ack
			USICTL0 |= USIOE;             // SDA = output
			USICNT = (USICNT & 0xE0) | 0x01;               // Bit counter = 1, send (N)Ack bit
		} else
			I2C_State = I2C_IDLE;         // next state: prep for next Start
		break;

	case I2C_RX_CMD: // Receive command byte 0x06

		USICTL0 &= ~USIOE;             // SDA = input
		USICNT = (USICNT & 0xE0) | 0x08;               // Bit counter = 8, RX address
		I2C_State = I2C_CMD_ACK;  // next state: Test data and (N)Ack
		break;

	case I2C_CMD_ACK:// Check Data & TX (N)Ack 0x08

		cmd = USISRL;

		I2C_State = I2C_RX_DATA;    // Rcv another byte

		USISRL = 0x00;              // Send Ack
		USICTL0 |= USIOE;           // SDA = output
		USICNT = (USICNT & 0xE0) | 0x01;               // Bit counter = 1, send (N)Ack bit
		break;

	case I2C_RX_DATA: // Receive data byte 0x08

		I2C_State = I2C_RX_ACK;  // next state: Test data and (N)Ack

		USISRL = 0x00;              // Send Ack
		USICTL0 &= ~USIOE;             // SDA = input
		USICNT = (USICNT & 0xE0) | 0x08;               // Bit counter = 8, RX address
		break;

	case I2C_RX_ACK:// Check Data & TX (N)Ack 0x08

		if(!Bytecount)
			Bytecount = cmd;

		if (Bytecount <= 4 ) { // expected number of bytes // If not last byte
			if(Bytecount == 0)
				(*CFG) = USISRL;
			else
				MST_Data[0x3 & (Bytecount - 1)] = USISRL;
			Bytecount++;

			I2C_State = I2C_RX_DATA;    // Rcv another byte

			USICTL0 |= USIOE;           // SDA = output
			USISRL = 0x00;              // Send Ack
			USICNT = (USICNT & 0xE0) | 0x01;               // Bit counter = 1, send (N)Ack bit
		} else {                            // Last Byte
			USICTL0 |= USIOE;           // SDA = output
			USISRL = 0xFF;              // Send NAck
			USICTL0 &= ~USIOE;          // SDA = input
			I2C_State = I2C_IDLE;       // Reset state machine
			Bytecount = 0;              // Reset counter for next TX/RX
		}
		break;

	case I2C_TX_DATA: // Send Data byte 0x0A
		USICTL0 |= USIOE;             // SDA = output
		USISRL = SLV_Data[2*SLV_Sel+Bytecount++];
		USICNT = (USICNT & 0xE0) | 0x08;               // Bit counter = 8, RX address
		I2C_State = I2C_RECEIVE_DATA;               // Go to next state: receive (N)Ack
		break;

	case I2C_RECEIVE_DATA:// Receive Data (N)Ack 0x0C
		USICTL0 &= ~USIOE;                          // SDA = input
		USICNT = (USICNT & 0xE0) | 0x01;               // Bit counter = 1, send (N)Ack bit
		I2C_State = I2C_CHECK_TXDATA;               // Go to next state: check (N)Ack
		break;

	case I2C_CHECK_TXDATA:// Process Data Ack/NAck 0x0E
		if (USISRL & 0x01)                     // If Nack received...
		{
			USICTL0 &= ~USIOE;             // SDA = input
			I2C_State = I2C_IDLE;          // Reset state machine
			Bytecount = 0;
			// LPM0_EXIT;                  // Exit active for next transfer
		} else {                               // Ack received
			USICTL0 |= USIOE;              // SDA = output
			USISRL = SLV_Data[2*SLV_Sel+Bytecount++];
			USICNT = (USICNT & 0xE0) | 0x08;               // Bit counter = 8, RX address
			I2C_State = I2C_RECEIVE_DATA;  // Go to next state: receive (N)Ack
		}
		break;
	default:
	case I2C_IDLE:                               // Idle, should not get here
		break;
	}

	USICTL1 &= ~USIIFG;                       // Clear pending flags
}

	void Setup_I2C(unsigned char* led_buff, unsigned char* adc_buff,
						unsigned char* cfg) {

// USIPE7       *      (0x80)    /* USI  Port Enable Px.7 */
// USIPE6       *      (0x40)    /* USI  Port Enable Px.6 */
// USIPE5              (0x20)    /* USI  Port Enable Px.5 */
// USILSB              (0x10)    /* USI  LSB first  1:LSB / 0:MSB */
// USIMST              (0x08)    /* USI  Master Select  0:Slave / 1:Master */
// USIGE               (0x04)    /* USI  General Output Enable Latch */
// USIOE               (0x02)    /* USI  Output Enable */
// USISWRST     *      (0x01)    /* USI  Software Reset */
	USICTL0 = USIPE6+USIPE7+USISWRST;         // Port & USI mode setup

// USICKPH             (0x80)    /* USI  Sync. Mode: Clock Phase */
// USII2C       *      (0x40)    /* USI  I2C Mode */
// USISTTIE     *      (0x20)    /* USI  START Condition interrupt enable */
// USIIE        *      (0x10)    /* USI  Counter Interrupt enable */
// USIAL               (0x08)    /* USI  Arbitration Lost */
// USISTP              (0x04)    /* USI  STOP Condition received */
// USISTTIFG           (0x02)    /* USI  START Condition interrupt Flag */
// USIIFG              (0x01)    /* USI  Counter Interrupt Flag */
	USICTL1 = USII2C+USIIE+USISTTIE;          // Enable I2C mode & USI interrupts

// USIDIV2             (0x80)    /* USI  Clock Divider 2 */
// USIDIV1             (0x40)    /* USI  Clock Divider 1 */
// USIDIV0             (0x20)    /* USI  Clock Divider 0 */
// USISSEL2            (0x10)    /* USI  Clock Source Select 2 */
// USISSEL1            (0x08)    /* USI  Clock Source Select 1 */
// USISSEL0            (0x04)    /* USI  Clock Source Select 0 */
// USICKPL       *     (0x02)    /* USI  Clock Polarity 0:Inactive=Low / 1:Inactive=High */
// USISWCLK            (0x01)    /* USI  Software Clock */
	USICKCTL = USICKPL;                       // Setup clock polarity

// USISCLREL           (0x80)    /* USI  SCL Released */
// USI16B              (0x40)    /* USI  16 Bit Shift Register Enable */
// USIIFGCC      *     (0x20)    /* USI  Interrupt Flag Clear Control */
// USICNT4             (0x10)    /* USI  Bit Count 4 */
// USICNT3             (0x08)    /* USI  Bit Count 3 */
// USICNT2             (0x04)    /* USI  Bit Count 2 */
// USICNT1             (0x02)    /* USI  Bit Count 1 */
// USICNT0             (0x01)    /* USI  Bit Count 0 */
	USICNT = USIIFGCC;                       // Disable automatic clear control

	USICTL0 &= ~USISWRST;                     // Enable USI
	USICTL1 &= ~USIIFG;                       // Clear pending flag

	SLV_Data = adc_buff;
	CFG = (unsigned char*)cfg;
}
