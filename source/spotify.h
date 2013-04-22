// ----------------------------------------------------------------------------
#ifndef __spotify_h__
#define __spotify_h__

// ----------------------------------------------------------------------------
#include <cmdque.h>
#include <audio_output_alsa.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <memory>
#include <thread>
#include <cassert>
#include <deque>

// ----------------------------------------------------------------------------
#include <libspotify/api.h>

// ----------------------------------------------------------------------------
extern const uint8_t g_appkey[];
extern const size_t  g_appkey_size;

// ----------------------------------------------------------------------------
class spotify_error : public std::runtime_error
{
private:
  const sp_error& error;
public:
  spotify_error(const sp_error& error)
    : std::runtime_error(sp_error_message(error)), error(error)
  {
  }
};

// ----------------------------------------------------------------------------
class spotify_t
{
public:
  spotify_t()
    :
    m_session(0),
    m_session_logged_in(false),
    m_running(true),
    m_session_next_timeout(0),
    m_track(0),
    m_track_playing(false),
    m_audio_output(),
    m_thr{&spotify_t::main, this}
  {
  }
  ~spotify_t()
  {
    std::cout << "spotify_t::" << __FUNCTION__ << std::endl;
    if ( m_running ) {
      stop();
    }
    m_thr.join();
    std::cout << "spotify_t::" << __FUNCTION__ << " joined thread" << std::endl;
  }
public:
  void stop()
  {
    m_command_queue.push([this]() {
      this->m_running = false;
    });
  }
public:
  void login(const std::string& username, const std::string& password)
  {
    m_command_queue.push([=]() {
      sp_session_login(m_session, username.c_str(), password.c_str(), 0, 0);
    });
  }
public:
  void player_play(const std::string& uri)
  {
    m_command_queue.push([=]() {
      m_play_queue.push_back(uri);
      if ( m_session_logged_in && !m_track ) {
        play_next_from_queue();
      }
    });
  }
public:
  void player_stop()
  {
#if 0
    m_command_queue.push([=]()
    {
      if ( m_track_playing )
      {
        sp_session_player_unload(m_session);
        m_track_playing = false;
      }
      if ( m_track )
      {
        sp_track_release(m_track);
        m_track = 0;
      }
      m_audio_output.reset();
    });
#endif
  }
public:
  void player_skip()
  {
#if 0
    m_command_queue.push([=]()
    {
      if ( m_track_playing )
      {
        sp_session_player_unload(m_session);
        m_track_playing = false;
      }
      if ( m_track )
      {
        sp_track_release(m_track);
        m_track = 0;
      }
      play_next_from_queue();
    });
#endif
  }
public:
  void player_queue_clear()
  {
#if 0
    m_command_queue.push([=]()
    {
      m_play_queue.clear();
    });
#endif
  }
private:
  void init();
  void main();
private:
  void logged_in_handler();
  void track_loaded_handler();
  void end_of_track_handler();
  void process_events_handler();
  void play_next_from_queue();
private:
  std::shared_ptr<audio_output_t> get_audio_output(int rate, int channels);
  std::shared_ptr<audio_output_t> get_audio_output();
private:
  // spotify_t callbacks.
  static void logged_in_cb(sp_session *session, sp_error error);
  static void logged_out_cb(sp_session *session);
  static void metadata_updated_cb(sp_session *session);
  static void connection_error_cb(sp_session *session, sp_error error);
  static void message_to_user_cb(sp_session *session, const char* message);
  static void notify_main_thread_cb(sp_session *session);
  static int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  static void play_token_lost_cb(sp_session *session);
  static void log_message_cb(sp_session *session, const char* data);
  static void end_of_track_cb(sp_session *session);
  static void stream_error_cb(sp_session *session, sp_error error);
  static void user_info_updated_cb(sp_session *session);
  static void start_playback_cb(sp_session *session);
  static void stop_playback_cb(sp_session *session);
  static void get_audio_buffer_stats_cb(sp_session *session, sp_audio_buffer_stats *stats);
  static void offline_status_updated_cb(sp_session *session);
  static void offline_error_cb(sp_session *session, sp_error error);
  static void credentials_blob_updated_cb(sp_session *session, const char* blob);
protected:
  sp_session* m_session;
  //sp_session_config m_session_config;
  bool m_session_logged_in;
  bool m_running;
  cmdque_t m_command_queue;
  int m_session_next_timeout;
  std::deque<std::string> m_play_queue;
  sp_track* m_track;
  sp_playlistcontainer* m_playlistcontainer;
  bool m_track_playing;
  std::shared_ptr<audio_output_t> m_audio_output;
  std::thread m_thr;
};

// ----------------------------------------------------------------------------
#endif // __spotify_h__
