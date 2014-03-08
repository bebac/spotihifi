// ----------------------------------------------------------------------------
//
//        Filename:  track.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __track_h__
#define __track_h__

// ----------------------------------------------------------------------------
#include <json/json.h>

// ----------------------------------------------------------------------------
#include <string>
#include <set>

// ----------------------------------------------------------------------------
class track_t
{
public:
  typedef std::set<std::string> pl_set_t;
public:
  track_t()
  {
  }
public:
  void track_id(std::string track_id)         { m_track_id = std::move(track_id); }
  void title(std::string title)               { m_title = std::move(title); }
  void track_number(int track_number)         { m_track_number = track_number; }
  void duration(int duration_in_msecs)        { m_duration = duration_in_msecs; }
  void artist(std::string artist)             { m_artist = std::move(artist); }
  void album(std::string album)               { m_album = std::move(album); }
  void cover_id(std::string cover_id)         { m_cover_id = std::move(cover_id); }
  void playlists(pl_set_t playlists)          { m_playlists = std::move(playlists); }
  void playlists_add(std::string playlist)    { m_playlists.insert(std::move(playlist)); }
  void playlists_remove(const std::string& playlist) { m_playlists.erase(playlist); }
  size_t playlists_size()                     { return m_playlists.size(); }
public:
  const std::string& track_id() const         { return m_track_id; }
  const std::string& title() const            { return m_title; }
  int                track_number() const     { return m_track_number; }
  int                duration() const         { return m_duration; }
  const std::string& artist() const           { return m_artist; }
  const std::string& album() const            { return m_album; }
  const std::string& cover_id() const         { return m_cover_id; }
  const pl_set_t&    playlists() const        { return m_playlists; }
private:
  std::string m_track_id;
  std::string m_title;
  int         m_track_number;
  int         m_duration;
  std::string m_artist;
  std::string m_album;
  std::string m_cover_id;
  pl_set_t    m_playlists;
};

// ----------------------------------------------------------------------------
static inline json::value to_json(const track_t& track)
{
  json::object o {
    { "track_id", track.track_id() },
    { "title", track.title() },
    { "track_number", track.track_number() },
    { "duration", track.duration() },
    { "artist", track.artist() },
    { "album", track.album() },
    { "cover_id", track.cover_id() }
  };

  json::array playlists;
  for ( const auto& pl : track.playlists() ) {
    playlists.push_back(pl);
  }

  o["playlists"] = playlists;

  return std::move(o);
}

// ----------------------------------------------------------------------------
#endif // __track_h__