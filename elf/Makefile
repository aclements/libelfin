CXXFLAGS+=-g -O2 -Werror
override CXXFLAGS+=-std=c++0x -Wall

SRCS := elf.cc mmap_loader.cc
HDRS := elf++.hh data.hh common.hh
CLEAN :=

libelf++.a: $(SRCS:.cc=.o)
	ar rcs $@ $^
CLEAN += elf++.a $(SRCS:.cc=.o)

$(SRCS:.cc=.o): $(HDRS)

clean:
	rm -f $(CLEAN)

.DELETE_ON_ERROR:
