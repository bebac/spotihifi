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
  spotify_t();
public:
  ~spotify_t();
public:
  void stop();
  void login(const std::string& username, const std::string& password);
  void player_play();
  void player_play(const std::string& uri);
  void player_skip();
  void player_pause();
  void player_stop();
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
  bool import_playlist(sp_playlist* pl);
private:
  std::shared_ptr<audio_output_t> get_audio_output(int rate, int channels);
  std::shared_ptr<audio_output_t> get_audio_output();
private:
  // session callbacks.
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
  // playlist callbacks.
  static void playlist_state_changed_cb(sp_playlist* pl, void* userdata);
protected:
  sp_session* m_session;
  bool m_session_logged_in;
  bool m_running;
  cmdque_t m_command_queue;
  int m_session_next_timeout;
  std::deque<std::string> m_play_queue;
  sp_track* m_track;
  sp_playlistcontainer* m_playlistcontainer;
  sp_playlist* m_starred;
  bool m_track_playing;
  std::shared_ptr<audio_output_t> m_audio_output;
  std::thread m_thr;
};

// ----------------------------------------------------------------------------
#endif // __spotify_h__
