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
#include <json.h>

// ----------------------------------------------------------------------------
#include <string>

// ----------------------------------------------------------------------------
class track_t
{
public:
  track_t()
  {
  }
public:
  void track_id(std::string track_id)     { m_track_id = std::move(track_id); }
  void title(std::string title)           { m_title = std::move(title); }
  void track_number(int track_number)     { m_track_number = track_number; }
  void artist(std::string artist)         { m_artist = std::move(artist); }
  void album(std::string album)           { m_album = std::move(album); }
public:
  const std::string& track_id() const     { return m_track_id; }
  const std::string& title() const        { return m_title; }
  int                track_number() const { return m_track_number; }
  const std::string& artist() const       { return m_artist; }
  const std::string& album() const        { return m_album; }
private:
  std::string m_track_id;
  std::string m_title;
  int         m_track_number;
  std::string m_artist;
  std::string m_album;
};

// ----------------------------------------------------------------------------
static inline json::value to_json(const track_t& track)
{
  json::object o;

  o.set("track_id", track.track_id());
  o.set("title", track.title());
  o.set("track_number", track.track_number());
  o.set("artist", track.artist());
  o.set("album", track.album());

  return std::move(o);
}

// ----------------------------------------------------------------------------
#endif // __track_h__