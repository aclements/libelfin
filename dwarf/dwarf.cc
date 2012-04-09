#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

//////////////////////////////////////////////////////////////////
// class dwarf
//

struct dwarf::impl
{
        impl(const std::shared_ptr<loader> &l)
                : l(l) { }

        std::shared_ptr<loader> l;

        std::shared_ptr<section> sec_info;
        std::shared_ptr<section> sec_abbrev;

        std::vector<compilation_unit> compilation_units;

        std::map<section_type, std::shared_ptr<section> > sections;
};

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
        while (!infocur.end()) {
                // XXX Circular reference
                m->compilation_units.emplace_back(
                        *this, infocur.get_section_offset());
                infocur.subsection();
        }
}

dwarf::~dwarf()
{
}

const std::vector<compilation_unit> &
dwarf::compilation_units() const
{
        static std::vector<compilation_unit> empty;
        if (!m)
                return empty;
        return m->compilation_units;
}

std::shared_ptr<section>
dwarf::get_section(section_type type) const
{
        if (type == section_type::info)
                return m->sec_info;
        if (type == section_type::abbrev)
                return m->sec_abbrev;

        auto it = m->sections.find(type);
        if (it != m->sections.end())
                return it->second;

        size_t size;
        const void *data = m->l->load(type, &size);
        if (!data)
                throw format_error(std::string(elf::section_type_to_name(type))
                                   + " section missing");
        m->sections[type] = std::make_shared<section>(section_type::str, data, size);
        return m->sections[type];
}

//////////////////////////////////////////////////////////////////
// class compilation_unit
//

compilation_unit::compilation_unit(shared_ptr<impl> m)
        : m(m)
{
}

compilation_unit::compilation_unit(const dwarf &file, section_offset offset)
{
        uhalf version;
        // .debug_abbrev-relative offset of this unit's abbrevs
        section_offset debug_abbrev_offset;
        ubyte address_size;

        // Read the CU header (DWARF4 section 7.5.1.1)
        cursor cur(file.get_section(section_type::info), offset);
        std::shared_ptr<section> subsec = cur.subsection();
        cursor sub(subsec);
        sub.skip_initial_length();
        version = sub.fixed<uhalf>();
        if (version < 2 || version > 4)
                throw format_error("unknown info unit version " + std::to_string(version));
        debug_abbrev_offset = sub.offset();
        address_size = sub.fixed<ubyte>();
        subsec->addr_size = address_size;

        m = make_shared<impl>(file, offset, subsec, debug_abbrev_offset,
                              sub.get_section_offset());
}

const dwarf &
compilation_unit::get_dwarf() const
{
        return m->file;
}

section_offset
compilation_unit::get_section_offset() const
{
        return m->offset;
}

const die&
compilation_unit::root() const
{
        if (!m->root.valid()) {
                m->force_abbrevs();
                m->root = die(m);
                m->root.read(m->root_offset);
        }
        return m->root;
}

const std::shared_ptr<section> &
compilation_unit::data() const
{
        return m->subsec;
}

const abbrev_entry &
compilation_unit::get_abbrev(abbrev_code acode)
{
        // XXX Fold this in once there are no more impl references
        return m->get_abbrev(acode);
}

void
compilation_unit::impl::force_abbrevs()
{
        // XXX Compilation units can share abbrevs.  Parse each table
        // at most once.
        if (have_abbrevs)
                return;

        // Section 7.5.3
        cursor c(file.get_section(section_type::abbrev),
                 debug_abbrev_offset);
        abbrev_entry entry;
        abbrev_code highest = 0;
        while (entry.read(&c)) {
                abbrevs_map[entry.code] = entry;
                if (entry.code > highest)
                        highest = entry.code;
        }

        // Typically, abbrev codes are assigned linearly, so it's more
        // space efficient and time efficient to store the table in a
        // vector.  Convert to a vector if it's dense enough, by some
        // rough estimate of "enough".
        if (highest * 10 < abbrevs_map.size() * 15) {
                // Move the map into the vector
                abbrevs_vec.resize(highest + 1);
                for (auto &entry : abbrevs_map)
                        abbrevs_vec[entry.first] = move(entry.second);
                abbrevs_map.clear();
        }

        have_abbrevs = true;
}

const abbrev_entry &
compilation_unit::impl::get_abbrev(abbrev_code acode)
{
        if (!abbrevs_vec.empty()) {
                if (acode >= abbrevs_vec.size())
                        goto unknown;
                const abbrev_entry &entry = abbrevs_vec[acode];
                if (entry.code == 0)
                        goto unknown;
                return entry;
        } else {
                auto it = abbrevs_map.find(acode);
                if (it == abbrevs_map.end())
                        goto unknown;
                return it->second;
        }

unknown:
        throw format_error("unknown abbrev code 0x" + to_hex(acode));
}

DWARFPP_END_NAMESPACE
