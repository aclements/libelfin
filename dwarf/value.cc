// Copyright (c) 2013 Austin T. Clements. All rights reserved.
// Use of this source code is governed by an MIT license
// that can be found in the LICENSE file.

#include "internal.hh"

#include <cstring>

using namespace std;

DWARFPP_BEGIN_NAMESPACE

value::value(const unit *cu,
             const attribute_spec &spec, section_offset offset)
        : cu(cu),
          form(spec.form),
          typ(spec.type),
          offset(offset),
          has_implicit_const(spec.form == DW_FORM::implicit_const),
          implicit_const(spec.implicit_const) {
        if (form == DW_FORM::indirect)
                resolve_indirect(spec.name);
}

section_offset
value::get_section_offset() const
{
        return cu->get_section_offset() + offset;
}

taddr
value::as_address() const
{
        cursor cur(cu->data(), offset);

        if (form == DW_FORM::addr) {
                return cur.address();
        }

        // DWARF 5 address index forms
        uint64_t index;
        switch (form) {
        case DW_FORM::addrx:
                index = cur.uleb128();
                break;
        case DW_FORM::addrx1:
                index = cur.fixed<uint8_t>();
                break;
        case DW_FORM::addrx2:
                index = cur.fixed<uint16_t>();
                break;
        case DW_FORM::addrx3:
                index = cur.fixed<uint8_t>() | (cur.fixed<uint16_t>() << 8);
                break;
        case DW_FORM::addrx4:
                index = cur.fixed<uint32_t>();
                break;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as address");
        }

        // Look up address in .debug_addr section
        // DWARF 5 .debug_addr has a header: length (4 or 12 bytes), version (2), addr_size (1), segment_selector_size (1)
        auto addr_sec = cu->get_dwarf().get_section(section_type::addr);
        section_offset header_size = 8;  // Simplified: assume 32-bit DWARF (4 + 2 + 1 + 1)
        unsigned addr_size = cu->data()->addr_size;
        cursor addr_cur(addr_sec, header_size + index * addr_size);
        // Read address directly using the CU's addr_size (not the section's)
        if (addr_size == 4) {
                return addr_cur.fixed<uint32_t>();
        } else if (addr_size == 8) {
                return addr_cur.fixed<uint64_t>();
        } else {
                throw format_error("unsupported address size " + std::to_string(addr_size));
        }
}

const void *
value::as_block(size_t *size_out) const
{
        // XXX Blocks can contain all sorts of things, including
        // references, which couldn't be resolved by callers in the
        // current minimal API.
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::block1:
                *size_out = cur.fixed<uint8_t>();
                break;
        case DW_FORM::block2:
                *size_out = cur.fixed<uint16_t>();
                break;
        case DW_FORM::block4:
                *size_out = cur.fixed<uint32_t>();
                break;
        case DW_FORM::block:
        case DW_FORM::exprloc:
                *size_out = cur.uleb128();
                break;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as block");
        }
        cur.ensure(*size_out);
        return cur.pos;
}

uint64_t
value::as_uconstant() const
{
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::data1:
                return cur.fixed<uint8_t>();
        case DW_FORM::data2:
                return cur.fixed<uint16_t>();
        case DW_FORM::data4:
                return cur.fixed<uint32_t>();
        case DW_FORM::data8:
                return cur.fixed<uint64_t>();
        case DW_FORM::udata:
                return cur.uleb128();
        case DW_FORM::implicit_const:
                return static_cast<uint64_t>(implicit_const);
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as uconstant");
        }
}

int64_t
value::as_sconstant() const
{
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::data1:
                return cur.fixed<int8_t>();
        case DW_FORM::data2:
                return cur.fixed<int16_t>();
        case DW_FORM::data4:
                return cur.fixed<int32_t>();
        case DW_FORM::data8:
                return cur.fixed<int64_t>();
        case DW_FORM::sdata:
                return cur.sleb128();
        case DW_FORM::implicit_const:
                return implicit_const;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as sconstant");
        }
}

expr
value::as_exprloc() const
{
        cursor cur(cu->data(), offset);
        size_t size;
        // Prior to DWARF 4, exprlocs were encoded as blocks.
        switch (form) {
        case DW_FORM::exprloc:
        case DW_FORM::block:
                size = cur.uleb128();
                break;
        case DW_FORM::block1:
                size = cur.fixed<uint8_t>();
                break;
        case DW_FORM::block2:
                size = cur.fixed<uint16_t>();
                break;
        case DW_FORM::block4:
                size = cur.fixed<uint32_t>();
                break;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as exprloc");
        }
        return expr(cu, cur.get_section_offset(), size);
}

bool
value::as_flag() const
{
        switch (form) {
        case DW_FORM::flag: {
                cursor cur(cu->data(), offset);
                return cur.fixed<ubyte>() != 0;
        }
        case DW_FORM::flag_present:
                return true;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as flag");
        }
}

