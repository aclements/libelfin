#include "dwarf++.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

const line_table::file *
coordinates::get_file() const
{
        DW_AT at = t == type::decl ? DW_AT::decl_file : DW_AT::call_file;
        value v = d.resolve(at);
        if (!v.valid())
                return nullptr;
        auto file = v.as_uconstant();
        if (file == 0)
                return nullptr;
        auto cu = dynamic_cast<const compilation_unit*>(&d.get_unit());
        if (!cu)
                throw format_error(to_string(at) + " in type unit");
        return cu->get_line_table().get_file(file);
}

unsigned
coordinates::get_line() const
{
        value v = d.resolve(t == type::decl ?
                            DW_AT::decl_line : DW_AT::call_line);
        if (!v.valid())
                return 0;
        return v.as_uconstant();
}

unsigned
coordinates::get_column() const
{
        value v = d.resolve(t == type::decl ?
                            DW_AT::decl_column : DW_AT::call_column);
        if (!v.valid())
                return 0;
        return v.as_uconstant();
}

string
coordinates::get_description() const
{
        const line_table::file *file = get_file();
        unsigned line = get_line(), column = get_column();

        string res = file ? file->path : string("UNKNOWN");
        if (line) {
                res.append(":").append(std::to_string(line));
                if (column)
                        res.append(":").append(std::to_string(column));
        }
        return res;
}

coordinates
subroutine::get_decl() const
{
        return coordinates(d);
}

coordinates
subroutine::get_call() const
{
        return coordinates(d, coordinates::type::call);
}

string
subroutine::get_name() const
{
        value v = d.resolve(DW_AT::name);
        if (v.valid())
                return v.as_string();
        return "";
}

DWARFPP_END_NAMESPACE
