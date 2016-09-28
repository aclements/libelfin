CXXFLAGS+=-g -O2 -Werror
override CXXFLAGS+=-std=c++0x -Wall

CLEAN :=

all: dump-sections dump-segments dump-syms dump-tree dump-lines find-pc

# Find libs
export PKG_CONFIG_PATH+=:../elf:../dwarf
CPPFLAGS+=$$(pkg-config --cflags libelf++ libdwarf++)
# Statically link against our libs to keep the example binaries simple.
LDLIBS+=-Wl,-Bstatic $$(pkg-config --libs libelf++ libdwarf++) -Wl,-Bdynamic

# Dependencies
CPPFLAGS+=-MD -MP -MF .$@.d
-include .*.d

dump-sections: dump-sections.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += dump-sections dump-sections.o

dump-segments: dump-segments.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += dump-segments dump-segments.o

dump-syms: dump-syms.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += dump-syms dump-syms.o

dump-tree: dump-tree.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += dump-tree dump-tree.o

dump-lines: dump-lines.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += dump-lines dump-lines.o

find-pc: find-pc.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += find-pc find-pc.o

clean:
	rm -f $(CLEAN) .*.d
