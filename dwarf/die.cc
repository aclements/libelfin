#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

sec_offset
die::get_section_offset() const
{
        return cu->info.offset + offset;
}

void
die::read(sec_offset off)
{
        cursor cur(cu->subsec, off);

        offset = off;

        abbrev_code acode = cur.uleb128();
        if (acode == 0) {
                abbrev = nullptr;
                next = cur.section_offset();
                return;
        }
        abbrev = &cu->abbrevs.at(acode);

        tag = abbrev->tag;

        // XXX We can pre-compute almost all of this work in the
        // abbrev_entry.
        // XXX Profile shows we spend most of our time allocating this
        // vector's space for new DIEs (iterator() or as_reference()).
        // We could reserve some number of slots in the DIE itself
        // since most DIEs are small and even use those only for
        // variable offsets.
        attrs.clear();
        for (auto &attr : abbrev->attributes) {
                attrs.push_back(cur.section_offset());
                cur.skip_form(attr.form);
        }
        next = cur.section_offset();
}

bool
die::has(DW_AT attr) const
{
        if (!abbrev)
                return false;
        // XXX Totally lame
        for (auto &a : abbrev->attributes)
                if (a.name == attr)
                        return true;
        return false;
}

const value
die::operator[](DW_AT attr) const
{
        // XXX We can pre-compute almost all of this work in the
        // abbrev_entry.
        if (abbrev) {
                int i = 0;
                for (auto &a : abbrev->attributes) {
                        if (a.name == attr)
                                return value(cu, a.name, a.form, a.type, attrs[i]);
                        i++;
                }
        }
        throw out_of_range("DIE does not have attribute " + to_string(attr));
}

die::iterator
die::begin() const
{
        if (!abbrev || !abbrev->children)
                return end();
        return iterator(cu, next);
}

die::iterator::iterator(std::shared_ptr<compilation_unit::impl> cu,
                        sec_offset off)
        : d(cu)
{
        d.read(off);
}

die::iterator &
die::iterator::operator++()
{
        if (!d.abbrev)
                return *this;

        if (!d.abbrev->children) {
                // The DIE has no children, so its successor follows
                // immediately
                d.read(d.next);
        } else if (d.has(DW_AT::sibling)) {
                // They made it easy on us.  Follow the sibling
                // pointer.  XXX Probably worth optimizing
                d = d[DW_AT::sibling].as_reference();
        } else {
                // It's a hard-knock life.  We have to iterate through
                // the children to find the next DIE.
                // XXX Particularly unfortunate if the user is doing a
                // DFS, since this will result in N^2 behavior.  Maybe
                // a small cache of terminator locations in the CU?
                iterator sub(d.cu, d.next);
                while (sub->abbrev)
                        ++sub;
                d.read(sub->next);
        }

        return *this;
}

const vector<pair<DW_AT, value> >
die::attributes() const
{
        vector<pair<DW_AT, value> > res;

        if (!abbrev)
                return res;

        int i = 0;
        for (auto &a : abbrev->attributes) {
                res.push_back(make_pair(a.name, value(cu, a.name, a.form, a.type, attrs[i])));
                i++;
        }
        return res;
}

bool
die::operator==(const die &o) const
{
        return cu == o.cu && offset == o.offset;
}

bool
die::operator!=(const die &o) const
{
        return !(*this == o);
}

DWARFPP_END_NAMESPACE

size_t
std::hash<dwarf::die>::operator()(const dwarf::die &a) const
{
        return hash<decltype(a.cu)>()(a.cu) ^
                hash<decltype(a.get_unit_offset())>()(a.get_unit_offset());
}
