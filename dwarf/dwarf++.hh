#ifndef _DWARFPP_HH_
#define _DWARFPP_HH_

#ifndef DWARFPP_BEGIN_NAMESPACE
#define DWARFPP_BEGIN_NAMESPACE namespace dwarf {
#define DWARFPP_END_NAMESPACE   }
#endif

#include "data.hh"
#include "small_vector.hh"

#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

DWARFPP_BEGIN_NAMESPACE

// Forward declarations
class dwarf;
class loader;
class compilation_unit;
class die;
class value;
class expr;
class expr_context;
class expr_result;
class rangelist;

// Internal type forward-declarations
struct section;
struct abbrev_entry;
struct cursor;

//////////////////////////////////////////////////////////////////
// DWARF files
//

/**
 * An exception indicating malformed DWARF data.
 */
class format_error : public std::runtime_error
{
public:
        explicit format_error(const std::string &what_arg)
                : std::runtime_error(what_arg) { }
        explicit format_error(const char *what_arg)
                : std::runtime_error(what_arg) { }
};

/**
 * DWARF section types.  These correspond to the names of ELF
 * sections, though DWARF can be embedded in other formats.
 */
enum class section_type
{
        abbrev,
        aranges,
        frame,
        info,
        line,
        loc,
        macinfo,
        pubnames,
        pubtypes,
        ranges,
        str,
        types,
};

std::string
to_string(section_type v);

/**
 * A DWARF file.  This class is internally reference counted and can
 * be efficiently copied.
 */
class dwarf
{
public:
        /**
         * Construct a DWARF file that is backed by sections read from
         * the given loader.
         */
        explicit dwarf(const std::shared_ptr<loader> &l);

        /**
         * Construct a DWARF file that is initially not valid.
         */
        dwarf() = default;
        dwarf(const dwarf&) = default;
        dwarf(dwarf&&) = default;
        ~dwarf();

        dwarf& operator=(const dwarf &o) = default;

        bool operator==(const dwarf &o) const
        {
                return m == o.m;
        }

        bool operator!=(const dwarf &o) const
        {
                return m != o.m;
        }

        /**
         * Return true if this object represents a DWARF file.
         * Default constructed dwarf objects are not valid.
         */
        bool valid() const
        {
                return !!m;
        }

        // XXX This allows the compilation units to be modified and
        // ties us to a vector.  Probably should return an opaque
        // iterable collection over const references.
        /**
         * Return the list of compilation units in this DWARF file.
         */
        const std::vector<compilation_unit> &compilation_units() const;

        /**
         * \internal Retrieve the specified section from this file.
         * If the section does not exist, throws format_error.
         */
        std::shared_ptr<section> get_section(section_type type) const;

private:
        struct impl;
        std::shared_ptr<impl> m;
};

/**
 * An interface for lazily loading DWARF sections.
 */
class loader
{
public:
        virtual ~loader() { }

        /**
         * Load the requested DWARF section into memory and return a
         * pointer to the beginning of it.  This memory must remain
         * valid and unchanged until the loader is destroyed.  If the
         * requested section does not exist, this should return
         * nullptr.  If the section exists but cannot be loaded for
         * any reason, this should throw an exception.
         */
        virtual const void *load(section_type section, size_t *size_out) = 0;
};

/**
 * A compilation unit within a DWARF file.  Most of the information
 * in a DWARF file is divided up by compilation unit.  This class is
 * internally reference counted and can be efficiently copied.
 */
class compilation_unit
{
public:
        /**
         * Return the byte offset of this compilation unit in the
         * .debug_info section.
         */
        section_offset get_section_offset() const;

        /**
         * Return the root DIE of this compilation unit.  This should
         * be a DW_TAG::compilation_unit or DW_TAG::partial_unit.
         */
        const die &root() const;

        struct impl;
        compilation_unit(std::shared_ptr<impl> m);

        /**
         * \internal Construct a compilation unit whose header begins
         * offset bytes into the .debug_info section of file.
         */
        compilation_unit(const dwarf &file, section_offset offset);

private:
        std::shared_ptr<impl> m;
};

