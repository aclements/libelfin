// Copyright (c) 2013 Austin T. Clements. All rights reserved.
// Use of this source code is governed by an MIT license
// that can be found in the LICENSE file.

#ifndef _ELFPP_HH_
#define _ELFPP_HH_

#include "common.hh"
#include "data.hh"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

ELFPP_BEGIN_NAMESPACE

    class elf;

    class loader;

    class section;

    class strtab;

    class symtab;

    class segment;

    class rel;

    class rela;
// XXX Audit for binary compatibility

// XXX Segments, other section types

/**
 * An exception indicating malformed ELF data.
 */
    class format_error : public std::runtime_error {
    public:
        explicit format_error(const std::string &what_arg)
                : std::runtime_error(what_arg) {}

        explicit format_error(const char *what_arg)
                : std::runtime_error(what_arg) {}
    };

/**
 * An ELF file.
 *
 * This class is internally reference counted and efficiently
 * copyable.
 *
 * Raw pointers to ELF data returned by any method of this object or
 * any object derived from this object point directly into loaded
 * section data.  Hence, callers must ensure that the loader passed to
 * this file remains live as long as any such pointer is in use.
 * Keeping any object that can return such a pointer live is
 * sufficient to keep the loader live.
 */
    class elf {
    public:
        /**
         * Construct an ELF file that is backed by data read from the
         * given loader.
         */
        explicit elf(const std::shared_ptr<loader> &l);

        /**
         * Construct an ELF file that is initially not valid.  Calling
         * methods other than operator= and valid on this results in
         * undefined behavior.
         */
        elf() = default;

        elf(const elf &o) = default;

        elf(elf &&o) = default;

        elf &operator=(const elf &o) = default;

        friend bool operator==(const elf &a, const elf &b);

        friend bool operator!=(const elf &a, const elf &b);

        bool valid() const {
            return !!m;
        }

        /**
         * Return the ELF file header in canonical form (ELF64 in
         * native byte order).
         */
        const Ehdr<> &get_hdr() const;

        /**
         * Return the loader used by this file.
         */
        std::shared_ptr<loader> get_loader() const;

        /**
         * Return the segments in this file.
         */
        const std::vector<segment> &segments() const;

        /**
         * Return the segment at the given index. If no such segment
         * is found, return an invalid segment.
         */
        const segment &get_segment(unsigned index) const;

        /**
         * Return the sections in this file.
         */
        const std::vector<section> &sections() const;

        /**
         * Return the section with the specified name. If no such
         * section is found, return an invalid section.
         */
        const section &get_section(const std::string &name) const;

        /**
         * Return the section at the given index.  If no such section
         * is found, return an invalid section.
         */
        const section &get_section(unsigned index) const;

        /**
         * @brief Returns the size in bytes each symbol table entry takes up
         * in this ELF file
         * @return
         */
        size_t get_symtab_entry_size() const;

    private:
        struct impl;
        std::shared_ptr<impl> m;
    };

/**
 * An interface for loading sections of an ELF file.
 */
    class loader {
    public:
        virtual ~loader() {}

        /**
         * Load the requested file section into memory and return a
         * pointer to the beginning of it.  This memory must remain
         * valid and unchanged until the loader is destroyed.  If the
         * loader cannot satisfy the full request for any reason
         * (including a premature EOF), it must throw an exception.
         */
        virtual const void *load(off_t offset, size_t size) = 0;
    };

/**
 * An mmap-based loader that maps requested sections on demand.  This
 * will close fd when done, so the caller should dup the file
 * descriptor if it intends to continue using it.
 */
    std::shared_ptr<loader> create_mmap_loader(int fd);

/**
 * An exception indicating that a section is not of the requested type.
 */
    class section_type_mismatch : public std::logic_error {
    public:
        explicit section_type_mismatch(const std::string &what_arg)
                : std::logic_error(what_arg) {}

        explicit section_type_mismatch(const char *what_arg)
                : std::logic_error(what_arg) {}
    };

