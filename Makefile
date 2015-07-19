F_CPU	= 2457600
DEVICE  = attiny4313
AVRDUDE = sudo avrdude -p attiny4313 -c linuxspi -P /dev/spidev0.0
CFLAGS  = 
OBJECTS = main.o

COMPILE = avr-gcc -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)


hex: main.hex

program: flash

flash: main.hex
	$(AVRDUDE) -U flash:w:main.hex:i

fuses:
	$(AVRDUDE) -U lfuse:w:0xca:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

clean:
	rm -f main.hex main.lst main.obj main.cof main.list main.map main.eep.hex main.elf *.o *.s

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
.c.s:
	$(COMPILE) -S $< -o $@


main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)

main.hex: main.elf
	rm -f main.hex main.eep.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size main.hex

disasm:	main.elf
	avr-objdump -d main.elf

cpp:
	$(COMPILE) -E main.c
