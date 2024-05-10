ifeq ($(OS),Windows_NT)
    TARGET_EXEC = out.exe
else
    #UNAME_S := $(shell uname -s)
    TARGET_EXEC = out.o
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
	rm $(TARGET_EXEC)