/**
 * An ELF segment.
 *
 * This class is internally reference counted and efficiently
 * copyable.
 */
    class segment {
    public:
        /**
         * Construct a segment that is initially not valid.  Calling
         * methods other than operator= and valid on this results in
         * undefined behavior.
         */
        segment() {}

        segment(const elf &f, const void *hdr);

        segment(const segment &o) = default;

        segment(segment &&o) = default;

        /**
         * Return true if this segment is valid and corresponds to a
         * segment in the ELF file.
         */
        bool valid() const {
            return !!m;
        }

        /**
         * Return the ELF section header in canonical form (ELF64 in
         * native byte order).
         */
        const Phdr<> &get_hdr() const;

        /**
         * Return this segment's data. The returned buffer will
         * be file_size() bytes long.
         */
        const void *data() const;

        /**
         * Return the on disk size of this segment in bytes.
         */
        size_t file_size() const;

        /**
         * Return the in-memory size of this segment in bytes.
         * Bytes between file_size() and mem_size() are implicitly zeroes.
         */
        size_t mem_size() const;

        /**
         * @brief Returns the ELF object containing this segment
         * @return
         */
        const elf &get_elf() const {
            return f;
        };

    private:
        struct impl;
        std::shared_ptr<impl> m;
        const elf f;
    };

/**
 * An ELF section.
 *
 * This class is internally reference counted and efficiently
 * copyable.
 */
    class section {
    public:
        /**
         * Construct a section that is initially not valid.  Calling
         * methods other than operator= and valid on this results in
         * undefined behavior.
         */
        section() {}

        section(const elf &f, const void *hdr);

        section(const section &o) = default;

        section(section &&o) = default;

        /**
         * Return true if this section is valid and corresponds to a
         * section in the ELF file.
         */
        bool valid() const {
            return !!m;
        }

        /**
         * Return the ELF section header in canonical form (ELF64 in
         * native byte order).
         */
        const Shdr<> &get_hdr() const;

        /**
         * Return this section's name.
         */
        const char *get_name(size_t *len_out) const;

        /**
         * Return this section's name.  The returned string copies its
         * data, so loader liveness requirements don't apply.
         */
        std::string get_name() const;

        /**
         * Return this section's data.  If this is a NOBITS section,
         * return nullptr.
         */
        const void *data() const;

        /**
         * Return the size of this section in bytes.
         */
        size_t size() const;

        /**
         * Return this section as a strtab.  Throws
         * section_type_mismatch if this section is not a string
         * table.
         */
        strtab as_strtab() const;

        /**
         * Return this section as a symtab.  Throws
         * section_type_mismatch if this section is not a symbol
         * table.
         */
        symtab as_symtab() const;

        /**
         * @brief Return list of REL entries.
         * @throws section_type_mismatch if this section is not of type REL
         */
        std::vector<rel> get_rels() const;

        /**
        * @brief Return list of RELA entries.
        * @throws section_type_mismatch if this section is not of type RELA
        */
        std::vector<rela> get_relas() const;

        /**
         * @brief Returns the ELF object containing this section
         * @return
         */
        const elf &get_elf() const {
            return f;
        };

    private:
        struct impl;
        std::shared_ptr<impl> m;
        const elf f;
    };

/**
* @brief Rel entry
*/
    class rel {
    public:
        rel(void *d, const section &s);

        /**
         * @brief Returns the index in symbol table this relocation references
         * @return
         */
        unsigned sym_idx() const {
            return data.info >> data.R_SYM_SHIFT();
        }

        /**
         * @brief Returns the relocation type for this relocation. Note, these
         * are processor dependent
         * @return
         */
        unsigned rel_type() const {
            return data.info & data.R_TYPE_MASK();
        }

        /**
         * @brief Returns the size in bytes this relocation entry takes up in
         * the ELF file
         * @return
         */
        size_t size() const {
            return sizeof(data.info) + sizeof(data.offset);
        }

        /**
         * @brief Returns the raw data this entry contains
         * @return
         */
        const Elf_Rel<> &get_data() const {
            return data;
        }

        /**
         * @brief Returns the section this relocation entry resides in
         * @return
         */
        const section &get_section() const {
            return s;
        }

    private:
        Elf_Rel<> data;
        const section s;
    };

    /**
    * @brief Rela entry
    */
    class rela {
    public:
        rela(void *d, const section &s);

        /**
        * @brief Returns the index in symbol table this relocation references
        * @return
        */
        unsigned sym_idx() const {
            return data.info >> data.R_SYM_SHIFT();
        }

        /**
         * @brief Returns the relocation type for this relocation. Note, these
         * are processor dependent
         * @return
         */
        unsigned rel_type() const {
            return data.info & data.R_TYPE_MASK();
        }

        /**
        * @brief Returns the size in bytes this relocation entry takes up in
        * the ELF file
        * @return
        */
        size_t size() const {
            return sizeof(data.info) + sizeof(data.offset) +
                   sizeof(data.addend);
        }

        /**
        * @brief Returns the raw data this entry contains
        * @return
        */
        const Elf_Rela<> &get_data() const {
            return data;
        }

        /**
         * @brief Returns the section this relocation entry resides in
         * @return
         */
        const section &get_section() const {
            return s;
        }

    private:
        Elf_Rela<> data;
        const section s;
    };

