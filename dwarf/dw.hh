#ifndef _DWARFPP_DW_HH_
#define _DWARFPP_DW_HH_

#include <cstdint>
#include <string>

#ifndef DWARFPP_BEGIN_NAMESPACE
#define DWARFPP_BEGIN_NAMESPACE namespace dwarf {
#define DWARFPP_END_NAMESPACE   }
#endif

DWARFPP_BEGIN_NAMESPACE

// Integer representations (Section 7.26)
typedef std::int8_t sbyte;
typedef std::uint8_t ubyte;
typedef std::uint16_t uhalf;
typedef std::uint32_t uword;

typedef std::uint64_t sec_offset;
typedef std::uint64_t sec_length;

// DIE tags (Section 7, figure 18).  typedef, friend, and namespace
// have a trailing underscore because they are reserved words.
enum class DW_TAG
{
        array_type               = 0x01,
        class_type               = 0x02,
        entry_point              = 0x03,
        enumeration_type         = 0x04,
        formal_parameter         = 0x05,
        imported_declaration     = 0x08,
        label                    = 0x0a,
        lexical_block            = 0x0b,
        member                   = 0x0d,
        pointer_type             = 0x0f,
        reference_type           = 0x10,
        compile_unit             = 0x11,
        string_type              = 0x12,
        structure_type           = 0x13,
        subroutine_type          = 0x15,
        typedef_                 = 0x16,

        union_type               = 0x17,
        unspecified_parameters   = 0x18,
        variant                  = 0x19,
        common_block             = 0x1a,
        common_inclusion         = 0x1b,
        inheritance              = 0x1c,
        inlined_subroutine       = 0x1d,
        module                   = 0x1e,
        ptr_to_member_type       = 0x1f,
        set_type                 = 0x20,
        subrange_type            = 0x21,
        with_stmt                = 0x22,
        access_declaration       = 0x23,
        base_type                = 0x24,
        catch_block              = 0x25,
        const_type               = 0x26,
        constant                 = 0x27,
        enumerator               = 0x28,
        file_type                = 0x29,
        friend_                  = 0x2a,

        namelist                 = 0x2b,
        namelist_item            = 0x2c,
        packed_type              = 0x2d,
        subprogram               = 0x2e,
        template_type_parameter  = 0x2f,
        template_value_parameter = 0x30,
        thrown_type              = 0x31,
        try_block                = 0x32,
        variant_part             = 0x33,
        variable                 = 0x34,
        volatile_type            = 0x35,
        dwarf_procedure          = 0x36,
        restrict_type            = 0x37,
        interface_type           = 0x38,
        namespace_               = 0x39,
        imported_module          = 0x3a,
        unspecified_type         = 0x3b,
        partial_unit             = 0x3c,
        imported_unit            = 0x3d,
        condition                = 0x3f,

        shared_type              = 0x40,
        type_unit                = 0x41,
        rvalue_reference_type    = 0x42,
        template_alias           = 0x43,
        lo_user                  = 0x4080,
        hi_user                  = 0xffff,
};

std::string
to_string(DW_TAG v);

// Child determination (Section 7, figure 19).
enum class DW_CHILDREN : ubyte
{
        no  = 0x00,
        yes = 0x01,
};

std::string
to_string(DW_CHILDREN v);

// Attribute names (Section 7, figure 20).  inline, friend, mutable,
// and explicit have a trailing underscore because they are reserved
// words.
enum class DW_AT
{
        sibling              = 0x01, // reference
        location             = 0x02, // exprloc, loclistptr
        name                 = 0x03, // string
        ordering             = 0x09, // constant
        byte_size            = 0x0b, // constant, exprloc, reference
        bit_offset           = 0x0c, // constant, exprloc, reference
        bit_size             = 0x0d, // constant, exprloc, reference
        stmt_list            = 0x10, // lineptr
        low_pc               = 0x11, // address
        high_pc              = 0x12, // address, constant
        language             = 0x13, // constant
        discr                = 0x15, // reference
        discr_value          = 0x16, // constant
        visibility           = 0x17, // constant
        import               = 0x18, // reference
        string_length        = 0x19, // exprloc, loclistptr
        common_reference     = 0x1a, // reference
        comp_dir             = 0x1b, // string
        const_value          = 0x1c, // block, constant, string

