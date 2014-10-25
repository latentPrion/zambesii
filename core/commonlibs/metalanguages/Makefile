.PHONY=
DELIVERABLE=../../metalanguages.a
OBJFILES=udi_gio.o udi_bridge.o udi_mgmt.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean: fonyphile
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

