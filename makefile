CC=gcc
CFLAGS=
LIB= -lfreetype -lwiringPi -lrt
INCLUDE= -I/usr/include/freetype2

all:thermostat

thermostat: checkbin thermostat.o XY_433.o
		$(CC)$(CFLAGS) build/thermostat.o build/XY_433.o $(LIB) -o bin/thermostat

thermostat.o: checkbuild source/thermostat.c
		$(CC)$(CFLAGS) -c source/thermostat.c $(INCLUDE) -o build/thermostat.o

XY_433.o: source/XY_433.c
		$(CC)$(CFLAGS) -c source/XY_433.c -o build/XY_433.o

clean:
	@rm -rf build/*
	@rm -rf bin/*

checkbuild:
	@mkdir -p build/

checkbin:
	@mkdir -p bin/