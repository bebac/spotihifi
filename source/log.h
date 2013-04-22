// ----------------------------------------------------------------------------
//
//        Filename:  log.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __log_h__
#define __log_h__

// ----------------------------------------------------------------------------
#include <sstream>
#include <thread>
#include <time.h>

// ----------------------------------------------------------------------------
#if defined (_WIN32)
#include <sys/timeb.h>
#elif defined (__GNUG__)
#include <sys/time.h>
#endif

// ----------------------------------------------------------------------------
namespace logging {

    enum level { FATAL, ERROR, WARNING, INFO, DEBUG };

// ----------------------------------------------------------------------------
#define LOG(LEVEL) \
    if (logging::LEVEL > logging::logger::level()); \
    else logging::logger().get(logging::LEVEL)


// ----------------------------------------------------------------------------
class logger
{
public:
  typedef logging::level level_t;

private:
  level_t _level;

protected:
    std::ostringstream _os;

public:
    logger();
    virtual ~logger();

    std::ostringstream& get(level_t level);

    static level_t& level();
    static std::string levelToString(level_t level);

private:
    // No copying!
    logger(const logger&) = delete;
    logger& operator = (const logger&) = delete;

    std::string now();
};


// ----------------------------------------------------------------------------
inline logger::logger()
{
}


// ----------------------------------------------------------------------------
inline logger::~logger()
{
    _os << std::endl;
    fprintf(stderr, "%s", _os.str().c_str());
    fflush(stderr);
}


// ----------------------------------------------------------------------------
inline std::ostringstream& logger::get(level_t level)
{
    _os << "[" << now() << ", #";
    _os.width(8);
    _os.fill('0');
    _os << std::this_thread::get_id() << "]";
    _os << " " << levelToString(level) << ": ";
    _os << std::string(level > DEBUG ? level - DEBUG : 0, '\t');
    return _os;
}


// ----------------------------------------------------------------------------
inline logger::level_t& logger::level()
{
    static logger::level_t logger_level = DEBUG;
    return logger_level;
}


// ----------------------------------------------------------------------------
inline std::string logger::levelToString(level_t level)
{
    switch(level)
    {
    case FATAL:   return "FATAL -";
    case ERROR:   return "ERROR -";
    case WARNING: return "WARN  -";
    case INFO:    return "INFO  -";
    case DEBUG:   return "DEBUG -";
    default:      return "  -   -";
    }
}


// ----------------------------------------------------------------------------

inline std::string logger::now()
{
#if defined (_WIN32)

    struct tm     ts;
    struct _timeb timebuffer;

    _ftime64_s( &timebuffer );
    gmtime_s( &ts, &timebuffer.time );

    char buf[64];

    size_t len = strftime( buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ts );
    sprintf_s( buf+len, sizeof(buf)-len, ".%03ld", timebuffer.millitm );

    return buf;

#elif defined (__GNUG__)

    struct tm      ts;
    struct timeval tp;

    gettimeofday( &tp, NULL );
    gmtime_r( &tp.tv_sec, &ts );

    char buf[64];

    size_t len = strftime( buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &ts );
    snprintf( buf+len, sizeof(buf)-len, ".%03ld", tp.tv_usec/1000 );

    return buf;
#else
#error "no Logger::now implementation"
#endif
}

} // namespace logging.


// ----------------------------------------------------------------------------
#endif // __log_h__
