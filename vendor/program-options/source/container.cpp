// ----------------------------------------------------------------------------
//
//        Filename:  container.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options/container.h>
#include <program_options/argv_streambuf.h>
#include <program_options/exception.h>

// ----------------------------------------------------------------------------
#include <cstring>
#include <iostream>
#include <iterator>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
struct tokens: std::ctype<char>
{
    tokens(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        typedef std::ctype<char> cctype;
        static const cctype::mask *const_rc= cctype::classic_table();

        static cctype::mask rc[cctype::table_size];
        std::memcpy(rc, const_rc, cctype::table_size * sizeof(cctype::mask));

        rc[' '] = std::ctype_base::print;
        return &rc[0];
    }
};

// ----------------------------------------------------------------------------
const std::string container::option_end = " =\n";

// ----------------------------------------------------------------------------
void container::parse(int argc, char *argv[])
{
    argv_streambuf buf(argc, argv);

    std::istreambuf_iterator<char> eos;
    std::istreambuf_iterator<char> iit (&buf);

    while ( iit != eos )
    {
        if ( *iit == '-' )
        {
            if ( *(++iit) == '-' )
            {
                std::string name;

                while ( ++iit != eos && option_end.find(*iit) == std::string::npos ) {
                    name += *iit;
                }

                if ( iit != eos ) {
                    iit++;
                }
                // Trigger possible underflow to make in_avail correct for argv_streambuf.
                buf.sgetc();

                auto& option = find_by_name(name);

                extract_option_value(option, name, buf);
            }
            else
            {
                bool cont = true;
                do
                {
                    char name = static_cast<char>(*iit++);

                    auto& option = find_by_name(name);

                    if ( *iit == '\n' )
                    {
                        iit++;
                        cont = false;
                    }

                    // Trigger possible underflow to make in_avail correct for argv_streambuf.
                    buf.sgetc();

                    extract_option_value(option, std::string(1, name), buf);

                    if ( option.meta().length() > 0 )
                    {
                        *iit++;
                        cont = false;
                    }

                }
                while( cont && iit != eos );
            }
        }
        else if ( *iit == '\n' )
        {
            iit++;
        }
        else
        {
            std::string arg;

            while (iit != eos && *iit != '\n') {
                arg += *iit++;
            }

            arguments.emplace_back(arg);
        }
    }
}

// ----------------------------------------------------------------------------
option& container::find_by_name(const std::string& name)
{
    for ( auto& option : options )
    {
        if ( option.long_name() == name ) {
            return option;
        }
    }
    throw unrecognized_option_error(name);
}

// ----------------------------------------------------------------------------
option& container::find_by_name(char name)
{
    for ( auto& option : options )
    {
        if ( option.short_name() == name ) {
            return option;
        }
    }
    throw unrecognized_option_error(std::string(1, name));
}

// ----------------------------------------------------------------------------
void container::extract_option_value(option& option, const std::string& name, std::streambuf& buf)
{
    std::istream is(&buf);
    is.imbue(std::locale(std::locale(), new tokens()));

    // Save number of available characters so we can back track on error.
    std::size_t pos = buf.in_avail();

    option.extract(is);

    if ( is.fail() || is.bad() || (option.meta().length() > 0 && !(is.peek() == std::streambuf::traits_type::eof() || is.peek() == '\n')) )
    {
        std::istreambuf_iterator<char> eos;
        std::istreambuf_iterator<char> iit (is.rdbuf());

        std::string value;

        is.rdbuf()->pubseekoff(pos-is.rdbuf()->in_avail(), std::ios_base::cur);
        while ( iit != eos && *iit != '\n' ) {
            value += *iit++;
        }
        throw invalid_option_value_error(name, value);
    }
}

// ----------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, const container& options)
{
    for ( auto option : options.options ) {
        os << option << std::endl;
    }
    return os;
}

// ----------------------------------------------------------------------------
} // program_options
