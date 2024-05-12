ifeq ($(OS),Windows_NT)
    TARGET_EXEC = out.exe
	EMPTY :=
	SEPARATOR := \$(EMPTY)
	RM = del 
else
    #UNAME_S := $(shell uname -s)
    TARGET_EXEC = out.o
	RM = rm 
	SEPARATOR := /
endif

CC = gcc			# compiler to use

#  -D _POSIX_C_SOURCE=200809L
# gcc -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework CoreFoundation -o test *.c /usr/local/lib/libportaudio.a -lm -pthread
LINKERFLAG = -lm -pthread /usr/local/lib/libportaudio.a -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework CoreFoundation
OPTIMIZATION = -O1

SRC_C  = $(wildcard *.c)
SRC_C += $(wildcard models/*.c)

SRC_H  = $(wildcard *.h)


$(TARGET_EXEC): $(SRC_C) $(SRC_H)
	gcc $(SRC_C) -o $@ $(CCFLAGS) $(LINKERFLAG) $(OPTIMIZATION)

.PHONY: clean
clean: 
	$(RM).$(SEPARATOR)$(TARGET_EXEC)

.PHONY: execute
execute: $(TARGET_EXEC)
	.$(SEPARATOR)$(TARGET_EXEC)

