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
    LOG(INFO) << __FUNCTION__ << " " << event;
    if ( notify_sender )
    {
      json::object notify;

      notify.set("jsonrpc", "2.0");
      notify.set("method", "pb-event");
      notify.set("params", std::move(event));

      notify_sender->send_notify(std::move(notify));
    }
  }
private:
  spotify_t& spotify;
  notify_sender_t* notify_sender;
};

// ----------------------------------------------------------------------------
#endif // __jsonrpc_spotify_handler_h__