rangelist
value::as_rangelist() const
{
        // The compilation unit may not have a base address.  In this
        // case, the first entry in the range list must be a base
        // address entry, but we'll just assume 0 for the initial base
        // address.
        die cudie = cu->root();
        taddr cu_low_pc = cudie.has(DW_AT::low_pc) ? at_low_pc(cudie) : 0;
        auto cusec = cu->data();

        // DWARF 5 uses rnglistx form with .debug_rnglists section
        if (form == DW_FORM::rnglistx) {
                cursor cur(cu->data(), offset);
                uint64_t index = cur.uleb128();

                // Get .debug_rnglists section
                auto rnglists_sec = cu->get_dwarf().get_section(section_type::rnglists);

                // Parse the rnglists header to find the offsets table
                // Header format: unit_length (4/12), version (2), addr_size (1),
                // segment_selector_size (1), offset_entry_count (4)
                cursor hdr(rnglists_sec, (section_offset)0);

                // Read unit length to determine format
                uint32_t unit_length32 = hdr.fixed<uint32_t>();
                format fmt;
                section_offset header_size;
                if (unit_length32 == 0xffffffff) {
                        // 64-bit DWARF
                        hdr.fixed<uint64_t>(); // actual length
                        fmt = format::dwarf64;
                        header_size = 20; // 12 + 2 + 1 + 1 + 4
                } else {
                        fmt = format::dwarf32;
                        header_size = 12; // 4 + 2 + 1 + 1 + 4
                }

                uint16_t version = hdr.fixed<uint16_t>();
                (void)version; // Should be 5
                uint8_t addr_size = hdr.fixed<uint8_t>();
                (void)addr_size;
                uint8_t segment_selector_size = hdr.fixed<uint8_t>();
                (void)segment_selector_size;
                uint32_t offset_entry_count = hdr.fixed<uint32_t>();

                if (index >= offset_entry_count) {
                        throw format_error("rnglistx index out of bounds");
                }

                // Read the offset from the offsets table
                section_offset offset_size = (fmt == format::dwarf64) ? 8 : 4;
                cursor offsets_cur(rnglists_sec, header_size + index * offset_size);
                section_offset range_offset;
                if (fmt == format::dwarf64) {
                        range_offset = offsets_cur.fixed<uint64_t>();
                } else {
                        range_offset = offsets_cur.fixed<uint32_t>();
                }

                // The offset is relative to the first range list entry (after offsets table)
                section_offset base_offset = header_size + offset_entry_count * offset_size;
                section_offset absolute_offset = base_offset + range_offset;

                return rangelist(rnglists_sec, absolute_offset, cusec->addr_size, cu_low_pc, true);
        }

        // DWARF 4 and earlier: direct offset into .debug_ranges
        section_offset off = as_sec_offset();
        auto sec = cu->get_dwarf().get_section(section_type::ranges);
        return rangelist(sec, off, cusec->addr_size, cu_low_pc, false);
}

die
value::as_reference() const
{
        section_offset off;
        // XXX Would be nice if we could avoid this.  The cursor is
        // all overhead here.
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::ref1:
                off = cur.fixed<ubyte>();
                break;
        case DW_FORM::ref2:
                off = cur.fixed<uhalf>();
                break;
        case DW_FORM::ref4:
                off = cur.fixed<uword>();
                break;
        case DW_FORM::ref8:
                off = cur.fixed<uint64_t>();
                break;
        case DW_FORM::ref_udata:
                off = cur.uleb128();
                break;

        case DW_FORM::ref_addr: {
                off = cur.offset();
                // These seem to be extremely rare in practice (I
                // haven't been able to get gcc to produce a
                // ref_addr), so it's not worth caching this lookup.
                const compilation_unit *base_cu = nullptr;
                for (auto &file_cu : cu->get_dwarf().compilation_units()) {
                        if (file_cu.get_section_offset() > off)
                                break;
                        base_cu = &file_cu;
                }
                die d(base_cu);
                d.read(off - base_cu->get_section_offset());
                return d;
        }

        case DW_FORM::ref_sig8: {
                uint64_t sig = cur.fixed<uint64_t>();
                try {
                        return cu->get_dwarf().get_type_unit(sig).type();
                } catch (std::out_of_range &e) {
                        throw format_error("unknown type signature 0x" + to_hex(sig));
                }
        }

        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as reference");
        }

        die d(cu);
        d.read(off);
        return d;
}

void
value::as_string(string &buf) const
{
        size_t size;
        const char *p = as_cstr(&size);
        buf.resize(size);
        memmove(&buf.front(), p, size);
}

string
value::as_string() const
{
        size_t size;
        const char *s = as_cstr(&size);
        return string(s, size);
}

