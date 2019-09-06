// Copyright (c) 2013 Austin T. Clements. All rights reserved.
// Use of this source code is governed by an MIT license
// that can be found in the LICENSE file.

#include "elf++.hh"

#include <cstring>
#include <sstream>
#include <stdexcept>

using namespace std;

ELFPP_BEGIN_NAMESPACE

    template<template<typename E, byte_order Order> class Hdr>
    void canon_hdr(Hdr<Elf64, byte_order::native> *out, const void *data,
                   elfclass ei_class, elfdata ei_data) {
        switch (ei_class) {
            case elfclass::_32:
                switch (ei_data) {
                    case elfdata::lsb:
                        out->from(*(Hdr<Elf32, byte_order::lsb> *) data);
                        break;
                    case elfdata::msb:
                        out->from(*(Hdr<Elf32, byte_order::msb> *) data);
                        break;
                }
                break;
            case elfclass::_64:
                switch (ei_data) {
                    case elfdata::lsb:
                        out->from(*(Hdr<Elf64, byte_order::lsb> *) data);
                        break;
                    case elfdata::msb:
                        out->from(*(Hdr<Elf64, byte_order::msb> *) data);
                        return;
                }
        }
    }

//////////////////////////////////////////////////////////////////
// class elf
//

    struct elf::impl {
        impl(const shared_ptr<loader> &l)
                : l(l) {}

        const shared_ptr<loader> l;
        Ehdr<> hdr;
        vector<section> sections;
        vector<segment> segments;

        section invalid_section;
        segment invalid_segment;
    };

    elf::elf(const std::shared_ptr<loader> &l)
            : m(make_shared<impl>(l)) {
        // Read the first six bytes to check the magic number, ELF
        // class, and byte order.
        struct core_hdr {
            char ei_magic[4];
            elfclass ei_class;
            elfdata ei_data;
            unsigned char ei_version;
        } *core_hdr = (struct core_hdr *) l->load(0, sizeof *core_hdr);

        // Check basic header
        if (strncmp(core_hdr->ei_magic, "\x7f" "ELF", 4) != 0)
            throw format_error("bad ELF magic number");
        if (core_hdr->ei_version != 1)
            throw format_error("unknown ELF version");
        if (core_hdr->ei_class != elfclass::_32 &&
            core_hdr->ei_class != elfclass::_64)
            throw format_error("bad ELF class");
        if (core_hdr->ei_data != elfdata::lsb &&
            core_hdr->ei_data != elfdata::msb)
            throw format_error("bad ELF data order");

        // Read in the real header and canonicalize it
        size_t hdr_size = (core_hdr->ei_class == elfclass::_32 ?
                           sizeof(Ehdr<Elf32>) : sizeof(Ehdr<Elf64>));
        const void *hdr = l->load(0, hdr_size);
        canon_hdr(&m->hdr, hdr, core_hdr->ei_class, core_hdr->ei_data);

        // More checks
        if (m->hdr.version != 1)
            throw format_error("bad section ELF version");
        if (m->hdr.shnum && m->hdr.shstrndx >= m->hdr.shnum)
            throw format_error("bad section name string table index");

        // Load segments
        const void *seg_data = l->load(m->hdr.phoff,
                                       m->hdr.phentsize * m->hdr.phnum);
        for (unsigned i = 0; i < m->hdr.phnum; i++) {
            const void *seg = ((const char *) seg_data) + i * m->hdr.phentsize;
            m->segments.push_back(segment(*this, seg));
        }

        // Load sections
        const void *sec_data = l->load(m->hdr.shoff,
                                       m->hdr.shentsize * m->hdr.shnum);
        for (unsigned i = 0; i < m->hdr.shnum; i++) {
            const void *sec = ((const char *) sec_data) + i * m->hdr.shentsize;
            // XXX Circular reference.  Maybe this should be
            // constructed on the fly?  Canonicalizing the header
            // isn't super-cheap.
            m->sections.push_back(section(*this, sec));
        }
    }

    const Ehdr<> &
    elf::get_hdr() const {
        return m->hdr;
    }

    shared_ptr<loader>
    elf::get_loader() const {
        return m->l;
    }

    const std::vector<section> &
    elf::sections() const {
        return m->sections;
    }

    const std::vector<segment> &
    elf::segments() const {
        return m->segments;
    }

    const section &
    elf::get_section(const std::string &name) const {
        for (auto &sec : sections())
            if (name == sec.get_name(nullptr))
                return sec;
        return m->invalid_section;
    }

    const section &
    elf::get_section(unsigned index) const {
        if (index >= sections().size())
            return m->invalid_section;
        return sections().at(index);
    }

    const segment &
    elf::get_segment(unsigned index) const {
        if (index >= segments().size())
            return m->invalid_segment;
        return segments().at(index);
    }

    size_t elf::get_symtab_entry_size() const {
        if (get_hdr().ei_class == elfclass::_32)
            return sizeof(Sym<Elf32>);
        else
            return sizeof(Sym<Elf64>);
    }

    bool operator==(const elf &a, const elf &b) {
        return a.get_loader() == b.get_loader();
    }

    bool operator!=(const elf &a, const elf &b) {
        return a.get_loader() != b.get_loader();
    }

