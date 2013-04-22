#include <spotify.h>

// ----------------------------------------------------------------------------
void spotify_t::init()
{
  sp_session_callbacks callbacks = {
    &logged_in_cb,
    &logged_out_cb,
    &metadata_updated_cb,
    &connection_error_cb,
    &message_to_user_cb,
    &notify_main_thread_cb,
    &music_delivery,
    &play_token_lost_cb,
    &log_message_cb,
    &end_of_track_cb,
    &stream_error_cb,
    0, //&user_info_updated_cb,
    &start_playback_cb,
    &stop_playback_cb,
    &get_audio_buffer_stats_cb,
    0, //&offline_status_updated_cb,
    0, //&offline_error_cb,
    &credentials_blob_updated_cb
  };

  sp_session_config config;

  config.callbacks = &callbacks;
  config.api_version = SPOTIFY_API_VERSION;
  config.cache_location = "tmp";
  config.settings_location = "tmp";
  config.application_key = g_appkey;
  config.application_key_size = g_appkey_size;
  config.user_agent = "spotd";
  config.userdata = this;
  config.compress_playlists = false;
  config.dont_save_metadata_for_playlists = true;
  config.initially_unload_playlists = false;
  config.device_id = 0;
  config.proxy = "";
  config.proxy_username = 0;
  config.proxy_password = 0;
  config.ca_certs_filename = 0;
  config.tracefile = 0;

  sp_error error = sp_session_create(&config, &m_session);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }

  error = sp_session_preferred_bitrate(m_session, SP_BITRATE_320k);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }

  error = sp_session_set_volume_normalization(m_session, true);
  if ( SP_ERROR_OK != error ) {
    throw spotify_error(error);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::main()
{
  try
  {
    init();
    while ( m_running )
    {
        auto cmd = m_command_queue.pop(std::chrono::seconds(1), []{
            //std::cout << "timeout" << std::endl;
        });
        cmd();
    }

    std::cout << "spotify_t::" << __FUNCTION__ << " release session " << m_session << std::endl;
    sp_session_release(m_session);
  }
  catch(std::exception& e)
  {
    std::cout << "spotify_t::" << __FUNCTION__ << " error=" << e.what() << std::endl;
    sp_session_release(m_session);
  }
  catch(...)
  {
    std::cout << "spotify_t::" << __FUNCTION__ << " error=<unknown>" << std::endl;
    sp_session_release(m_session);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::logged_in_handler()
{
  std::cout << "spotify_t::" << __FUNCTION__ << std::endl;
  m_session_logged_in = true;
  //play_next_from_queue();
}

// ----------------------------------------------------------------------------
void spotify_t::track_loaded_handler()
{
  if ( !m_track_playing )
  {
    sp_artist* artist;
    sp_album* album;

    sp_artist_add_ref(artist = sp_track_artist(m_track, 0));
    sp_album_add_ref(album = sp_track_album(m_track));

    std::cout << "Playing : " << sp_artist_name(artist) << " - " << sp_album_name(album) << " - " << sp_track_name(m_track) << std::endl;

    sp_session_player_load(m_session, m_track);
    sp_session_player_play(m_session, 1);

    m_track_playing = true;

    sp_artist_release(artist);
    sp_album_release(album);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::end_of_track_handler()
{
  std::cout << "spotify_t::" << __FUNCTION__ << std::endl;

  assert(m_session);
  assert(m_track);
  assert(m_track_playing);

  // Release current track.
  sp_session_player_unload(m_session);
  sp_track_release(m_track);

  m_track = 0;
  m_track_playing = false;

  play_next_from_queue();
}

// ----------------------------------------------------------------------------
void spotify_t::process_events_handler()
{
  do
  {
    sp_session_process_events(m_session, &m_session_next_timeout);
  } while (m_session_next_timeout == 0);
}

// ----------------------------------------------------------------------------
void spotify_t::play_next_from_queue()
{
  if ( m_play_queue.size() > 0 )
  {
    auto uri = m_play_queue.front();

    m_play_queue.pop_front();

    sp_link* link = sp_link_create_from_string(uri.c_str());
    sp_track_add_ref(m_track = sp_link_as_track(link));
    sp_link_release(link);

    assert(m_track);

    sp_error err = sp_track_error(m_track);
    if (err == SP_ERROR_OK) {
      track_loaded_handler();
    }
  }
  else {
    m_audio_output.reset();
  }
}

// ----------------------------------------------------------------------------
std::shared_ptr<audio_output_t> spotify_t::get_audio_output(int rate, int channels)
{
  assert(rate == 44100);
  assert(channels == 2);

  if ( ! m_audio_output.get() ) {
      m_audio_output = std::make_shared<audio_output_t>();
  }

  return m_audio_output;
}

// ----------------------------------------------------------------------------
std::shared_ptr<audio_output_t> spotify_t::get_audio_output()
{
  return m_audio_output;
}

//
// Spotify callbacks.
//

// ----------------------------------------------------------------------------
void spotify_t::logged_in_cb(sp_session *session, sp_error error)
{
  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));
  self->m_command_queue.push(std::bind(&spotify_t::logged_in_handler, self));
}

// ----------------------------------------------------------------------------
void spotify_t::logged_out_cb(sp_session *session)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
inline void spotify_t::metadata_updated_cb(sp_session *session)
{
  //std::cout << "callback:  " << __FUNCTION__ << std::endl;
  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));

  if ( self->m_track )
  {
    sp_error err = sp_track_error(self->m_track);
    if (err == SP_ERROR_OK) {
      self->m_command_queue.push(std::bind(&spotify_t::track_loaded_handler, self));
    }
  }
}

// ----------------------------------------------------------------------------
void spotify_t::connection_error_cb(sp_session *session, sp_error error)
{
  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::message_to_user_cb(sp_session *session, const char* message)
{
  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::notify_main_thread_cb(sp_session *session)
{
  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));
  self->m_command_queue.push(std::bind(&spotify_t::process_events_handler, self));
}

// ----------------------------------------------------------------------------
int spotify_t::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames)
{
  //std::cout << "callback:  " << __FUNCTION__ << " num_frames=" << num_frames << std::endl;

  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));

  size_t num_bytes = num_frames * sizeof(int16_t) * format->channels;

  auto audio_output = self->get_audio_output(44100, format->channels);
  audio_output->write(frames, num_bytes);

  return num_frames;
}

