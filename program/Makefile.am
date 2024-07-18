
bin_PROGRAMS = passerelle_modbus
passerelle_modbus_SOURCES = main.c server.c proxy.c log.c raw_request_with_tid.c

passerelle_modbus_CFLAGS += -lmodbus 

AM_CFLAGS = @passerelle_modbus_CFLAGS@
AM_LDFLAGS = @passerelle_modbus_LIBS@

CLEANFILES = *~ 
