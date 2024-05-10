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

LINKERFLAG = -lm -pthread

SRC_C  = $(wildcard *.c)
SRC_C += $(wildcard models/*.c)

SRC_H  = $(wildcard *.h)

$(TARGET_EXEC): $(SRC_C) $(SRC_H)
	gcc $(SRC_C) -o $@ $(CCFLAGS) $(LINKERFLAG)

.PHONY: clean
clean:
	$(RM).$(SEPARATOR)$(TARGET_EXEC)

execute:
	.$(SEPARATOR)$(TARGET_EXEC)

