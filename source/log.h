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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cassert>

// ----------------------------------------------------------------------------
#define _log_(LEVEL) \
  if (logging::LEVEL <= logging::logger::level()) \
    logging::logger().get(logging::LEVEL)
;

// ----------------------------------------------------------------------------
namespace logging
{
  enum level { fatal, error, warning, info, debug };

  inline std::string format_time_point(std::chrono::high_resolution_clock::time_point tp)
  {
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;
    using std::setw;
    using std::setfill;

    std::time_t t = high_resolution_clock::to_time_t(tp);

    char buf[64];
    size_t len = strftime( buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::localtime(&t) );

    assert(len > 0);

    std::stringstream ss;
    ss << buf << "." << setw(3) << setfill('0') << duration_cast<milliseconds>(tp.time_since_epoch()).count() % 1000;
    return std::move(ss.str());
  }

  class logger
  {
  public:
    typedef logging::level level_t;
  public:
    logger() {};
  public:
    virtual ~logger()
    {
      _os << std::endl;
      std::cerr << _os.str() << std::flush;
    }
  private:
    // No copying!
    logger(const logger&) = delete;
    logger& operator = (const logger&) = delete;
  public:
    std::ostringstream& get(level_t level)
    {
      using std::setw;
      using std::setfill;

      _os << "["
          << now()
          << ", #" << setw(8) << setfill('0') << std::this_thread::get_id()
          << ", " << level_to_string(level) << "] ";

      return _os;
    }
  public:
    static level_t& level()
    {
      static logger::level_t logger_level = info;
      return logger_level;
    }
  public:
    static std::string level_to_string(level_t level)
    {
      switch(level)
      {
        case fatal:   return "FTL";
        case error:   return "ERR";
        case warning: return "WRN";
        case info:    return "INF";
        case debug:   return "DBG";
        default:      return " - ";
      }
    }
  private:
    std::string now()
    {
      return format_time_point(std::chrono::high_resolution_clock::now());
    }
  private:
    level_t _level;
  protected:
    std::ostringstream _os;
  };
} // namespace logging

#endif // __log_h__
