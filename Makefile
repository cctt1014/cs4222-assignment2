CONTIKI_PROJECT = etimer-buzzer rtimer-lightSensor rtimer-IMUSensor buzz
all: $(CONTIKI_PROJECT)


CONTIKI = ../assignment/contiki
include $(CONTIKI)/Makefile.include