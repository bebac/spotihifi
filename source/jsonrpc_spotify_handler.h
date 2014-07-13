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
#include <json/json.h>
#include <jsonrpc.h>
#include <spotify.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <fstream>
#include <future>

// ----------------------------------------------------------------------------
class notify_sender_t
{
public:
  virtual void send_notify(json::value message) = 0;
};

// ----------------------------------------------------------------------------
class jsonrpc_spotify_handler : public jsonrpc_handler, public player_observer_t
{
public:
  jsonrpc_spotify_handler(spotify_t& spotify)
    :
    spotify(spotify),
    notify_sender(0)
  {
  }
public:
  ~jsonrpc_spotify_handler()
  {
  }
public:
  void set_notify_sender(notify_sender_t* sender)
  {
    notify_sender = sender;
  }
public:
  void player_observer_attach(std::shared_ptr<player_observer_t> observer)
  {
    spotify.observer_attach(observer);
  }
public:
  void player_observer_detach(std::shared_ptr<player_observer_t> observer)
  {
    spotify.observer_detach(observer);
  }
public:
  virtual void call_method(const std::string& method, json::value params, json::object& response)
  {
    _log_(info) << "method: '" << method << "', params:" << params;

    if ( method == "sync" )
    {
      // TODO: Error handling.
      json::object o = params.as_object();

      long long incarnation = -1;
      long long transaction = -1;

      if ( o["incarnation"].is_string() ) {
        incarnation = std::stol(o["incarnation"].as_string());
      }

      if ( o["transaction"].is_string() ) {
        transaction = std::stol(o["transaction"].as_string());
      }

      auto v = spotify.get_tracks(incarnation, transaction).get();

      _log_(info)
        << "sync result"
        << " incarnation=" << v["incarnation"].as_string()
        << ", transaction=" << v["transaction"].as_string();

      response["result"] = v;
    }
    else if ( method == "play" )
    {
      if ( params.is_object() )
      {
        json::object o = params.as_object();

        if ( o["playlist"].is_string() )
        {
          auto pl = o["playlist"].as_string();
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
      response["result"] = "ok";
    }
    else if ( method == "pause" )
    {
      spotify.player_pause();
      response["result"] = "ok";
    }
    else if ( method == "skip" )
    {
      spotify.player_skip();
      response["result"] = "ok";
    }
    else if ( method == "stop" )
    {
      spotify.player_stop();
      response["result"] = "ok";
    }
    else if ( method == "queue" )
    {
      json::array p = params.as_array();
      auto v = p[0].as_string();

      spotify.player_play(v);

      response["result"] = "ok";
    }
    else if ( method == "get-cover" )
    {
      if ( params.is_object() )
      {
        json::object o = params.as_object();

        if ( o["track_id"].is_string() && o["cover_id"].is_string() )
        {
          auto v = spotify.get_cover(o["track_id"].as_string(), o["cover_id"].as_string()).get();
          if ( !v["error"].is_null() ) {
            response["error"] = std::move(v["error"]);
          }
          else {
            response["result"] = std::move(v);
          }
        }
        else
        {
          response["error"] = json::object{ { "code", -32602 }, { "message", "Invalid parameters" } };
        }
      }
      else
      {
        response["error"] = json::object{ { "code", -32602 }, { "message", "Invalid parameters" } };
      }
    }
    else
    {
      response["error"] = json::object{ { "code", -32601 }, { "message", "Method not found" } };
      _log_(error) << response;
    }
  }
public:
  //
  // Implement the player observer interface.
  //
  // NOTE: The player_state_event callback is called from spotify service context.
  //       There should probably be some sort of synchronization to prevent the
  //       the notify_sender from being unset after checking whether it is set.
  //       Since it is not currently unset I'll let it slide.
  //
  void player_state_event(json::object event)
  {
    _log_(info) << __FUNCTION__ << " " << event;
    if ( notify_sender )
    {
      json::object notify{
        { "jsonrpc", "2.0" },
        { "method", "pb-event" },
        { "params", std::move(event) }
      };

      notify_sender->send_notify(std::move(notify));
    }
  }
private:
  spotify_t& spotify;
  notify_sender_t* notify_sender;
};

// ----------------------------------------------------------------------------
#endif // __jsonrpc_spotify_handler_h__
