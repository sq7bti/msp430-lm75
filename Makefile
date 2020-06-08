#CC - Contains the current C compiler. Defaults to cc. 
#CFLAGS - Special options which are added to the built-in C rule. (See next page.) 
#$@ - Full name of the current target. 
#$? - A list of files for current dependency which are out-of-date. 
#$< - The source file of the current (single) dependency. 

CC=msp430-gcc
#CFLAGS=-Os -mmcu=msp430g2252
CFLAGS=-mmcu=msp430g2452
#CFLAGS=-mmcu=msp430g2553
#USCICFLAGS=-Os -mmcu=msp430g2553
LIBS=#-lm
OBJECTS = msp-lm75.o adc.o
STRIP   = msp430-strip
SIZE    = msp430-size

msp-lm75: msp-lm75.o i2c_usi.o adc.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@
	#$(STRIP) $@
	#$(SIZE) $@

adc: adc.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

i2c_usi: i2c_usi.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

i2c_slave: i2c_slave.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	rm -rf msp-lm75.elf *.o *~
