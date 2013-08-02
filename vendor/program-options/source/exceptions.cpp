// ----------------------------------------------------------------------------
//
//        Filename:  exception.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <program_options/exception.h>

// ----------------------------------------------------------------------------
#include <sstream>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
unrecognized_option_error::unrecognized_option_error(const std::string& name)
try
    : error(), name(name)
{
}
catch(...) {
    // Not much we can do.
    //std::cout << __FUNCTION__ << "failed!" << std::endl;
}

// ----------------------------------------------------------------------------
unrecognized_option_error::~unrecognized_option_error() noexcept
{
}

// ----------------------------------------------------------------------------
const char* unrecognized_option_error::what() const noexcept
{
    try
    {
        std::stringstream e;
        e << "unrecognized option '" << name << "'";
        return (msg = std::move(e.str())).c_str();
    }
    catch (...) {
        return "unrecognized_option_error::what() error";
    }
}

// ----------------------------------------------------------------------------
invalid_option_value_error::invalid_option_value_error(const std::string& name, const std::string& value)
try
    : error(), name(name), value(value)
{
}
catch(...) {
    // Not much we can do.
    //std::cout << __FUNCTION__ << "failed!" << std::endl;
}

// ----------------------------------------------------------------------------
invalid_option_value_error::~invalid_option_value_error() noexcept
{
}

// ----------------------------------------------------------------------------
const char* invalid_option_value_error::what() const noexcept
{
    try
    {
        std::stringstream e;
        e << "invalid option value '" << value << "' for option '" << name << "'";
        return (msg = std::move(e.str())).c_str();
    }
    catch (...) {
        return "invalid_option_value_error::what() error";
    }
}

// ----------------------------------------------------------------------------
} // program_options