//////////////////////////////////////////////////////////////////
// class segment
//

    struct segment::impl {
        impl(const elf &f)
                : f(f) {}

        const elf f;
        Phdr<> hdr;
        //  const char *name;
        //  size_t name_len;
        const void *data;
    };

    segment::segment(const elf &f, const void *hdr)
            : m(make_shared<impl>(f)), f(f) {
        canon_hdr(&m->hdr, hdr, f.get_hdr().ei_class, f.get_hdr().ei_data);
    }

    const Phdr<> &
    segment::get_hdr() const {
        return m->hdr;
    }

    const void *
    segment::data() const {
        if (!m->data)
            m->data = m->f.get_loader()->load(m->hdr.offset,
                                              m->hdr.filesz);
        return m->data;
    }

    size_t
    segment::file_size() const {
        return m->hdr.filesz;
    }

    size_t
    segment::mem_size() const {
        return m->hdr.memsz;
    }

//////////////////////////////////////////////////////////////////
// class section
//

    std::string
    enums::to_string(shn v) {
        if (v == shn::undef)
            return "undef";
        if (v == shn::abs)
            return "abs";
        if (v == shn::common)
            return "common";
        return std::to_string(v);
    }

    struct section::impl {
        impl(const elf &f)
                : f(f), name(nullptr), data(nullptr) {}

        const elf f;
        Shdr<> hdr;
        const char *name;
        size_t name_len;
        const void *data;
    };

    section::section(const elf &f, const void *hdr)
            : m(make_shared<impl>(f)), f(f) {
        canon_hdr(&m->hdr, hdr, f.get_hdr().ei_class, f.get_hdr().ei_data);
    }

    const Shdr<> &
    section::get_hdr() const {
        return m->hdr;
    }

    const char *
    section::get_name(size_t *len_out) const {
        // XXX Should the section name strtab be cached?
        if (!m->name)
            m->name = m->f.get_section(m->f.get_hdr().shstrndx)
                    .as_strtab().get(m->hdr.name, &m->name_len);
        if (len_out)
            *len_out = m->name_len;
        return m->name;
    }

    string
    section::get_name() const {
        return get_name(nullptr);
    }

    const void *
    section::data() const {
        if (m->hdr.type == sht::nobits)
            return nullptr;
        if (!m->data)
            m->data = m->f.get_loader()->load(m->hdr.offset, m->hdr.size);
        return m->data;
    }

    size_t
    section::size() const {
        return m->hdr.size;
    }

    strtab
    section::as_strtab() const {
        if (m->hdr.type != sht::strtab)
            throw section_type_mismatch("cannot use section as strtab");
        return strtab(m->f, data(), size());
    }

    symtab
    section::as_symtab() const {
        if (m->hdr.type != sht::symtab && m->hdr.type != sht::dynsym)
            throw section_type_mismatch("cannot use section as symtab");
        return symtab(m->f, data(), size(),
                      m->f.get_section(get_hdr().link).as_strtab());
    }

    std::vector<rel>
    section::get_rels() const {
        if (m->hdr.type != sht::rel) {
            throw section_type_mismatch("section not a REL type");
        }

        std::vector<rel> result;
        uintptr_t curr = (uintptr_t) data();
        uintptr_t end = (uintptr_t) ((uintptr_t) data() + size());
        while (curr < end) {
            rel rel_entry((void *) curr, *this);
            result.emplace_back(rel_entry);
            curr += rel_entry.size();
        }

        return result;
    }

    std::vector<rela>
    section::get_relas() const {
        if (m->hdr.type != sht::rela) {
            throw section_type_mismatch("section not a RELA type");
        }

        std::vector<rela> result;
        uintptr_t curr = (uintptr_t) data();
        uintptr_t end = (uintptr_t) ((uintptr_t) data() + size());
        while (curr < end) {
            rela rela_entry((void *) curr, *this);
            result.emplace_back(rela_entry);
            curr += rela_entry.size();
        }
        return result;
    }

