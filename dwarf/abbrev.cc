#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

static value::type
resolve_type(DW_AT name, DW_FORM form)
{
        switch (form) {
        case DW_FORM::addr:
                return value::type::address;
        case DW_FORM::block:
        case DW_FORM::block1:
        case DW_FORM::block2:
        case DW_FORM::block4:
                return value::type::block;
        case DW_FORM::data1:
        case DW_FORM::data2:
        case DW_FORM::data4:
        case DW_FORM::data8:
        case DW_FORM::udata:
                return value::type::uconstant;
        case DW_FORM::sdata:
                return value::type::sconstant;
        case DW_FORM::exprloc:
                return value::type::exprloc;
        case DW_FORM::flag:
        case DW_FORM::flag_present:
                return value::type::flag;
        case DW_FORM::ref1:
        case DW_FORM::ref2:
        case DW_FORM::ref4:
        case DW_FORM::ref8:
        case DW_FORM::ref_addr:
        case DW_FORM::ref_sig8:
        case DW_FORM::ref_udata:
                return value::type::reference;
        case DW_FORM::string:
        case DW_FORM::strp:
                return value::type::string;

        case DW_FORM::indirect:
                // There's nothing meaningful we can do
                return value::type::invalid;

        case DW_FORM::sec_offset:
                // The type of this form depends on the attribute
                switch (name) {
                case DW_AT::stmt_list:
                        return value::type::lineptr;

                case DW_AT::location:
                case DW_AT::string_length:
                case DW_AT::return_addr:
                case DW_AT::data_member_location:
                case DW_AT::frame_base:
                case DW_AT::segment:
                case DW_AT::static_link:
                case DW_AT::use_location:
                case DW_AT::vtable_elem_location:
                        return value::type::loclistptr;

                case DW_AT::macro_info:
                        return value::type::macptr;

                case DW_AT::start_scope:
                case DW_AT::ranges:
                        return value::type::rangelistptr;

                default:
                        throw format_error("DW_FORM_sec_offset not expected for attribute " +
                                           to_string(name));
                }
        }
        throw format_error("unknown attribute form " + to_string(form));
}

attribute_spec::attribute_spec(DW_AT name, DW_FORM form)
        : name(name), form(form), type(resolve_type(name, form))
{
}

bool
abbrev_entry::read(cursor *cur)
{
        attributes.clear();

        // Section 7.5.3
        code = cur->uleb128();
        if (!code)
                return false;

        tag = (DW_TAG)cur->uleb128();
        children = cur->fixed<DW_CHILDREN>() == DW_CHILDREN::yes;
        while (1) {
                DW_AT name = (DW_AT)cur->uleb128();
                DW_FORM form = (DW_FORM)cur->uleb128();
                if (name == (DW_AT)0 && form == (DW_FORM)0)
                        break;
                attributes.push_back(attribute_spec(name, form));
        }
        attributes.shrink_to_fit();
        return true;
}

DWARFPP_END_NAMESPACE
