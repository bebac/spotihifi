// ----------------------------------------------------------------------------
//
//        Filename:  argv_streambuf.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __argv_streambuf_h__
#define __argv_streambuf_h__

// ----------------------------------------------------------------------------
#include <streambuf>

// ----------------------------------------------------------------------------
namespace program_options
{

class argv_streambuf : public std::streambuf
{
public:
    typedef std::streambuf::char_type char_type;
    typedef std::streambuf::int_type int_type;
public:
    argv_streambuf(int argc, char* argv[]);
private:
    virtual int_type overflow(int_type ch = traits_type::eof());
    virtual int_type underflow();
    virtual int sync();
private:
    void setg(char_type* buf);
private:
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
private:
    int    argc;
    char** argv;
    int    argi;
private:
    static const char separator[];
};

// ----------------------------------------------------------------------------
} // program_options

// ----------------------------------------------------------------------------
#endif // __argv_streambuf_h__
