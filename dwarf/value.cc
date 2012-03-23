#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

sec_offset
value::get_section_offset() const
{
        return cu->info.offset + offset;
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
                throw value_type_mismatch("cannot read " + to_string(typ) + " as a reference");
        }

        die d(cu);
        d.read(off);
        return d;
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
