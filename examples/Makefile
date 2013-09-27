CXXFLAGS+=-g -O2 -Werror
override CXXFLAGS+=-std=c++0x -Wall
CPPFLAGS+=-I../dwarf -I../elf

CLEAN :=

all: dump-tree dump-lines find-pc

dump-tree: dump-tree.o ../dwarf/libdwarf++.a ../elf/libelf++.a
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
dump-tree.o: ../dwarf/dwarf++.hh ../elf/elf++.hh
CLEAN += dump-tree dump-tree.o

dump-lines: dump-lines.o ../dwarf/libdwarf++.a ../elf/libelf++.a
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
dump-lines.o: ../dwarf/dwarf++.hh ../elf/elf++.hh
CLEAN += dump-lines dump-lines.o

find-pc: find-pc.o ../dwarf/libdwarf++.a ../elf/libelf++.a
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
find-pc.o: ../dwarf/dwarf++.hh ../elf/elf++.hh
CLEAN += find-pc find-pc.o

../dwarf/libdwarf++.a: always
	$(MAKE) -C ../dwarf libdwarf++.a 

../elf/libelf++.a: always
	$(MAKE) -C ../elf libelf++.a 

.PHONY: always

clean:
	rm -f $(CLEAN)