//////////////////////////////////////////////////////////////////
// Debugging information entries (DIEs)
//

/**
 * A Debugging Information Entry, or DIE.  The basic unit of
 * information in a DWARF file.
 */
class die
{
        // XXX Make this class better for use in maps.  Currently dies
        // are fairly big and expensive to copy, but most of that
        // information can be constructed lazily.  This is also bad
        // for use in caches since it will keep the DWARF file alive.
        // OTOH, maybe caches need eviction anyway.
public:
        DW_TAG tag;

        die() : abbrev(nullptr) { }
        die(const die &o) = default;
        die(die &&o) = default;

        die& operator=(const die &o) = default;

        /**
         * Return true if this object represents a DIE in a DWARF
         * file.  Default constructed objects are not valid and some
         * methods return invalid DIEs to indicate failures.
         */
        bool valid() const
        {
                return abbrev != nullptr;
        }

        /**
         * Return this DIE's byte offset within its compilation unit.
         */
        section_offset get_unit_offset() const
        {
                return offset;
        }

        /**
         * Return this DIE's byte offset within its section.
         */
        section_offset get_section_offset() const;

        /**
         * Return true if this DIE has the requested attribute.
         */
        bool has(DW_AT attr) const;

        /**
         * Return the value of attr.  Throws out_of_range if this DIE
         * does not have the specified attribute.  It is generally
         * better to use the type-safe attribute getters (the global
         * functions beginning with at_*) when possible.
         */
        const value operator[](DW_AT attr) const;

        class iterator;

        /**
         * Return an iterator over the children of this DIE.  Note
         * that the DIEs returned by this iterator are temporary, so
         * if you need to store a DIE for more than one loop
         * iteration, you must copy it.
         */
        iterator begin() const;
        iterator end() const;

        /**
         * Return a vector of the attributes of this DIE.
         */
        const std::vector<std::pair<DW_AT, value> > attributes() const;

        bool operator==(const die &o) const;
        bool operator!=(const die &o) const;

private:
        friend class compilation_unit;
        friend class value;
        // XXX If we can get the CU, we don't need this
        friend struct ::std::hash<die>;

        std::shared_ptr<compilation_unit::impl> cu;
        // The abbrev of this DIE.  By convention, if this DIE
        // represents a sibling list terminator, this is null.  This
        // object is kept live by the CU.
        const abbrev_entry *abbrev;
        // The beginning of this DIE, relative to the CU.
        section_offset offset;
        // Offsets of attributes, relative to cu's subsection.  The
        // vast majority of DIEs tend to have six or fewer attributes,
        // so we reserve space in the DIE itself for six attributes.
        small_vector<section_offset, 6> attrs;
        // The offset of the next DIE, relative to cu'd subsection.
        // This is set even for sibling list terminators.
        section_offset next;

        die(std::shared_ptr<compilation_unit::impl> cu);

        /**
         * Read this DIE from the given offset in cu.
         */
        void read(section_offset off);
};

/**
 * An iterator over a sequence of sibling DIEs.
 */
class die::iterator
{
public:
        iterator() = default;
        iterator(const iterator &o) = default;
        iterator(iterator &&o) = default;

        const die &operator*() const
        {
                return d;
        }

        const die *operator->() const
        {
                return &d;
        }

        bool operator!=(const iterator &o) const
        {
                // Quick test of abbrevs.  In particular, this weeds
                // out non-end against end, which is a common
                // comparison while iterating, though it also weeds
                // out many other things.
                if (d.abbrev != o.d.abbrev)
                        return true;

                // Same, possibly NULL abbrev.  If abbrev is NULL,
                // then next's are uncomparable, so we need to stop
                // now.  We consider all ends to be the same, without
                // comparing cu's.
                if (d.abbrev == nullptr)
                        return false;

                // Comparing two non-end abbrevs.
                return d.next != o.d.next || d.cu != o.d.cu;
        }

