#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

dwarf::dwarf(const std::shared_ptr<loader> &l)
        : m(make_shared<impl>(l))
{
        const void *data;
        size_t size;

        // Get required sections
        data = l->load(section_type::info, &size);
        if (!data)
                throw format_error("required .debug_info section missing");
        m->sec_info = make_shared<section>(section_type::info, data, size);

        data = l->load(section_type::abbrev, &size);
        if (!data)
                throw format_error("required .debug_abbrev section missing");
        m->sec_abbrev = make_shared<section>(section_type::abbrev, data, size);

        // Get compilation units.  Everything derives from these, so
        // there's no point in doing it lazily.
        cursor infocur(m->sec_info);
        info_unit info;
        while (!infocur.end()) {
                info.read(&infocur);
                // XXX Circular reference
                m->compilation_units.push_back(
                        make_shared<compilation_unit::impl>(m, info));
        }
}

dwarf::~dwarf()
{
}

const std::vector<compilation_unit> &
dwarf::compilation_units() const
{
        return m->compilation_units;
}

compilation_unit::compilation_unit(shared_ptr<impl> m)
        : m(m)
{
}

section_offset
compilation_unit::get_section_offset() const
{
        return m->info.offset;
}

const die&
compilation_unit::root() const
{
        if (!m->root.valid()) {
                m->force_abbrevs();
                m->root = die(m);
                m->root.read(m->info.entries.get_section_offset());
        }
        return m->root;
}

void
compilation_unit::impl::force_abbrevs()
{
        // XXX Compilation units can share abbrevs.  Parse each table
        // at most once.
        if (!abbrevs.empty())
                return;

        // Section 7.5.3
        cursor c(file->sec_abbrev, info.debug_abbrev_offset);
        abbrev_entry entry;
        while (entry.read(&c))
                abbrevs[entry.code] = entry;
}

DWARFPP_END_NAMESPACE
