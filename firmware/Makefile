C_SOURCES = debug.c readline.c variables.c basic.c main.c
ASM_SOURCES = zeropage.s65 interrupt.s65 startup.s65 utils.s65 sid.s65 acia.s65 led.s65 lcd.s65 keys.s65

# Compilation of C files
%.o: %.c
	cc65 --cpu 6502 -O -t none -o $(@:.o=.s) $<
	ca65 --cpu 6502 -o $@ -l $(@:.o=.lst) $(<:.c=.s)

# Compilation of assembler files
%.o: %.s65
	ca65 --cpu 6502 -o $@ -l $(@:.o=.lst) $<

# Default target
all: firmware

# Build the firmware binary
firmware: $(ASM_SOURCES:.s65=.o) $(C_SOURCES:.c=.o)
	cl65 -C firmware.cfg -m firmware.map -o $@ $^ cc65.lib

# Remove all generated files
clean:
	rm -f firmware *.s *.o *.lst *.map

# Rebuild the firmware and use minpro to burn the EEPROM
flash: clean all
	minipro -p at28c256 -w firmware
