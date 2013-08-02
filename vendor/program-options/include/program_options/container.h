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

// ----------------------------------------------------------------------------
template<typename T>
struct extractor {
    extractor(T& value) : value_(value) {}
public:
    void operator()(std::istream& is)
    {
    	is >> value_;
    }
private:
    T& value_;
};

template<typename T>
struct extractor<std::vector<T>> {
    extractor(std::vector<T>& value) : value_(value) {}
public:
    void operator()(std::istream& is)
    {
        T tmp;
        is >> tmp;
        value_.push_back(tmp);
    }
private:
    std::vector<T>& value_;
};

template<>
struct extractor<bool> {
    extractor(bool& value) : value_(value) {}
public:
    void operator()(std::istream& is)
    {
    	value_ = !value_;
    }
private:
    bool& value_;
};

// ----------------------------------------------------------------------------
class container
{
//public:
//    container() {}
public:
    template<typename T>
    void add(char sname, const std::string& lname, const std::string& desc, T& option_value_ref, const std::string& meta="")
    {
        options.emplace_back(sname, lname, desc, extractor<T>(option_value_ref), meta);
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
