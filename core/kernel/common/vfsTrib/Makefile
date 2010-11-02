.PHONY=
DELIVERABLE=../../../vfsTrib.a
OBJFILES=vfsTrib.o vfsTypes.o vfsTrib_traverse.o vfsTrib_tree.o \
	vfsTrib_folder.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

