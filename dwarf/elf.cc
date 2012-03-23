#include "dwarf++.hh"

#include <cstring>

using namespace std;

DWARFPP_BEGIN_NAMESPACE

bool
elf::section_name_to_type(const char *name, section_type *out)
{
        struct
        {
                const char *name;
                section_type type;
        } sections[] = {
                {".debug_info",     section_type::info},
                {".debug_abbrev",   section_type::abbrev},
                {".debug_pubtypes", section_type::pubtypes},
                {".debug_str",      section_type::str},
        };

        for (auto &sec : sections) {
                if (strcmp(sec.name, name) == 0) {
                        *out = sec.type;
                        return true;
                }
        }
        return false;
}

DWARFPP_END_NAMESPACE
