.PHONY=
DELIVERABLE=../../../vfsTrib.a
OBJFILES=vfsTrib.o \
#	vfsTrib_traverse.o vfsTrib_tree.o vfsTrib_folder.o \
#	vfsTrib_fileDesc.o vfsTrib_fileInode.o vfsTrib_dirDesc.o \
#	vfsTrib_dirInode.o vfsTrib_pathParse.o vfsTrib_file.o \
	hvfs_core.o

all: Makefile $(DELIVERABLE)

$(DELIVERABLE): $(OBJFILES)
	$(AR) -rcs $@ $(OBJFILES)

clean:
	rm -rf *.o *.s

fonyphile:
	rm -f clean aclean dirclean