//////////////////////////////////////////////////////////////////
// class strtab
//

    struct strtab::impl {
        impl(const elf &f, const char *data, const char *end)
                : f(f), data(data), end(end) {}

        const elf f;
        const char *data, *end;
    };

    strtab::strtab(elf f, const void *data, size_t size)
            : m(make_shared<impl>(f, (const char *) data,
                                  (const char *) data + size)) {
    }

    const char *
    strtab::get(Elf64::Off offset, size_t *len_out) const {
        const char *start = m->data + offset;

        if (start >= m->end)
            throw range_error("string offset " + std::to_string(offset) +
                              " exceeds section size");

        // Find the null terminator
        const char *p = start;
        while (p < m->end && *p)
            p++;
        if (p == m->end)
            throw format_error("unterminated string");

        if (len_out)
            *len_out = p - start;
        return start;
    }

    std::string
    strtab::get(Elf64::Off offset) const {
        return get(offset, nullptr);
    }

    const elf &strtab::get_elf() const {
        return m->f;
    }

//////////////////////////////////////////////////////////////////
// class sym
//

    sym::sym(elf f, const void *data, strtab strs)
            : strs(strs) {
        canon_hdr(&this->data, data, f.get_hdr().ei_class, f.get_hdr().ei_data);
    }

    const char *
    sym::get_name(size_t *len_out) const {
        return strs.get(get_data().name, len_out);
    }

    std::string
    sym::get_name() const {
        return strs.get(get_data().name);
    }

    const elf &sym::get_elf() const {
        return strs.get_elf();
    }

    bool operator==(const sym &a, const sym &b) {
        return (a.get_name() == b.get_name() &&
                a.get_data().binding() != stb::local &&
                a.get_data().binding() != stb::local
        );
    }

    bool operator!=(const sym &a, const sym &b) {
        return !(a == b);
    }

    bool operator<(const sym &a, const sym &b) {
        return (a.get_name() < b.get_name());
    }

    bool operator>(const sym &a, const sym &b) {
        return (a.get_name() > b.get_name());
    }

//////////////////////////////////////////////////////////////////
// class rela
//
    rela::rela(void *d, const section &s) :
            data(), s(s) {
        std::memcpy(&data.offset, d, sizeof(data.offset));
        std::memcpy(&data.info, (void *) ((char *) d + sizeof(data.offset)),
                    sizeof(data.info));
        std::memcpy(&data.addend, (void *) ((char *) d + sizeof(data.offset) +
                                            sizeof(data.info)),
                    sizeof(data.addend));
    }

//////////////////////////////////////////////////////////////////
// class rel
//

    rel::rel(void *d, const section &s) :
            data(), s(s) {
        std::memcpy(&data.offset, d, sizeof(data.offset));
        std::memcpy(&data.info, (void *) ((char *) d + sizeof(data.offset)),
                    sizeof(data.info));
    }

//////////////////////////////////////////////////////////////////
// class symtab
//

    struct symtab::impl {
        impl(const elf &f, const char *data, const char *end, strtab strs)
                : f(f), data(data), end(end), strs(strs) {}

        const elf f;
        const char *data, *end;
        const strtab strs;
    };

    symtab::symtab(elf f, const void *data, size_t size, strtab strs)
            : m(make_shared<impl>(f, (const char *) data,
                                  (const char *) data + size,
                                  strs)) {
    }

    sym symtab::get_sym(unsigned idx) const {
        size_t entry_size = get_elf().get_symtab_entry_size();

        if (m->data + idx * entry_size >= m->end) {
            std::stringstream err;
            err << "Index " << idx << " out of bounds";
            throw std::out_of_range(err.str());
        }

        return sym(m->f, m->data + idx * entry_size, m->strs);
    }

    symtab::iterator::iterator(const symtab &tab, const char *pos)
            : f(tab.m->f), strs(tab.m->strs), pos(pos) {
        stride = f.get_symtab_entry_size();
    }

    symtab::iterator
    symtab::begin() const {
        return iterator(*this, m->data);
    }

    symtab::iterator
    symtab::end() const {
        return iterator(*this, m->end);
    }

    const elf &symtab::get_elf() const {
        return m->f;
    }

ELFPP_END_NAMESPACE
