#include "internal.hh"

using namespace std;

DWARFPP_BEGIN_NAMESPACE

struct loader::impl
{
        map<section_type, shared_ptr<section> > sections;
};

loader::loader()
        : m(new impl)
{
}

loader::~loader()
{
}

void
loader::register_section(section_type type, const void *begin,
                         unsigned long long length)
{
        m->sections[type] = make_shared<section>(type, begin, length);
}

dwarf
loader::load()
{
        return dwarf(dwarf::impl::create(m->sections));
}

DWARFPP_END_NAMESPACE