const char *
value::as_cstr(size_t *size_out) const
{
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::string:
                return cur.cstr(size_out);
        case DW_FORM::strp: {
                section_offset off = cur.offset();
                cursor scur(cu->get_dwarf().get_section(section_type::str), off);
                return scur.cstr(size_out);
        }
        case DW_FORM::line_strp: {
                section_offset off = cur.offset();
                cursor scur(cu->get_dwarf().get_section(section_type::line_str), off);
                return scur.cstr(size_out);
        }
        case DW_FORM::strx:
        case DW_FORM::strx1:
        case DW_FORM::strx2:
        case DW_FORM::strx3:
        case DW_FORM::strx4: {
                // DWARF 5: Read string index, look up in .debug_str_offsets, then read from .debug_str
                uint64_t index;
                switch (form) {
                case DW_FORM::strx:
                        index = cur.uleb128();
                        break;
                case DW_FORM::strx1:
                        index = cur.fixed<uint8_t>();
                        break;
                case DW_FORM::strx2:
                        index = cur.fixed<uint16_t>();
                        break;
                case DW_FORM::strx3:
                        index = cur.fixed<uint8_t>() | (cur.fixed<uint16_t>() << 8);
                        break;
                case DW_FORM::strx4:
                        index = cur.fixed<uint32_t>();
                        break;
                default:
                        index = 0;
                        break;
                }
                // Get str_offsets_base from CU root DIE's DW_AT_str_offsets_base
                // For now, we use a simplified approach: read from start of section + header
                // DWARF 5 .debug_str_offsets has a header (length + version + padding)
                // We skip the 8-byte header (4-byte length + 2-byte version + 2-byte padding for 32-bit DWARF)
                auto str_offsets_sec = cu->get_dwarf().get_section(section_type::str_offsets);
                section_offset header_size = 8;  // Simplified: assume 32-bit DWARF
                unsigned offset_size = (str_offsets_sec->addr_size == 8) ? 8 : 4;
                cursor offsets_cur(str_offsets_sec,
                                   header_size + index * offset_size);
                section_offset str_off = offsets_cur.offset();
                cursor scur(cu->get_dwarf().get_section(section_type::str), str_off);
                return scur.cstr(size_out);
        }
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as string");
        }
}

section_offset
value::as_sec_offset() const
{
        // Prior to DWARF 4, sec_offsets were encoded as data4 or
        // data8.
        cursor cur(cu->data(), offset);
        switch (form) {
        case DW_FORM::data4:
                return cur.fixed<uint32_t>();
        case DW_FORM::data8:
                return cur.fixed<uint64_t>();
        case DW_FORM::sec_offset:
                return cur.offset();
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as sec_offset");
        }
}

void
value::resolve_indirect(DW_AT name)
{
        if (form != DW_FORM::indirect)
                return;

        cursor c(cu->data(), offset);
        DW_FORM form;
        do {
                form = (DW_FORM)c.uleb128();
        } while (form == DW_FORM::indirect);
        attribute_spec spec(name, form);
        typ = spec.type;
        has_implicit_const = (form == DW_FORM::implicit_const);
        implicit_const = spec.implicit_const;
        offset = c.get_section_offset();
}

string
to_string(const value &v)
{
        switch (v.get_type()) {
        case value::type::invalid:
                return "<invalid value type>";
        case value::type::address:
                return "0x" + to_hex(v.as_address());
        case value::type::block: {
                size_t size;
                const char *b = (const char*)v.as_block(&size);
                string res = ::to_string(size) + " byte block:";
                for (size_t pos = 0; pos < size; ++pos) {
                        res += ' ';
                        res += to_hex(b[pos]);
                }
                return res;
        }
        case value::type::constant:
                return "0x" + to_hex(v.as_uconstant());
        case value::type::uconstant:
                return ::to_string(v.as_uconstant());
        case value::type::sconstant:
                return ::to_string(v.as_sconstant());
        case value::type::exprloc:
                // XXX
                return "<exprloc>";
        case value::type::flag:
                return v.as_flag() ? "true" : "false";
        case value::type::line:
                return "<line 0x" + to_hex(v.as_sec_offset()) + ">";
        case value::type::loclist:
                return "<loclist 0x" + to_hex(v.as_sec_offset()) + ">";
        case value::type::mac:
                return "<mac 0x" + to_hex(v.as_sec_offset()) + ">";
        case value::type::rangelist:
                return "<rangelist 0x" + to_hex(v.as_sec_offset()) + ">";
        case value::type::reference: {
                die d = v.as_reference();
                auto tu = dynamic_cast<const type_unit*>(&d.get_unit());
                if (tu)
                        return "<.debug_types+0x" + to_hex(d.get_section_offset()) + ">";
                return "<0x" + to_hex(d.get_section_offset()) + ">";
        }
        case value::type::string:
                return v.as_string();
        }
        return "<unexpected value type " + to_string(v.get_type()) + ">";
}

DWARFPP_END_NAMESPACE
