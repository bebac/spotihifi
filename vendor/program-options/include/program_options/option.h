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
#ifndef __program_options__option_h__
#define __program_options__option_h__

// ----------------------------------------------------------------------------
#include <functional>
#include <string>

// ----------------------------------------------------------------------------
namespace program_options
{

// ----------------------------------------------------------------------------
class option
{
public:
    typedef std::function<void(std::istream&)> ExtractorFunc;
public:
    friend std::ostream& operator<<(std::ostream& os, const option& option);
public:
    option(char sname, const std::string& lname, const std::string& desc, ExtractorFunc extractor, const std::string& meta);
public:
    const char         short_name()  const { return sname; }
    const std::string& long_name()   const { return lname; }
    const std::string& description() const { return desc;  }
    const std::string& meta()        const { return meta_; }
public:
    void extract(std::istream& is);
private:
    char          sname;
    std::string   lname;
    std::string   desc;
    ExtractorFunc extractor;
    std::string   meta_;
};

// ----------------------------------------------------------------------------
} // program_options

// ----------------------------------------------------------------------------
#endif // __program_options__option_h__