        iterator &operator++();

private:
        friend class die;

        iterator(std::shared_ptr<compilation_unit::impl> cu, section_offset off);

        die d;
};

inline die::iterator
die::end() const
{
        return iterator();
}

/**
 * An exception indicating that a value is not of the requested type.
 */
class value_type_mismatch : public std::logic_error
{
public:
        explicit value_type_mismatch(const std::string &what_arg)
                : std::logic_error(what_arg) { }
        explicit value_type_mismatch(const char *what_arg)
                : std::logic_error(what_arg) { }
};

/**
 * The value of a DIE attribute.
 *
 * This is logically a union of many different types.  Each type has a
 * corresponding as_* methods that will return the value as that type
 * or throw value_type_mismatch if the attribute is not of the
 * requested type.
 *
 * Values of "constant" type are somewhat ambiguous and
 * context-dependent.  Constant forms with specified signed-ness have
 * type "uconstant" or "sconstant", while other constant forms have
 * type "constant".  If the value's type is "constant", it can be
 * retrieved using either as_uconstant or as_sconstant.
 */
class value
{
public:
        // XXX Remove the "ptr" suffixes
        enum class type
        {
                invalid,
                address,
                block,
                constant,
                uconstant,
                sconstant,
                exprloc,
                flag,
                lineptr,
                loclistptr,
                macptr,
                rangelistptr,
                reference,
                string
        };

        value(const value &o) = default;
        value(value &&o) = default;
        value &operator=(const value &o) = default;

        /**
         * Return this value's byte offset within its compilation
         * unit.
         */
        section_offset get_unit_offset() const
        {
                return offset;
        }

        /**
         * Return this value's byte offset within its section.
         */
        section_offset get_section_offset() const;

        type get_type() const
        {
                return typ;
        }

        /**
         * Return this value's attribute encoding.  This automatically
         * resolves indirect encodings, so this will never return
         * DW_FORM::indirect.  Note that the mapping from forms to
         * types is non-trivial and often depends on the attribute
         * (especially prior to DWARF 4).
         */
        DW_FORM get_form() const
        {
                return form;
        }

        taddr as_address() const;

        /**
         * Return this value as a block.  The returned pointer points
         * directly into the section data, so the caller must ensure
         * that remains valid as long as the data is in use.
         * *size_out is set to the length of the returned block, in
         * bytes.
         */
        const void *as_block(size_t *size_out) const;

        uint64_t as_uconstant() const;

        int64_t as_sconstant() const;

        expr as_exprloc() const;

        bool as_flag() const;

        // XXX lineptr, loclistptr, macptr

        rangelist as_rangelist() const;

        die as_reference() const;

        /**
         * Return this value as a string.
         */
        std::string as_string() const;

        /**
         * Fill the given string buffer with the string value of this
         * value.  This is useful to minimize allocation when reading
         * several string-type values.
         */
        void as_string(std::string &buf) const;

        /**
         * Return this value as a NUL-terminated character string.
         * The returned pointer points directly into the section data,
         * so the caller must ensure that remains valid as long as the
         * data is in use.  *size_out, if not NULL, is set to the
         * length of the returned string without the NUL-terminator.
         */
        const char *as_cstr(size_t *size_out = nullptr) const;

        /**
         * Return this value as a section offset.  This is applicable
         * to lineptr, loclistptr, macptr, and rangelistptr.
         */
        section_offset as_sec_offset() const;

private:
        friend class die;

        value(const std::shared_ptr<compilation_unit::impl> cu,
              DW_AT name, DW_FORM form, type typ, section_offset offset);

        void resolve_indirect(DW_AT name);

        std::shared_ptr<compilation_unit::impl> cu;
        DW_FORM form;
        type typ;
        section_offset offset;
};

std::string
to_string(value::type v);

std::string
to_string(const value &v);

//////////////////////////////////////////////////////////////////
// Expressions and location descriptions
//

/**
 * An exception during expression evaluation.
 */
