// ----------------------------------------------------------------------------
//
//        Filename:  argv_streambuf.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options/argv_streambuf.h>
#include <cstring>
#include <cassert>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
const char argv_streambuf::separator[]  = "\n";

// ----------------------------------------------------------------------------
argv_streambuf::argv_streambuf(int argc, char* argv[])
    :
    std::streambuf(),
    argc(argc),
    argv(argv),
    argi(0)
{
    setg(argv[0]);
}

// ----------------------------------------------------------------------------
argv_streambuf::int_type argv_streambuf::overflow(int_type ch)
{
    return ch;
}

// ----------------------------------------------------------------------------
argv_streambuf::int_type argv_streambuf::underflow()
{
    if ( (argi < argc-1) && (*gptr() == '\0') )
    {
        gptr() == separator+1 ? setg(argv[++argi]) : setg(const_cast<char *>(separator));
        return traits_type::not_eof(sgetc());
    }
    return traits_type::eof();
}

// ----------------------------------------------------------------------------
int argv_streambuf::sync()
{
    return 0;
}

// ----------------------------------------------------------------------------
void argv_streambuf::setg(char_type* buf)
{
    assert(buf);
    std::streambuf::setg(buf, buf, buf+strlen(buf));
}

// ----------------------------------------------------------------------------
std::streampos argv_streambuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which)
{
    // TODO: Validate way / which.
    char_type* buf = argv[argi];
    std::streambuf::setg(buf, gptr()-off, buf+strlen(buf));
    // TODO: Return correct pos.
    return 7;
}

// ----------------------------------------------------------------------------
} // program_options.
