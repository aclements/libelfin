CXXFLAGS+=-g -O2 -Werror
override CXXFLAGS+=-std=c++0x -Wall
CPPFLAGS+=-I../dwarf -I../elf

CLEAN :=

all: dump-sections dump-syms dump-tree dump-lines find-pc

dump-sections: dump-sections.o ../elf/libelf++.a
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
dump-sections.o: ../elf/elf++.hh
CLEAN += dump-sections dump-sections.o

dump-syms: dump-syms.o ../elf/libelf++.a
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
dump-syms.o: ../elf/elf++.hh
CLEAN += dump-syms dump-syms.o

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
