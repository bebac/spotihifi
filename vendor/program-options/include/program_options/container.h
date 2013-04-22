// ----------------------------------------------------------------------------
//
//        Filename:  container.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __program_options__container_h__
#define __program_options__container_h__

// ----------------------------------------------------------------------------
#include <vector>
#include <istream>

// ----------------------------------------------------------------------------
#include <program_options/option.h>

// ----------------------------------------------------------------------------
namespace program_options
{

class container
{
//public:
//    container() {}
public:
    template<typename T>
    void add(char sname, const std::string& lname, const std::string& desc, T& option_value_ref, const std::string& meta="VALUE") 
    {
        options.emplace_back(sname, lname, desc, [&option_value_ref](std::istream& is) { is >> option_value_ref; }, meta);
    }
public:
    void add(char sname, const std::string& lname, const std::string& desc, bool& option_value_ref, const std::string& meta="")
    {
        options.emplace_back(sname, lname, desc, [&option_value_ref](std::istream& is) { option_value_ref = !option_value_ref; }, meta);
    }
public:
    void parse(int argc, char *argv[]);    
public:
    option& find_by_name(const std::string& name);
    option& find_by_name(char name);
private:
    void extract_option_value(option& option, const std::string& name, std::streambuf& buf);
private:
    std::vector<option> options;
public:
    std::vector<std::string> arguments;
private:
    static const std::string option_end;
public:
    friend std::ostream& operator<<(std::ostream& os, const container& options);
};

// ----------------------------------------------------------------------------
} // program_options

// ----------------------------------------------------------------------------
#endif // __program_options__container_h__
