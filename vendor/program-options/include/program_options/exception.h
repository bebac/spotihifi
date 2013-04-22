// ----------------------------------------------------------------------------
//
//        Filename:  option.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __program_options__exception_h__
#define __program_options__exception_h__

// ----------------------------------------------------------------------------
#include <string>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
class error : public std::exception
{
public:
    error() {}
public:
    virtual ~error() noexcept {}
protected:
    mutable std::string msg;
};

// ----------------------------------------------------------------------------
class unrecognized_option_error : public error
{
public:
    unrecognized_option_error(const std::string& name);
public:
    ~unrecognized_option_error() noexcept;
public:
    virtual const char* what() const noexcept;
private:
    std::string name;
};

// ----------------------------------------------------------------------------
class invalid_option_value_error : public error
{
public:
    invalid_option_value_error(const std::string& name, const std::string& value);
public:
    ~invalid_option_value_error() noexcept;
public:
    virtual const char* what() const noexcept;
private:
    std::string name;
    std::string value;
};

// ----------------------------------------------------------------------------
} // program_options

#endif // __program_options__exception_h__