class expr_error : public std::runtime_error
{
public:
        explicit expr_error(const std::string &what_arg)
                : std::runtime_error(what_arg) { }
        explicit expr_error(const char *what_arg)
                : std::runtime_error(what_arg) { }
};

/**
 * A DWARF expression or location description.
 */
class expr
{
public:
        /**
         * Short-hand for evaluate(ctx, {}).
         */
        expr_result evaluate(expr_context *ctx) const;

        /**
         * Short-hand for evaluate(ctx, {argument}).
         */
        expr_result evaluate(expr_context *ctx, taddr argument) const;

        /**
         * Return the result of evaluating this expression using the
         * specified expression context.  The expression stack will be
         * initialized with the given arguments such that the first
         * arguments is at the top of the stack and the last argument
         * at the bottom of the stack.
         *
         * Throws expr_error if there is an error evaluating the
         * expression (such as an unknown operation, stack underflow,
         * bounds error, etc.)
         */
        expr_result evaluate(expr_context *ctx, const std::initializer_list<taddr> &arguments) const;

private:
        // XXX This will need more information for some operations
        expr(const std::shared_ptr<compilation_unit::impl> cu,
             section_offset offset, section_length len);

        friend class value;

        std::shared_ptr<compilation_unit::impl> cu;
        section_offset offset;
        section_length len;
};

/**
 * An interface that provides contextual information for expression
 * evaluation.  Callers of expr::evaluate are expected to subclass
 * this in order to provide this information to the expression
 * evaluation engine.  The default implementation throws expr_error
 * for all methods.
 */
class expr_context
{
public:
        virtual ~expr_context() { }

        /**
         * Return the value stored in register regnum.  This is used
         * to implement DW_OP_breg* operations.
         */
        virtual taddr reg(unsigned regnum)
        {
                throw expr_error("DW_OP_breg* operations not supported");
        }

        /**
         * Implement DW_OP_deref_size.
         */
        virtual taddr deref_size(taddr address, unsigned size)
        {
                throw expr_error("DW_OP_deref_size operations not supported");
        }

        /**
         * Implement DW_OP_xderef_size.
         */
        virtual taddr xderef_size(taddr address, taddr asid, unsigned size)
        {
                throw expr_error("DW_OP_xderef_size operations not supported");
        }

        /**
         * Implement DW_OP_form_tls_address.
         */
        virtual taddr form_tls_address(taddr address)
        {
                throw expr_error("DW_OP_form_tls_address operations not supported");
        }
};

/**
 * An instance of expr_context that throws expr_error for all methods.
 * This is equivalent to the default construction of expr_context, but
 * often more convenient to use.
 */
extern expr_context no_expr_context;

/**
 * The result of evaluating a DWARF expression or location
 * description.
 */
class expr_result
{
public:
        enum class type {
                /**
                 * value specifies the address in memory of an object.
                 * This is also the result type used for general
                 * expressions that do not refer to object locations.
                 */
                address,
                /**
                 * value specifies a register storing an object.
                 */
                reg,
                /**
                 * The object does not have a location.  value is the
                 * value of the object.
                 */
                literal,
                /**
                 * The object does not have a location.  Its value is
                 * pointed to by the 'implicit' field.
                 */
                implicit,
                /**
                 * The object is present in the source, but not in the
                 * object code, and hence does not have a location or
                 * a value.
                 */
                empty,
        };

        /**
         * For location descriptions, the type of location this result
         * describes.
         */
        type location_type;

        /**
         * For general-purpose expressions, the result of expression.
         * For address location descriptions, the address in memory of
         * the object.  For register location descriptions, the
         * register storing the object.  For literal location
         * descriptions, the value of the object.
         */
        taddr value;

        /**
         * For implicit location descriptions, a pointer to a block
         * representing the value in the memory representation of the
         * target machine.
         */
        const char *implicit;
        size_t implicit_len;

        // XXX Composite locations
};

std::string
to_string(expr_result::type v);

//////////////////////////////////////////////////////////////////
// Range lists
//

