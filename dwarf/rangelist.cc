// Copyright (c) 2013 Austin T. Clements. All rights reserved.
// Use of this source code is governed by an MIT license
// that can be found in the LICENSE file.

#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

rangelist::rangelist(const std::shared_ptr<section> &sec, section_offset off,
                     unsigned cu_addr_size, taddr cu_low_pc, bool is_dwarf5)
        : sec(sec->slice(off, ~0, format::unknown, cu_addr_size)),
          base_addr(cu_low_pc),
          is_dwarf5(is_dwarf5)
{
}

rangelist::rangelist(const initializer_list<pair<taddr, taddr> > &ranges)
        : is_dwarf5(false)
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
                synthetic.size() * sizeof(taddr),
                native_order(), format::unknown, sizeof(taddr));

        base_addr = 0;
}

rangelist::iterator
rangelist::begin() const
{
        if (sec)
                return iterator(sec, base_addr, is_dwarf5);
        return end();
}

rangelist::iterator
rangelist::end() const
{
        return iterator();
}

bool
rangelist::contains(taddr addr) const
{
        for (auto ent : *this)
                if (ent.contains(addr))
                        return true;
        return false;
}

rangelist::iterator::iterator(const std::shared_ptr<section> &sec, taddr base_addr, bool is_dwarf5)
        : sec(sec), base_addr(base_addr), pos(0), is_dwarf5(is_dwarf5)
{
        // Read in the first entry
        ++(*this);
}

rangelist::iterator &
rangelist::iterator::operator++()
{
        cursor cur(sec, pos);

        if (is_dwarf5) {
                // DWARF 5 range list entries (Section 2.17.3)
                while (true) {
                        if (cur.end()) {
                                sec.reset();
                                pos = 0;
                                return *this;
                        }

                        DW_RLE rle = (DW_RLE)cur.fixed<uint8_t>();

                        switch (rle) {
                        case DW_RLE::end_of_list:
                                sec.reset();
                                pos = 0;
                                return *this;

                        case DW_RLE::base_addressx:
                                // Index into .debug_addr - for now, skip this
                                cur.uleb128();
                                break;

                        case DW_RLE::startx_endx:
                                // Both start and end are indices into .debug_addr
                                cur.uleb128();
                                cur.uleb128();
                                // Skip for now - would need .debug_addr lookup
                                break;

                        case DW_RLE::startx_length:
                                // Start is index, length is ULEB128
                                cur.uleb128();
                                cur.uleb128();
                                // Skip for now - would need .debug_addr lookup
                                break;

                        case DW_RLE::offset_pair:
                                // Two ULEB128 offsets from base address
                                entry.low = base_addr + cur.uleb128();
                                entry.high = base_addr + cur.uleb128();
                                pos = cur.get_section_offset();
                                return *this;

                        case DW_RLE::base_address:
                                // New base address (full address)
                                base_addr = cur.address();
                                break;

                        case DW_RLE::start_end:
                                // Two full addresses
                                entry.low = cur.address();
                                entry.high = cur.address();
                                pos = cur.get_section_offset();
                                return *this;

                        case DW_RLE::start_length:
                                // Full address + ULEB128 length
                                entry.low = cur.address();
                                entry.high = entry.low + cur.uleb128();
                                pos = cur.get_section_offset();
                                return *this;

                        default:
                                throw format_error("unknown DW_RLE encoding " + to_string(rle));
                        }
                }
        } else {
                // DWARF 4 section 2.17.3
                taddr largest_offset = ~(taddr)0;
                if (sec->addr_size < sizeof(taddr))
                        largest_offset += 1 << (8 * sec->addr_size);

                // Read in entries until we reach a regular entry or an
                // end-of-list.  Note that pos points to the beginning of the
                // entry *following* the current entry, so that's where we
                // start.
                while (true) {
                        entry.low = cur.address();
                        entry.high = cur.address();

                        if (entry.low == 0 && entry.high == 0) {
                                // End of list
                                sec.reset();
                                pos = 0;
                                break;
                        } else if (entry.low == largest_offset) {
                                // Base address change
                                base_addr = entry.high;
                        } else {
                                // Regular entry.  Adjust by base address.
                                entry.low += base_addr;
                                entry.high += base_addr;
                                pos = cur.get_section_offset();
                                break;
                        }
                }
        }

        return *this;
}

DWARFPP_END_NAMESPACE
