// ----------------------------------------------------------------------------
//
//     Filename:  track_stat.cpp
//
//  Description:
//
//       Author:  Benny Bach
//      Company:
//
// ---------------------------------------------------------------------------
#include <track_stat.h>

// ---------------------------------------------------------------------------
#include <fstream>

// ---------------------------------------------------------------------------
void track_stat_t::increase_play_count()
{
    m_play_count++;
    // Increase rating.
    m_rating *= 1.1;
}

// ---------------------------------------------------------------------------
void track_stat_t::increase_skip_count()
{
    m_skip_count++;
    // Decrease rating.
    m_rating *= 0.9;
}

// ---------------------------------------------------------------------------
track_stat_t track_stat_t::from_json(json::object& object)
{
    if ( !object["track_id"].is_string() ) {
        throw std::runtime_error("track stat track_id must be a string!");
    }

    if ( !object["play_count"].is_number() ) {
        throw std::runtime_error("track stat play_count must be a number!");
    }

    if ( !object["skip_count"].is_number() ) {
        throw std::runtime_error("track stat skip_count must be a number!");
    }

    if ( !object["rating"].is_number() ) {
        throw std::runtime_error("track stat rating must be a number!");
    }

    return track_stat_t{
        object["track_id"].as_string(),
        unsigned(object["play_count"].as_number()),
        unsigned(object["skip_count"].as_number()),
        object["rating"].as_number()
    };
}

// ---------------------------------------------------------------------------
void load_track_stats(track_stat_map_t& map, const std::string& filename)
{
  std::ifstream f(filename);

  if ( ! f.good() ) {
    return;
  }

  std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

  json::value  doc;
  json::parser parser(doc);

  parser.parse(str.c_str(), str.length());

  if ( !doc.is_array() ) {
    throw std::runtime_error("track stat file must be a json array!");
  }

  for ( auto& ts : doc.as_array() )
  {
    if ( !ts.is_object() ) {
        throw std::runtime_error("track stat must a json object!");
    }

    track_stat_t track_stat = track_stat_t::from_json(ts.as_object());

    map[track_stat.track_id()] = track_stat;
  }
}

// ---------------------------------------------------------------------------
void save_track_stats(track_stat_map_t& map, const std::string& filename)
{
  std::ofstream f(filename);

  if ( ! f.good() ) {
    throw std::runtime_error("failed to open track stat file!");
  }

  json::array track_stats;
  for ( auto& ts : map ) {
    track_stats.push_back(to_json(ts.second));
  }

  f << track_stats;

  f.close();
}