/**
 * A string table.
 *
 * This class is internally reference counted and efficiently
 * copyable.
 */
    class strtab {
    public:
        /**
         * Construct a strtab that is initially not valid.  Calling
         * methods other than operator= and valid on this results in
         * undefined behavior.
         */
        strtab() = default;

        strtab(elf f, const void *data, size_t size);

        bool valid() const {
            return !!m;
        }

        /**
         * Return the string at the given offset in this string table.
         * If the offset is out of bounds, throws std::range_error.
         * This is very efficient since the returned pointer points
         * directly into the loaded section, though this still
         * verifies that the returned string is NUL-terminated.
         */
        const char *get(Elf64::Off offset, size_t *len_out) const;

        /**
         * Return the string at the given offset in this string table.
         */
        std::string get(Elf64::Off offset) const;

        /**
         * @brief Returns the elf object containing this string table
         * @return
         */
        const elf &get_elf() const;

    private:
        struct impl;
        std::shared_ptr<impl> m;
    };

/**
 * A symbol from a symbol table.
 */
    class sym {
        const strtab strs;
        Sym<> data;

    public:
        sym(elf f, const void *data, strtab strs);

        /**
         * Return this symbol's raw data.
         */
        const Sym<> &get_data() const {
            return data;
        }

        /**
         * Return this symbol's name.
         *
         * This returns a pointer into the string table and, as such,
         * is very efficient.  If len_out is non-nullptr, *len_out
         * will be set the length of the returned string.
         */
        const char *get_name(size_t *len_out) const;

        /**
         * Return this symbol's name as a string.
         */
        std::string get_name() const;

        /**
         * @brief Returns the elf object containing this symbol
         * @return
         */
        const elf &get_elf() const;

        friend bool operator==(const sym &a, const sym &b);

        friend bool operator!=(const sym &a, const sym &b);

        friend bool operator<(const sym &a, const sym &b);

        friend bool operator>(const sym &a, const sym &b);
    };

/**
 * A symbol table.
 *
 * This class is internally reference counted and efficiently
 * copyable.
 */
    class symtab {
    public:
        /**
         * Construct a symtab that is initially not valid.  Calling
         * methods other than operator= and valid on this results in
         * undefined behavior.
         */
        symtab() = default;

        symtab(elf f, const void *data, size_t size, strtab strs);

        bool valid() const {
            return !!m;
        }

        /* Returns the symbol at the index provided, or raises
         * an out_of_range if the index is too large */
        sym get_sym(unsigned idx) const;

        class iterator {
            const elf f;
            const strtab strs;
            const char *pos;
            size_t stride;

            iterator(const symtab &tab, const char *pos);

            friend class symtab;

        public:
            sym operator*() const {
                return sym(f, pos, strs);
            }

            iterator &operator++() {
                return *this += 1;
            }

            iterator operator++(int) {
                iterator cur(*this);
                *this += 1;
                return cur;
            }

            iterator &operator+=(std::ptrdiff_t x) {
                pos += x * stride;
                return *this;
            }

            iterator &operator-=(std::ptrdiff_t x) {
                pos -= x * stride;
                return *this;
            }

            bool operator==(iterator &o) const {
                return pos == o.pos;
            }

            bool operator!=(iterator &o) const {
                return pos != o.pos;
            }
        };

        /**
         * Return an iterator to the first symbol.
         */
        iterator begin() const;

        /**
         * Return an iterator just past the last symbol.
         */
        iterator end() const;

        const elf &get_elf() const;

    private:
        struct impl;
        std::shared_ptr<impl> m;
    };

ELFPP_END_NAMESPACE

#endif
