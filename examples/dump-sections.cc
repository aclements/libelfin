#include "elf++.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
        if (argc != 2) {
                fprintf(stderr, "usage: %s elf-file\n", argv[0]);
                return 2;
        }

        int fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
                fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
                return 1;
        }

        elf::elf f(elf::create_mmap_loader(fd));
        int i = 0;
        printf("  [Nr] %-16s %-16s %-16s %s\n",
               "Name", "Type", "Address", "Offset");
        printf("       %-16s %-16s %-15s %5s %4s %5s\n",
               "Size", "EntSize", "Flags", "Link", "Info", "Align");
        for (auto &sec : f.sections()) {
                auto &hdr = sec.get_hdr();
                printf("  [%2d] %-16s %-16s %016lx %08lx\n", i++,
                       sec.get_name().c_str(),
                       to_string(hdr.type).c_str(),
                       (unsigned long)hdr.addr, (unsigned long)hdr.offset);
                printf("       %016zx %016lx %-15s %5s %4d %5lu\n",
                       sec.size(), (unsigned long)hdr.entsize,
                       to_string(hdr.flags).c_str(), to_string(hdr.link).c_str(),
                       (int)hdr.info, (unsigned long)hdr.addralign);
        }

        return 0;
}
