SOURCES = zeropage.s65 tools.s65 sid.s65 string.s65 lcd.s65 keys.s65 main.s65

%.o: %.s65
	ca65 -o $@ -l $(@:.o=.lst) $<

all: firmware

firmware: $(SOURCES:.s65=.o)
	cl65 -C firmware.cfg -t none -o $@ $^

clean:
	rm -f firmware *.o *.lst
