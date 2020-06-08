# msp430-lm75

Source code for MSP430G2452 facilitating temperature measurement in https://github.com/sq7bti/ApolloVR
Implements LM75-like temperature sensor on address 0x48 and 0x49, for 060 and own die temperature

I2C slave fakes LM75 at addresses:
0x48 - internal temperature measured in MSP430g2452. Tends to overestimates as the internal heat dissipation heats up chip itself
0x49 - temperature measured on the basis of THERM sensor in Motorola 68060. Accuracy is heavily dependent on the calibration, as it can be off by major factor. See 68060_THERM.txt for example 3 pieces measured using an thermo-chamber.

Unit 0x49 reacts on Thyst register writes, and produces PWM control signal for fan connected to pin P2.6, with minimum PWM around 75%, and proportionally for the next 16C up to 100%.

0x4a I2C answers with 2 byte word of PWM, currently applied on Pin 2.6

Under address 0x4b tacho readout can be queried : first byte is a result of bootup check of PWM/tacho feedback (can be used to change PWM control), while second byte is current measurement.
