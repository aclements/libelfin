#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

rangelist::rangelist(const initializer_list<pair<taddr, taddr> > &ranges)
{
        synthetic.reserve(ranges.size() * 2 + 2);
        for (auto &range : ranges) {
                synthetic.push_back(range.first);
                synthetic.push_back(range.second);
        }
        synthetic.push_back(0);
        synthetic.push_back(0);

        sec = make_shared<section>(
                section_type::ranges, (const char*)synthetic.data(),
                synthetic.size() * sizeof(taddr), format::unknown,
                sizeof(taddr));

        base_addr = 0;
}

rangelist::iterator
rangelist::begin()
{
        return iterator(sec, base_addr);
}

rangelist::iterator
rangelist::end()
{
        return iterator();
}

bool
rangelist::contains(taddr addr)
{
        for (auto ent : *this)
                if (ent.contains(addr))
                        return true;
        return false;
}

rangelist::entry
rangelist::iterator::operator*() const
{
        cursor cur(sec, pos);
        taddr low = cur.address() + base_addr;
        taddr high = cur.address() + base_addr;
        return {low, high};
}

rangelist::iterator &
rangelist::iterator::operator++()
{
        pos += 2 * sec->addr_size;
        sync();
        return *this;
}

void
rangelist::iterator::sync()
{
        // DWARF4 section 2.17.3
        taddr largest_offset = ~(taddr)0;
        if (sec->addr_size < sizeof(taddr))
                largest_offset += 1 << (8 * sec->addr_size);

        // Process base address selections and end-of-lists
        cursor cur(sec, pos);
        while (true) {
                pos = cur.section_offset();

                taddr low = cur.address();
                taddr high = cur.address();

                if (low == 0 && high == 0) {
                        // End of list
                        sec.reset();
                        pos = 0;
                        return;
                } else if (low == largest_offset) {
                        base_addr = high;
                } else {
                        return;
                }
        }
}

DWARFPP_END_NAMESPACE
