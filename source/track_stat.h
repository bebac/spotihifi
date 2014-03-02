// ----------------------------------------------------------------------------
//
//        Filename:  track_stat.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __track_stat_h__
#define __track_stat_h__

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <string>
#include <map>

// ----------------------------------------------------------------------------
class track_stat_t
{
public:
  track_stat_t()
    :
    m_track_id(),
    m_play_count(0),
    m_skip_count(0),
    m_rating(1.0)
  {
  }
public:
  track_stat_t(std::string id)
    :
    m_track_id(std::move(id)),
    m_play_count(0),
    m_skip_count(0),
    m_rating(1.0)
  {
  }
public:
  track_stat_t(std::string id, unsigned pc, unsigned sc, double rating)
    :
    m_track_id(std::move(id)),
    m_play_count(pc),
    m_skip_count(sc),
    m_rating(rating)
  {
  }
public:
  void track_id(std::string track_id)         { m_track_id = std::move(track_id); }
public:
  void increase_play_count();
  void increase_skip_count();
public:
  const std::string& track_id() const         { return m_track_id; }
  unsigned           play_count() const       { return m_play_count; }
  unsigned           skip_count() const       { return m_skip_count; }
  double             rating()     const       { return m_rating; }
public:
  static track_stat_t from_json(json::object& object);
private:
  std::string m_track_id;
  unsigned    m_play_count;
  unsigned    m_skip_count;
  double      m_rating;
};

// ----------------------------------------------------------------------------
static inline json::value to_json(const track_stat_t& track_stat)
{
  json::object o {
    { "track_id", track_stat.track_id() },
    { "play_count", track_stat.play_count() },
    { "skip_count", track_stat.skip_count() },
    { "rating", track_stat.rating() }
  };

  return std::move(o);
}

// ----------------------------------------------------------------------------
typedef std::map<std::string, track_stat_t> track_stat_map_t;

// ----------------------------------------------------------------------------
void load_track_stats(track_stat_map_t& map, const std::string& filename);
void save_track_stats(track_stat_map_t& map, const std::string& filename);


// ----------------------------------------------------------------------------
#endif // __track_stat_h__