/**
 * A DWARF range list describing a set of possibly non-contiguous
 * addresses.
 */
class rangelist
{
public:
        rangelist(const std::shared_ptr<section> &sec,
                  taddr base_addr);

        /**
         * Construct a range list from a sequence of {low, high}
         * pairs.
         */
        rangelist(const std::initializer_list<std::pair<taddr, taddr> > &ranges);

        rangelist() = default;
        rangelist(const rangelist &o) = default;
        rangelist(rangelist &&o) = default;

        class entry;
        typedef entry value_type;

        class iterator;

        /**
         * Return an iterator over the entries in this range list.
         * The ranges returned by this iterator are temporary, so if
         * you need to store a range for more than one loop iteration,
         * you must copy it.
         */
        iterator begin();
        iterator end();

        /**
         * Return true if this range list contains the given address.
         */
        bool contains(taddr addr);

private:
        std::vector<taddr> synthetic;
        std::shared_ptr<section> sec;
        taddr base_addr;
};

/**
 * An entry in a range list.  The range spans addresses [low, high).
 */
class rangelist::entry
{
public:
        taddr low, high;

        /**
         * Return true if addr is within this entry's bounds.
         */
        bool contains(taddr addr)
        {
                return low <= addr && addr < high;
        }
};

/**
 * An iterator over a sequence of ranges in a range list.
 */
class rangelist::iterator
{
public:
        iterator() : sec(nullptr), base_addr(0), pos(0) { }
        iterator(const iterator &o) = default;
        iterator(iterator &&o) = default;

        const rangelist::entry &operator*() const
        {
                return entry;
        }

        const rangelist::entry *operator->() const
        {
                return &entry;
        }

        bool operator!=(const iterator &o) const
        {
                return sec != o.sec || pos != o.pos;
        }

        iterator &operator++();

private:
        iterator(const std::shared_ptr<section> &sec, taddr base_addr);

        friend class rangelist;

        std::shared_ptr<section> sec;
        taddr base_addr;
        section_offset pos;
        rangelist::entry entry;
};

//////////////////////////////////////////////////////////////////
// Type-safe attribute getters
//

// XXX More

die at_abstract_origin(const die &d);
DW_ACCESS at_accessibility(const die &d);
uint64_t at_allocated(const die &d, expr_context *ctx);
bool at_artificial(const die &d);
uint64_t at_associated(const die &d, expr_context *ctx);
uint64_t at_bit_offset(const die &d, expr_context *ctx);
uint64_t at_bit_size(const die &d, expr_context *ctx);
uint64_t at_bit_stride(const die &d, expr_context *ctx);
uint64_t at_byte_size(const die &d, expr_context *ctx);
uint64_t at_byte_stride(const die &d, expr_context *ctx);
DW_CC at_calling_convention(const die &d);
die at_common_reference(const die &d);
std::string at_comp_dir(const die &d);
value at_const_value(const die &d);
bool at_const_expr(const die &d);
die at_containing_type(const die &d);
uint64_t at_count(const die &d, expr_context *ctx);
expr_result at_data_member_location(const die &d, expr_context *ctx, taddr base, taddr pc);
bool at_declaration(const die &d);
std::string at_description(const die &d);
die at_discr(const die &d);
value at_discr_value(const die &d);
bool at_elemental(const die &d);
DW_ATE at_encoding(const die &d);
DW_END at_endianity(const die &d);
taddr at_entry_pc(const die &d);
bool at_enum_class(const die &d);
bool at_explicit(const die &d);
die at_extension(const die &d);
bool at_external(const die &d);
die at_friend(const die &d);
taddr at_high_pc(const die &d);
DW_ID at_identifier_case(const die &d);
die at_import(const die &d);
DW_INL at_inline(const die &d);
bool at_is_optional(const die &d);
DW_LANG at_language(const die &d);
std::string at_linkage_name(const die &d);
taddr at_low_pc(const die &d);
uint64_t at_lower_bound(const die &d, expr_context *ctx);
bool at_main_subprogram(const die &d);
bool at_mutable(const die &d);
std::string at_name(const die &d);
die at_namelist_item(const die &d);
die at_object_pointer(const die &d);
DW_ORD at_ordering(const die &d);
std::string at_picture_string(const die &d);
die at_priority(const die &d);
std::string at_producer(const die &d);
bool at_prototyped(const die &d);
bool at_pure(const die &d);
rangelist at_ranges(const die &d);
bool at_recursive(const die &d);
die at_sibling(const die &d);
die at_signature(const die &d);
die at_small(const die &d);
die at_specification(const die &d);
bool at_threads_scaled(const die &d);
die at_type(const die &d);
uint64_t at_upper_bound(const die &d, expr_context *ctx);
bool at_use_UTF8(const die &d);
bool at_variable_parameter(const die &d);
DW_VIRTUALITY at_virtuality(const die &d);
DW_VIS at_visibility(const die &d);

