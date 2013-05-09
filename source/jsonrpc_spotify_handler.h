// ----------------------------------------------------------------------------
//
//        Filename:  jsonrpc_spotify_handler.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __jsonrpc_spotify_handler_h__
#define __jsonrpc_spotify_handler_h__

// ----------------------------------------------------------------------------
#include <json.h>
#include <spotify.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <fstream>
#include <future>

// ----------------------------------------------------------------------------
class jsonrpc_spotify_handler : public jsonrpc_handler
{
public:
  jsonrpc_spotify_handler(spotify_t& spotify)
    :
    spotify(spotify)
  {
  }
public:
  virtual void call_method(const std::string& method, json::value params, json::object& response)
  {
    LOG(INFO) << "method: '" << method << "', params:" << params;

    if ( method == "sync" )
    {
      // TODO: Error handling.
      json::object o = params.get<json::object>();

      long long incarnation = -1;
      long long transaction = -1;

      if ( o.has("incarnation") ) {
        incarnation = std::stol(o.get("incarnation").get<json::string>().str());
      }

      if ( o.has("transaction") ) {
        transaction = std::stol(o.get("transaction").get<json::string>().str());
      }

      auto v = spotify.get_tracks(incarnation, transaction).get();

      LOG(INFO) << "sync result"
                << " incarnation=" << v.get("incarnation").get<json::string>()
                << ", transaction=" << v.get("transaction").get<json::string>();

      response.set("result", v);
    }
    else if ( method == "play" )
    {
      if ( params.is_object() )
      {
        json::object o = params.get<json::object>();

        if ( o.has("playlist") )
        {
          auto pl = o.get("playlist").get<json::string>().str();
          if ( pl.length() == 0 )
          {
            spotify.player_stop();
            spotify.build_track_set_all();
          }
          else
          {
            spotify.player_stop();
            spotify.build_track_set_from_playlist(pl);
          }
        }
      }
      spotify.player_play();
      response.set("result", "ok");
    }
    else if ( method == "pause" )
    {
      spotify.player_pause();
      response.set("result", "ok");
    }
    else if ( method == "skip" )
    {
      spotify.player_skip();
      response.set("result", "ok");
    }
    else if ( method == "stop" )
    {
      spotify.player_stop();
      response.set("result", "ok");
    }
    else if ( method == "queue" )
    {
      json::array p = params.get<json::array>();
      auto v = p.at(0).get<json::string>().str();

      spotify.player_play(v);

      response.set("result", "ok");
    }
    else
    {
      json::object e;

      e.set("code", -32601);
      e.set("message", "Method not found");

      LOG(ERROR) << response;

      response.set("error", e);
    }
  }
private:
  spotify_t& spotify;
};

// ----------------------------------------------------------------------------
#endif // __jsonrpc_spotify_handler_h__