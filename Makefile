# The name of your project (used to name the compiled .hex file)
# TARGET = $(nodir $CURDIR) 
TARGET = encode

# 2019/01/12 to build serial "debug" version
# $ make clean
# $ make coreserial
# $ make 
# $ make upload
# 
# 2019/01/12  to build the USB keyboard version
#  $ make clean
#  $ make corekeyboard
#  $ USB=keyboard; make keyboard
#  $ make upload

# Path to arduino installation, not used any more
# ARDUINOPATH = /home/john/opt/arduino-1.6.1
TEENSYPATH = /home/john/src/teensy
TEENSYCORE = /home/john/src/teensy/teensy3
INC = $(TEENSYPATH)/include
LIB = $(TEENSYPATH)/lib
CORELIB = $(LIB)/core.a

# path location for Teensy Loader, teensy_post_compile and teensy_reboot
# TOOLSPATH = $(ARDUINOPATH)/hardware/tools
TOOLSPATH = $(TEENSYPATH)/tools
COMPILERPATH = $(TOOLSPATH)/arm/bin

USBFLAG = -DUSB_SERIAL
ifeq ($(USB),keyboard) 
   USBFLAG=-DUSB_KEYBOARDONLY
endif

# compiler options for C++ only
CXXFLAGS = -c -O2 -g -Wall -ffunction-sections -fdata-sections -nostdlib -MMD -fno-exceptions -felide-constructors -std=gnu++14 -fno-rtti -mthumb -mcpu=cortex-m4 -fsingle-precision-constant -D__MK20DX256__ -DTEENSYDUINO=141 -DARDUINO=10805 -DF_CPU=96000000 -DLAYOUT_US_ENGLISH $(USBFLAG) 

# linker options
LDFLAGS = -O2 -Wl,--gc-sections,--relax,--defsym=__rtc_localtime=1520076245 -T$(INC)/mk20dx256.ld -lstdc++  -mthumb -mcpu=cortex-m4 -fsingle-precision-constant 

# libraries to link... my version of the library
LIBS = $(LIB)/core.a -larm_cortexM4l_math -lm

# names for the compiler programs
CC = $(abspath $(COMPILERPATH))/arm-none-eabi-gcc
CXX = $(abspath $(COMPILERPATH))/arm-none-eabi-g++
OBJCOPY = $(abspath $(COMPILERPATH))/arm-none-eabi-objcopy
OBJDUMP= $(abspath $(COMPILERPATH))/arm-none-eabi-objdump
SIZE = $(abspath $(COMPILERPATH))/arm-none-eabi-size

# automatically create lists of the sources and objects
# TODO: this does not handle Arduino libraries yet...
# C_FILES := $(wildcard *.c)
# CPP_FILES := $(wildcard *.cpp)
# OBJS := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o)

# .SECONDARY do not delete intermediate files
.SECONDARY:
# this invocation of make will be run serially
.NOTPARALLEL:

all: $(TARGET).hex

coreserial:
	cd $(TEENSYCORE) ; make

corekeyboard:
	cd $(TEENSYCORE) ; make USB=keyboard

keyboard:
	make USB=keyboard

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(INC) -o "$@" "$<"

%.o: %.c
	$(CC) $(CFLAGS) -I$(INC) -o "$@" "$<"

%.elf: %.o
	$(CC) $(LDFLAGS) -o "$@" "$<" -L$(LIB) $(LIBS)

upload: $(TARGET).hex
	$(abspath $(TOOLSPATH))/teensy_post_compile -file=$(TARGET) -path=$(shell pwd) -tools=$(abspath $(TOOLSPATH))
	-$(abspath $(TOOLSPATH))/teensy_reboot

dump: 
	$(OBJDUMP) -d $(TARGET).elf

# compiler generated dependency info
# -include $(OBJS:.o=.d)

clean:
	rm -f *.o *.d $(TARGET).elf $(TARGET).hex


# MAKEFILE NOTES
# $@  -- the file named on the left side of the :
# $<  -- the first item in the dependencies list