/**
 * Return the PC range spanned by the code of a DIE.  The DIE must
 * either have DW_AT::ranges or DW_AT::low_pc.  It may optionally have
 * DW_AT::high_pc.
 */
rangelist die_pc_range(const die &d);

//////////////////////////////////////////////////////////////////
// Utilities
//

/**
 * An index of sibling DIEs by some string attribute.  This index is
 * lazily constructed and space-efficient.
 */
class die_str_map
{
public:
        /**
         * Construct the index of the attr attribute of all immediate
         * children of parent whose tags are in accept.
         */
        die_str_map(const die &parent, DW_AT attr,
                    const std::initializer_list<DW_TAG> &accept);

        die_str_map() = default;
        die_str_map(const die_str_map &o) = default;
        die_str_map(die_str_map &&o) = default;

        die_str_map& operator=(const die_str_map &o) = default;

        /**
         * Construct a string map for the type names of parent's
         * immediate children.
         *
         * XXX This should use .debug_pubtypes if parent is a compile
         * unit's root DIE, but it currently does not.
         */
        static die_str_map from_type_names(const die &parent);

        /**
         * Return the DIE whose attribute matches val.  If no such DIE
         * exists, return an invalid die object.
         */
        const die &operator[](const char *val) const;

        /**
         * Short-hand for [value.c_str()].
         */
        const die &operator[](const std::string &val) const
        {
                return (*this)[val.c_str()];
        }

private:
        struct impl;
        std::shared_ptr<impl> m;
};

//////////////////////////////////////////////////////////////////
// ELF support
//

namespace elf
{
        /**
         * Translate an ELF section name info a DWARF section type.
         * If the section is a valid DWARF section name, sets *out to
         * the type and returns true.  If not, returns false.
         */
        bool section_name_to_type(const char *name, section_type *out);

        /**
         * Translate a DWARF section type into an ELF section name.
         */
        const char *section_type_to_name(section_type type);

        template<typename Elf>
        class elf_loader : public loader
        {
                Elf f;

        public:
                elf_loader(const Elf &file) : f(file) { }

                const void *load(section_type section, size_t *size_out)
                {
                        auto sec = f.get_section(section_type_to_name(section));
                        if (!sec.valid())
                                return nullptr;
                        *size_out = sec.size();
                        return sec.data();
                }
        };

        /**
         * Create a DWARF section loader backed by the given ELF
         * file.  This is templatized to eliminate a static dependency
         * between the libelf++ and libdwarf++, though it can only
         * reasonably be used with elf::elf from libelf++.
         */
        template<typename Elf>
        std::shared_ptr<elf_loader<Elf> > create_loader(const Elf &f)
        {
                return std::make_shared<elf_loader<Elf> >(f);
        }
};

DWARFPP_END_NAMESPACE

//////////////////////////////////////////////////////////////////
// Hash specializations
//

namespace std
{
        template<>
        struct hash<dwarf::die>
        {
                typedef size_t result_type;
                typedef const dwarf::die &argument_type;
                result_type operator()(argument_type a) const;
        };
}

#endif
