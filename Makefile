all: elf/libelf++.a dwarf/libdwarf++.a

elf/libelf++.a: always
	$(MAKE) -C elf libelf++.a 

dwarf/libdwarf++.a: always
	$(MAKE) -C dwarf libdwarf++.a 

.PHONY: always

clean:
	$(MAKE) -C elf clean
	$(MAKE) -C dwarf clean
