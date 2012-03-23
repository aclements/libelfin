#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

dwarf::dwarf(std::shared_ptr<impl> m)
        : m(m)
{
}

dwarf::~dwarf()
{
}

const std::vector<compilation_unit> &
dwarf::compilation_units()
{
        return m->compilation_units;
}

shared_ptr<dwarf::impl>
dwarf::impl::create(const std::map<section_type, std::shared_ptr<section> > &sections)
{
        if (!sections.count(section_type::info))
                throw format_error("required .debug_info section missing");
        if (!sections.count(section_type::abbrev))
                throw format_error("required .debug_abbrev section missing");
        shared_ptr<section> sec_str;
        if (sections.count(section_type::str))
                sec_str = sections.at(section_type::str);

        auto m = make_shared<impl>(sections.at(section_type::info),
                                   sections.at(section_type::abbrev),
                                   sec_str);

        // Get compilation units.  Everything derives from these, so
        // there's no point in doing it lazily.
        cursor infocur(m->sec_info);
        info_unit info;
        while (!infocur.end()) {
                info.read(&infocur);
                m->compilation_units.push_back(
                        make_shared<compilation_unit::impl>(m, info));
        }

        return m;
}

compilation_unit::compilation_unit(shared_ptr<impl> m)
        : m(m)
{
}

sec_offset
compilation_unit::get_section_offset() const
{
        return m->info.offset;
}

die
compilation_unit::root() const
{
        m->force_abbrevs();
        die d(m);
        d.read(m->info.entries.section_offset());
        return d;
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