// ----------------------------------------------------------------------------
void spotify_t::play_token_lost_cb(sp_session *session)
{
  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::log_message_cb(sp_session *session, const char* data)
{
  //std::cout << "callback:  " << __FUNCTION__ << " : " << data;
}

// ----------------------------------------------------------------------------
void spotify_t::end_of_track_cb(sp_session *session)
{
  //std::cout << "callback:  " << __FUNCTION__ << std::endl;
  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));
  self->m_command_queue.push(std::bind(&spotify_t::end_of_track_handler, self));
}

// ----------------------------------------------------------------------------
void spotify_t::stream_error_cb(sp_session *session, sp_error error)
{
  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::user_info_updated_cb(sp_session *session)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::start_playback_cb(sp_session *session)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::stop_playback_cb(sp_session *session)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::get_audio_buffer_stats_cb(sp_session *session, sp_audio_buffer_stats *stats)
{
  spotify_t* self = reinterpret_cast<spotify_t*>(sp_session_userdata(session));

  auto audio_output = self->get_audio_output();
  if ( audio_output.get() )
  {
    stats->samples = audio_output->queued_frames();
    stats->stutter = 0;
  }

  //std::cout << "callback:  " << __FUNCTION__ << " samples=" << stats->samples << ", stutter=" << stats->stutter << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::offline_status_updated_cb(sp_session *session)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::offline_error_cb(sp_session *session, sp_error error)
{
    std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
void spotify_t::credentials_blob_updated_cb(sp_session *session, const char* blob)
{
//  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

