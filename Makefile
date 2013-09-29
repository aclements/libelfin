all:
	$(MAKE) -C elf
	$(MAKE) -C dwarf

clean:
	$(MAKE) -C elf clean
	$(MAKE) -C dwarf clean
