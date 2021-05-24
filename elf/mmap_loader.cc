// Copyright (c) 2013 Austin T. Clements. All rights reserved.
// Use of this source code is governed by an MIT license
// that can be found in the LICENSE file.

#include "elf++.hh"

#include <system_error>

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#include <Windows.h>
#endif

using namespace std;

ELFPP_BEGIN_NAMESPACE

class mmap_loader : public loader
{
        void *base;
        size_t lim;

public:
        mmap_loader(int fd)
        {
                // on windows, just read the file for now
                off_t end = lseek(fd, 0, SEEK_END);
                if (end == (off_t)-1)
                        throw system_error(errno, system_category(),
                                           "finding file length");
                lim = end;
#if __linux__
                base = mmap(nullptr, lim, PROT_READ, MAP_SHARED, fd, 0);
                if (base == MAP_FAILED)
                        throw system_error(errno, system_category(),
                                           "mmap'ing file");
#elif _WIN32
                base = VirtualAlloc(nullptr, lim, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                lseek(fd, 0, SEEK_SET);
                read(fd, base, lim);
#endif
                close(fd);
        }

        ~mmap_loader()
        {
#if __linux__
            munmap(base, lim);
#elif _WIN32
            VirtualFree(base, 0, MEM_RELEASE);
#endif
        }

        const void *load(off_t offset, size_t size)
        {
                if (offset + size > lim)
                        throw range_error("offset exceeds file size");
                return (const char*)base + offset;
        }
};

std::shared_ptr<loader>
create_mmap_loader(int fd)
{
        return make_shared<mmap_loader>(fd);
}

ELFPP_END_NAMESPACE
