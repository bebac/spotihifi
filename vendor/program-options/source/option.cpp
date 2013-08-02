// ----------------------------------------------------------------------------
//
//        Filename:  option.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options/option.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <iomanip>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
option::option(char sname, const std::string& lname, const std::string& desc, ExtractorFunc extractor, const std::string& meta)
    :
    sname_(sname),
    lname_(lname),
    desc_(desc),
    extractor_(extractor),
    meta_(meta)
{
}

// ----------------------------------------------------------------------------
void option::extract(std::istream& is)
{
    extractor_(is);
}

// ----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const option& option)
{
    using namespace std;

    unsigned    name_col_width = 40;
    std::string name_col;

    if ( option.short_name() != -1 ) {
        name_col = std::string("  -") + option.short_name() + ", --" + option.long_name();
    }
    else {
        name_col = std::string("    ") + "  --" + option.long_name();
    }

    if ( option.meta().length() > 0 ) {
        name_col += "=" + option.meta();
    }

    os << setw(name_col_width) << left << name_col;
    if ( name_col.length() >= name_col_width ) {
        os << endl << setw(name_col_width) << "";
    }

    const std::string& desc = option.description();

    size_t pos0 = 0;
    size_t pos1 = desc.find('\n');

    os << desc.substr(pos0, pos1-pos0);

    while ( pos1 != std::string::npos )
    {
        pos0 = pos1+1;
        pos1 = desc.find('\n', pos0);
        os << endl << setw(name_col_width) << "" << desc.substr(pos0, pos1-pos0);
    }

    return os;
}

// ----------------------------------------------------------------------------
} // program_options