        containing_type      = 0x1d, // reference
        default_value        = 0x1e, // reference
        inline_              = 0x20, // constant
        is_optional          = 0x21, // flag
        lower_bound          = 0x22, // constant, exprloc, reference
        producer             = 0x25, // string
        prototyped           = 0x27, // flag
        return_addr          = 0x2a, // exprloc, loclistptr
        start_scope          = 0x2c, // constant, rangelistptr
        bit_stride           = 0x2e, // constant, exprloc, reference
        upper_bound          = 0x2f, // constant, exprloc, reference
        abstract_origin      = 0x31, // reference
        accessibility        = 0x32, // constant
        address_class        = 0x33, // constant
        artificial           = 0x34, // flag
        base_types           = 0x35, // reference
        calling_convention   = 0x36, // constant
        count                = 0x37, // constant, exprloc, reference
        data_member_location = 0x38, // constant, exprloc, loclistptr
        decl_column          = 0x39, // constant

        decl_file            = 0x3a, // constant
        decl_line            = 0x3b, // constant
        declaration          = 0x3c, // flag
        discr_list           = 0x3d, // block
        encoding             = 0x3e, // constant
        external             = 0x3f, // flag
        frame_base           = 0x40, // exprloc, loclistptr
        friend_              = 0x41, // reference
        identifier_case      = 0x42, // constant
        macro_info           = 0x43, // macptr
        namelist_item        = 0x44, // reference
        priority             = 0x45, // reference
        segment              = 0x46, // exprloc, loclistptr
        specification        = 0x47, // reference
        static_link          = 0x48, // exprloc, loclistptr
        type                 = 0x49, // reference
        use_location         = 0x4a, // exprloc, loclistptr
        variable_parameter   = 0x4b, // flag
        virtuality           = 0x4c, // constant
        vtable_elem_location = 0x4d, // exprloc, loclistptr

        allocated            = 0x4e, // constant, exprloc, reference
        associated           = 0x4f, // constant, exprloc, reference
        data_location        = 0x50, // exprloc
        byte_stride          = 0x51, // constant, exprloc, reference
        entry_pc             = 0x52, // address
        use_UTF8             = 0x53, // flag
        extension            = 0x54, // reference
        ranges               = 0x55, // rangelistptr
        trampoline           = 0x56, // address, flag, reference, string
        call_column          = 0x57, // constant
        call_file            = 0x58, // constant
        call_line            = 0x59, // constant
        description          = 0x5a, // string
        binary_scale         = 0x5b, // constant
        decimal_scale        = 0x5c, // constant
        small                = 0x5d, // reference
        decimal_sign         = 0x5e, // constant
        digit_count          = 0x5f, // constant
        picture_string       = 0x60, // string
        mutable_             = 0x61, // flag

        threads_scaled       = 0x62, // flag
        explicit_            = 0x63, // flag
        object_pointer       = 0x64, // reference
        endianity            = 0x65, // constant
        elemental            = 0x66, // flag
        pure                 = 0x67, // flag
        recursive            = 0x68, // flag
        signature            = 0x69, // reference (new in DWARF 4)
        main_subprogram      = 0x6a, // flag (new in DWARF 4)
        data_bit_offset      = 0x6b, // constant (new in DWARF 4)
        const_expr           = 0x6c, // flag (new in DWARF 4)
        enum_class           = 0x6d, // flag (new in DWARF 4)
        linkage_name         = 0x6e, // string (new in DWARF 4)

        lo_user              = 0x2000,
        hi_user              = 0x3fff,
};

std::string
to_string(DW_AT v);

// Attribute form encodings (Section 7, figure 21)
enum class DW_FORM
{
        addr         = 0x01,    // address
        block2       = 0x03,    // block
        block4       = 0x04,    // block
        data2        = 0x05,    // constant
        data4        = 0x06,    // constant
        data8        = 0x07,    // constant
        string       = 0x08,    // string
        block        = 0x09,    // block
        block1       = 0x0a,    // block
        data1        = 0x0b,    // constant
        flag         = 0x0c,    // flag
        sdata        = 0x0d,    // constant
        strp         = 0x0e,    // string
        udata        = 0x0f,    // constant
        ref_addr     = 0x10,    // reference
        ref1         = 0x11,    // reference
        ref2         = 0x12,    // reference
        ref4         = 0x13,    // reference
        ref8         = 0x14,    // reference

        ref_udata    = 0x15,    // reference
        indirect     = 0x16,    // (Section 7.5.3)
        sec_offset   = 0x17,    // lineptr, loclistptr, macptr, rangelistptr
        exprloc      = 0x18,    // exprloc
        flag_present = 0x19,    // flag
        ref_sig8     = 0x20,    // reference
};

std::string
to_string(DW_FORM v);

DWARFPP_END_NAMESPACE

#endif
