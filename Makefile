# a minimalistic Makefile to build bpi_ledset on Linux
# can be used with buildroot environment for example

CC=$(CROSS_COMPILE)gcc
CCPP=$(CROSS_COMPILE)g++

all: monitor_IR monitor_DH11 pwm_transition monitor_mhz19

monitor_IR:
	$(CC) -Os -Wall monitor_IR_sensor.c -o monitor_IR_sensor -lwiringPi

monitor_DH11:
	$(CC) -Os -Wall monitor_dh11_sensor.c -o monitor_DH11 -lwiringPi

monitor_mhz19:
	$(CC) -Os -Wall monitor_mhz19_sensor.c -o monitor_mhz19 -lwiringPi

pwm_transition:
	$(CC) -Os -Wall pwm_transition.c -o pwm_transition -lwiringPi -lpthread

clean:
	rm -f monitor_IR_sensor monitor_DH11 pwm_transition
