#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

sec_offset
value::get_section_offset() const
{
        return cu->info.offset + offset;
}

uint64_t
value::as_address() const
{
        if (form != DW_FORM::addr)
                throw value_type_mismatch("cannot read " + to_string(typ) + " as address");

        cursor cur(cu->subsec, offset);
        switch (cu->subsec->addr_size) {
        case 1:
                return cur.fixed<uint8_t>();
        case 2:
                return cur.fixed<uint16_t>();
        case 4:
                return cur.fixed<uint32_t>();
        case 8:
                return cur.fixed<uint64_t>();
        default:
                throw runtime_error("address size " + ::to_string(cu->subsec->addr_size) + " not supported");
        }
}

const void *
value::as_block(size_t *size_out) const
{
        // XXX It appears that expressions are often encoded as
        // blocks, not as exprlocs.  Need DW_AT-aware types?
        // XXX Blocks can contain all sorts of things, including
        // references, which couldn't be resolved by callers in the
        // current minimal API.
        cursor cur(cu->subsec, offset);
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
        cursor cur(cu->subsec, offset);
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
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as uconstant");
        }
}

int64_t
value::as_sconstant() const
{
        cursor cur(cu->subsec, offset);
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
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as sconstant");
        }
}

bool
value::as_flag() const
{
        switch (form) {
        case DW_FORM::flag: {
                cursor cur(cu->subsec, offset);
                return cur.fixed<ubyte>() != 0;
        }
        case DW_FORM::flag_present:
                return true;
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as flag");
        }
}

die
value::as_reference() const
{
        sec_offset off;
        // XXX Would be nice if we could avoid this.  The cursor is
        // all overhead here.
        cursor cur(cu->subsec, offset);
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

                // XXX Implement
        case DW_FORM::ref_addr:
                throw runtime_error("DW_FORM::ref_addr not implemented");
        case DW_FORM::ref_sig8:
                throw runtime_error("DW_FORM::ref_sig8 not implemented");

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
        cursor cur(cu->subsec, offset);
        switch (form) {
        case DW_FORM::string:
                cur.string(buf);
                return;
        case DW_FORM::strp: {
                sec_offset off = cur.offset();
                cursor scur(cu->file->get_sec_str(), off);
                scur.string(buf);
                return;
        }
        default:
                throw value_type_mismatch("cannot read " + to_string(typ) + " as string");
        }
}

string
value::as_string() const
{
        string buf;
        as_string(buf);
        return buf;
}

void
value::resolve_indirect(DW_AT name)
{
        if (form != DW_FORM::indirect)
                return;

        cursor c(cu->subsec, offset);
        DW_FORM form;
        do {
                form = (DW_FORM)c.uleb128();
        } while (form == DW_FORM::indirect);
        typ = attribute_spec(name, form).type;
        offset = c.section_offset();
}

DWARFPP_END_NAMESPACE
