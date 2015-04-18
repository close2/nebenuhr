NAME=nebenuhr
CC=avr-gcc
CXX=avr-gcc
OBJDUMP=avr-objdump
ATMEGA=atmega328
CFLAGS_DEBUG=-I ../libcl -Wall -mmcu=$(ATMEGA) -g3 -gdwarf-2
CXXFLAGS_DEBUG=-g3 -gdwarf-2 -I ../libcl -Wall -Wno-unused-function -mcall-prologues -mmcu=$(ATMEGA) -fno-threadsafe-statics -std=c++11 -fwhole-program
CFLAGS_RELEASE=-g -gdwarf-2 -I ../libcl -Os -Wall -Wno-unused-function -mcall-prologues -mmcu=$(ATMEGA)
CXXFLAGS_RELEASE=-g -gdwarf-2 -I ../libcl -Os -Wall -Wno-unused-function -mcall-prologues -mmcu=$(ATMEGA) -fno-threadsafe-statics -std=c++11 -fwhole-program
OBJ2HEX=avr-objcopy 
# FLASH_COMMAND	= avrdude -pm328 -c usbasp
FLASH_COMMAND	= avrdude -pm328 -c avrisp2 -P usb
# if usbprog can't connect try this (http://stackoverflow.com/questions/15313269/avrispmkii-connection-status-unknown-status-0x00):
# FLASH_COMMAND	= avrdude -pm328 -c avrisp2 -P usb -B 22
FLASH		= $(FLASH_COMMAND) -U flash:w:$(NAME).hex
FLASH_FAST	= $(FLASH_COMMAND) -V -U flash:w:$(NAME).hex
FLASH_EEP	= $(FLASH_COMMAND) -U eeprom:w:$(NAME).eep
FLASH_FUSE_8MHZ = avrdude -pm328 -c usbasp -U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m
FLASH_FUSE_1MHZ = avrdude -pm328 -c usbasp -U lfuse:w:0x62:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m
HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings

# avrsim config:
# avr-gcc -mmcu=atmega328 -c simavr_conf.c -o simavr_conf.o
# avr-objcopy -O binary --only-section=.mmcu simavr_conf.o simavr_conf.mmcu
# avr-objcopy --add-section .mmcu=simavr_conf.mmcu main.obj main_sim.obj

# get the calibration byte
#avrdude -pm48 -D -c usbasp -U cal:r:cal.tmp:r

.PHONY: debug release all clean clobber flash flashEep flashFast flashFuse8Mhz startSim

debug :
	echo CFLAGS = $(CFLAGS_DEBUG) > compile.flags; echo CXXFLAGS = $(CXXFLAGS_DEBUG) >> compile.flags

release :
	echo CFLAGS = $(CFLAGS_RELEASE) > compile.flags; echo CXXFLAGS = $(CXXFLAGS_RELEASE) >> compile.flags

-include compile.flags

all : $(NAME).hex $(NAME).eep $(NAME).obj

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.elf : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

%.eep : %.obj
	$(OBJ2HEX) $(HEX_EEPROM_FLAGS) -O ihex $< $@

%.s : %.obj
	$(OBJDUMP) -S --disassemble $< > $@

clean :
	rm -f $(NAME).o $(NAME).obj $(NAME).s *.sch~ gschem.log *.S~ *.hex *.map *.eep compile.flags

clobber : clean
	rm -f $(NAME).hex $(NAME).ps $(NAME).eep

flash : $(NAME).hex
	$(FLASH)

flashFast : $(NAME).hex
	$(FLASH_FAST)

flashEep : $(NAME).eep
	$(FLASH_EEP)

flashFuse8MHz :
	$(FLASH_FUSE_8MHZ)

startSim :
	./simavr.sh &
