// ----------------------------------------------------------------------------
//
//        Filename:  spotify.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#include <spotify.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <random>
#include <algorithm>

// ----------------------------------------------------------------------------
static std::string sp_track_id(sp_track* track);
static std::shared_ptr<track_t> make_track_from_sp_track(sp_track* const track);

// ----------------------------------------------------------------------------
spotify_t::spotify_t(const std::string& audio_device_name,
                     const std::string& cache_dir,
                     const std::string& last_fm_username,
                     const std::string& last_fm_password)
  :
  m_session(0),
  m_session_logged_in(false),
  m_running(true),
  m_session_next_timeout(0),
  m_track(0),
  m_playlistcontainer(0),
  m_track_playing(false),
  m_audio_device_name(audio_device_name),
  m_audio_output(),
  m_cache_dir(cache_dir),
  m_last_fm_username(last_fm_username),
  m_last_fm_password(last_fm_password),
  /////
  // Tracks database.
  m_tracks(),
  m_tracks_initialized(false), // Not used yet.
  m_tracks_incarnation(reinterpret_cast<long long>(this)),
  m_tracks_transaction(0), // Always zero for now.
  /////
  m_continued_playback(true),
  m_thr{&spotify_t::main, this}
{
  LOG(DEBUG) << "constructed spotify instance " << this;
}

// ----------------------------------------------------------------------------
spotify_t::~spotify_t()
{
  LOG(DEBUG) << "begin destruction of spotify instance " << this;
  if ( m_running ) {
    stop();
  }
  m_thr.join();
  LOG(DEBUG) << "end destruction of spotify instance " << this;
}

// ----------------------------------------------------------------------------
void spotify_t::stop()
{
  m_command_queue.push([this]()
  {
    if ( m_track_playing )
    {
      player_state_notify("stopped");
      sp_session_player_unload(m_session);
      m_track_playing = false;
    }

    this->m_running = false;
  });
}

// ----------------------------------------------------------------------------
void spotify_t::login(const std::string& username, const std::string& password)
{
  m_command_queue.push([=]() {
    sp_session_login(m_session, username.c_str(), password.c_str(), 0, 0);
  });
}

// ----------------------------------------------------------------------------
void spotify_t::player_play()
{
  m_command_queue.push([=]() {
    if ( m_session_logged_in && m_track )
    {
      // TODO: Include track.
      player_state_notify("playing");
      sp_session_player_play(m_session, 1);
    }
    else {
      play_next_from_queue();
    }
  });
}

// ----------------------------------------------------------------------------
void spotify_t::player_play(const std::string& uri)
{
  m_command_queue.push([=]() {
    m_play_queue.push_back(uri);
    if ( m_session_logged_in && !m_track ) {
      play_next_from_queue();
    }
  });
}

// ----------------------------------------------------------------------------
void spotify_t::player_pause()
{
  m_command_queue.push([=]() {
    if ( m_session_logged_in && m_track )
    {
      player_state_notify("paused");
      sp_session_player_play(m_session, 0);
    }
  });
}

// ----------------------------------------------------------------------------
void spotify_t::player_skip()
{
  m_command_queue.push([=]()
  {
    if ( m_track_playing )
    {
      // TODO: Include track.
      player_state_notify("skip");
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
}

// ----------------------------------------------------------------------------
void spotify_t::player_stop()
{
  m_command_queue.push([=]()
  {
    if ( m_track_playing )
    {
      player_state_notify("stopped");
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
}

// ----------------------------------------------------------------------------
void spotify_t::build_track_set_all()
{
  m_command_queue.push([=]()
  {
    LOG(INFO) << "build_track_set_all";
    m_continued_playback_tracks.clear();
    for ( auto& t : m_tracks )
    {
      auto id = t.second->track_id();
      m_continued_playback_tracks.push_back(id);
    }
    srand(time(0));
  });
}

// ----------------------------------------------------------------------------
void spotify_t::build_track_set_from_playlist(std::string playlist)
{
  m_command_queue.push([=]()
  {
    LOG(INFO) << "build_track_set_from_playlist playlist=" << playlist;
    m_continued_playback_tracks.clear();
    for ( auto& t : m_tracks )
    {
      auto pl_set = t.second->playlists();
      if ( pl_set.find(playlist) != pl_set.end() )
      {
        auto id = t.second->track_id();
        m_continued_playback_tracks.push_back(id);
      }
    }
    srand(time(0));
  });
}

// ----------------------------------------------------------------------------
std::future<json::object> spotify_t::get_tracks(long long incarnation, long long transaction)
{
  auto promise = std::make_shared<std::promise<json::object>>();
  m_command_queue.push([=]()
  {
    LOG(INFO) << "get_tracks"
              << " m_tracks_incarnation=" << m_tracks_incarnation
              << ", incarnation=" << incarnation
              << ", m_tracks_transaction=" << m_tracks_transaction
              << ", transaction=" << transaction;

    json::object result{
      { "incarnation", std::to_string(m_tracks_incarnation) },
      { "transaction", std::to_string(m_tracks_transaction) }
    };

    if ( incarnation != m_tracks_incarnation )
    {
      // If incarnation has changed send back the complete track list.
      json::array tracks;
      for ( auto& t : m_tracks ) {
        tracks.push_back(to_json(*t.second));
      }
      result["tracks"] = tracks;
    }
    else
    {
      // TODO: Handle transaction count to only send back what has been updated.
    }
    promise->set_value(result);
  });
  return promise->get_future();
}

// ----------------------------------------------------------------------------
void spotify_t::observer_attach(std::shared_ptr<player_observer_t> observer)
{
  m_command_queue.push([=]()
  {
    observers.push_back(observer);
    LOG(INFO) << "attached observer " << observer << " (" << observers.size() << ")";
  });
}

// ----------------------------------------------------------------------------
void spotify_t::observer_detach(std::shared_ptr<player_observer_t> observer)
{
  m_command_queue.push([=]()
  {
    auto it = std::find(observers.begin(), observers.end(), observer);
    observers.erase(it);
    LOG(INFO) << "detached observer " << observer << " (" << observers.size() << ")";
  });
}

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
  config.cache_location = m_cache_dir.c_str();
  config.settings_location = m_cache_dir.c_str();
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
      auto cmd = m_command_queue.pop(std::chrono::milliseconds(500), [this]
      {
        //std::cout << "timeout" << std::endl;

        // Check if there are tracke to be added and/or removed in the
        // tracks to add/remove queues.
        process_tracks_to_remove();
        process_tracks_to_add();

        // TODO: Check somehow if playlist import is done.
        if ( m_session_logged_in &&
             m_track &&
             m_continued_playback &&
             m_continued_playback_tracks.size() == 0 )
        {
          LOG(INFO) << "building continued playback track set";

          for ( auto& t : m_tracks )
          {
            auto id = t.second->track_id();
            m_continued_playback_tracks.push_back(id);
          }
          srand(time(0));
        }
      });
      cmd();
    }

    LOG(INFO) << "spotify_t::" << __FUNCTION__ << " releasing session " << m_session;
    sp_session_release(m_session);
  }
  catch(std::exception& e)
  {
    LOG(FATAL) << "spotify_t::" << __FUNCTION__ << " error=" << e.what();
    LOG(INFO) << "spotify_t::" << __FUNCTION__ << " releasing session " << m_session;
    sp_session_release(m_session);
  }
  catch(...)
  {
    LOG(FATAL) << "spotify_t::" << __FUNCTION__ << " unknown error";
    LOG(INFO) << "spotify_t::" << __FUNCTION__ << " releasing session " << m_session;
    sp_session_release(m_session);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::logged_in_handler()
{
  LOG(DEBUG) << "spotify_t::" << __FUNCTION__;

  m_session_logged_in = true;

  // Start loading the starred playlist.
  sp_playlist* pl = sp_session_starred_create(m_session);
  sp_playlist_add_ref(pl);

  // Import starred tracks.
  import_playlist(pl);

  // Load playlist container.
  sp_playlistcontainer* m_playlistcontainer = sp_session_playlistcontainer(m_session);
  sp_playlistcontainer_add_ref(m_playlistcontainer);

  if ( sp_playlistcontainer_is_loaded(m_playlistcontainer) )
  {
    container_loaded_cb(m_playlistcontainer, this);

    sp_playlistcontainer_callbacks container_callbacks = {
      &playlist_added_cb,
      0,
      0,
      0,
    };

    sp_playlistcontainer_add_callbacks(m_playlistcontainer, &container_callbacks, this);
  }
  else
  {
    sp_playlistcontainer_callbacks container_callbacks = {
      &playlist_added_cb,
      0,
      0,
      &container_loaded_cb,
    };

    sp_playlistcontainer_add_callbacks(m_playlistcontainer, &container_callbacks, this);
  }

  if ( m_last_fm_username.length() > 0 )
  {
    LOG(INFO) << "scrobbling to last.fm";
    sp_session_set_social_credentials(m_session, SP_SOCIAL_PROVIDER_LASTFM, m_last_fm_username.c_str(), m_last_fm_password.c_str());
    sp_session_set_scrobbling(m_session, SP_SOCIAL_PROVIDER_LASTFM, SP_SCROBBLING_STATE_LOCAL_ENABLED);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::track_loaded_handler()
{
  LOG(INFO) << "spotify_t::" << __FUNCTION__;
  if ( !m_track_playing )
  {
    sp_artist* artist;
    sp_album* album;

    sp_artist_add_ref(artist = sp_track_artist(m_track, 0));
    sp_album_add_ref(album = sp_track_album(m_track));

    sp_error err;

    if ( (err=sp_session_player_load(m_session, m_track)) != SP_ERROR_OK ) {
      LOG(ERROR) << "sp_session_player_load error " << err;
    }

    if ( (err=sp_session_player_play(m_session, 1)) != SP_ERROR_OK ) {
      LOG(ERROR) << "sp_session_player_play error " << err;
    }

    LOG(INFO) << "Start playing " << sp_track_name(m_track) << " - " << sp_artist_name(artist) << " - " << sp_album_name(album);

    auto it = m_tracks.find(sp_track_id(m_track));

    if ( it != end(m_tracks) ) {
      player_state_notify("playing", (*it).second);
    }
    else {
      LOG(ERROR) << "track id " << sp_track_id(m_track) << " not found in database!";
    }

    m_track_playing = true;

    sp_artist_release(artist);
    sp_album_release(album);
  }
}

// ----------------------------------------------------------------------------
void spotify_t::end_of_track_handler()
{
  LOG(INFO) << "spotify_t::" << __FUNCTION__;

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

    play_track(uri);

    m_play_queue.pop_front();
  }
  else if ( m_continued_playback && m_continued_playback_tracks.size() > 0 )
  {
    std::string uri("spotify:track:");

    // Pick a track from the track set.
    size_t index = rand() % (m_continued_playback_tracks.size());

    uri += m_continued_playback_tracks[index];

    LOG(INFO) << "continued playback uri " << uri;

    play_track(uri);
  }
  else
  {
    player_state_notify("stopped");
    m_audio_output.reset();
  }
}

// ----------------------------------------------------------------------------
void spotify_t::play_track(const std::string& uri)
{
  sp_link* link = sp_link_create_from_string(uri.c_str());
  if ( link )
  {
    if ( sp_link_type(link) == SP_LINKTYPE_TRACK )
    {
      sp_track_add_ref(m_track = sp_link_as_track(link));

      assert(m_track);

      sp_error err = sp_track_error(m_track);
      if (err == SP_ERROR_OK) {
        track_loaded_handler();
      }
    }
    else {
      LOG(ERROR) << "'" << uri.c_str() << "' is not a track";
      play_next_from_queue();
    }

    sp_link_release(link);
  }
  else {
    LOG(ERROR) << "failed to create link from '" << uri.c_str() << "'";
    play_next_from_queue();
  }
}

// ----------------------------------------------------------------------------
void spotify_t::import_playlist(sp_playlist* sp_pl_ptr)
{
  m_command_queue.push([=]()
  {
    if ( ! sp_playlist_is_loaded(sp_pl_ptr) ) {
      //LOG(DEBUG) << "playlist not loaded!";
      import_playlist(sp_pl_ptr);
      return;
    }

    size_t num_tracks = sp_playlist_num_tracks(sp_pl_ptr);

    std::string pl_name = sp_playlist_name(sp_pl_ptr);

    if ( pl_name.length() == 0 ) {
      pl_name = "Starred";
    }

    for (size_t i = 0; i < num_tracks; i++ )
    {
      sp_track* track = sp_playlist_track(sp_pl_ptr, i);
      if ( ! sp_track_is_loaded(track) )
      {
        //LOG(DEBUG) << "waiting for tracks to load for playlist " << pl_name;
        import_playlist(sp_pl_ptr);
        return;
      }
    }

    auto pl = playlist_t{num_tracks};

    for (size_t i = 0; i < num_tracks; i++ )
    {
      sp_track* sp_t = sp_playlist_track(sp_pl_ptr, i);

      auto track = make_track_from_sp_track(sp_t);

      sp_availability avail = sp_track_get_availability(m_session, sp_t);
      if ( avail == SP_TRACK_AVAILABILITY_AVAILABLE )
      {
        auto it = m_tracks.find(track->track_id());

        if ( it != end(m_tracks) )
        {
          track->playlists((*it).second->playlists());
        }
        track->playlists_add(pl_name);

        m_tracks[track->track_id()] = pl[i] = track;
      }
      else {
        LOG(WARNING) << "track unavailable " << to_json(*track);
      }
    }

    m_playlists[pl_name] = std::move(pl);

    LOG(INFO) << "imported playlist " << pl_name << ", m_tracks.size=" << m_tracks.size();

    set_playlist_callbacks(sp_pl_ptr);

    return;
  });
}

// ----------------------------------------------------------------------------
void spotify_t::process_tracks_to_add()
{
  if ( m_tracks_to_add.size() > 0 )
  {
    auto  data = m_tracks_to_add.front();
    auto& pl   = m_playlists[data.playlist_name];

    //LOG(INFO) << "### BEFORE ADD " << data.playlist_name;
    //for ( auto& track : pl )
    //{
    //  LOG(INFO) << to_json(*track);
    //}

    for ( auto& track : data.tracks )
    {
      auto it = m_tracks.find(track->track_id());
      if ( it != end(m_tracks) ) {
        track->playlists((*it).second->playlists());
      }

      track->playlists_add(data.playlist_name);

      LOG(INFO) << "added track " << to_json(*track) << " to playlist '" << data.playlist_name << "'";

      m_tracks[track->track_id()] = track;
    }

    pl.insert(pl.begin()+data.position, data.tracks.begin(), data.tracks.end());

    m_tracks_to_add.pop();

    //LOG(INFO) << "### AFTER ADD " << data.playlist_name;
    //for ( auto& track : pl )
    //{
    //  LOG(INFO) << to_json(*track);
    //}
  }
}

// ----------------------------------------------------------------------------
void spotify_t::process_tracks_to_remove()
{
  if ( m_tracks_to_remove.size() > 0 )
  {
    auto  data = m_tracks_to_remove.front();
    auto& pl   = m_playlists[data.playlist_name];

    //LOG(INFO) << "### BEFORE REMOVE " << data.playlist_name;
    //for ( auto& track : pl )
    //{
    //  LOG(INFO) << to_json(*track);
    //}

    for ( size_t pos : data.positions )
    {
      auto& track = pl[pos];

      track->playlists_remove(data.playlist_name);

      LOG(INFO) << "removed track " << to_json(*track) << " from playlist '" << data.playlist_name << "'";

      pl.erase(pl.begin()+pos);

      // TODO: Remove from track map if playlist set is empty?
    }

    m_tracks_to_remove.pop();

    //LOG(INFO) << "### AFTER REMOVE " << data.playlist_name;
    //for ( auto& track : pl )
    //{
    //  LOG(INFO) << to_json(*track);
    //}
  }
}

// ----------------------------------------------------------------------------
std::shared_ptr<audio_output_t> spotify_t::get_audio_output(int rate, int channels)
{
  assert(rate == 44100);
  assert(channels == 2);

  if ( ! m_audio_output.get() ) {
      m_audio_output = std::make_shared<audio_output_t>(m_audio_device_name);
  }

  return m_audio_output;
}

// ----------------------------------------------------------------------------
std::shared_ptr<audio_output_t> spotify_t::get_audio_output()
{
  return m_audio_output;
}

// ----------------------------------------------------------------------------
void spotify_t::player_state_notify(std::string state, std::shared_ptr<track_t> track)
{
  for ( auto& observer : observers )
  {
    if ( observer.get() )
    {
      json::object event{ { "state", state } };

      if ( track ) {
        event["track"] = to_json(*track);
      }

      observer->player_state_event(std::move(event));
    }
    else {
      LOG(ERROR) << "observer is null";
    }
  }
}

// ----------------------------------------------------------------------------
void spotify_t::set_playlist_callbacks(sp_playlist* pl)
{
  sp_playlist_callbacks playlist_callbacks = {
    &playlist_tracks_added_cb,
    &playlist_tracks_removed_cb,
    0,
    0,
    0, //&playlist_state_changed_cb,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  };

  sp_playlist_add_callbacks(pl, &playlist_callbacks, this);
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
    LOG(INFO) << "callback:  " << __FUNCTION__;
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
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::message_to_user_cb(sp_session *session, const char* message)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
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

  //
  // NOTE: Since music_delivery is called from another thread synchronization is
  //       important here. We could just allocate an audio_buffer here and send
  //       to the spotify main thread, but it seems better to just send it straight
  //       to the audio output thread.
  //
  //       I have seen an issue where we get a music_delivery when trying to stop
  //       and are stopping the audio output thread. A call to get_audio_output
  //       will then try start a new audio output thread. To avoid this we check
  //       if m_track_playing is true before writing the audio output.
  //

  if ( self->m_track_playing )
  {
    size_t num_bytes = num_frames * sizeof(int16_t) * format->channels;

    auto audio_output = self->get_audio_output(44100, format->channels);
    audio_output->write(frames, num_bytes);
  }
  else {
    LOG(WARNING) << "callback:  " << __FUNCTION__ << " while not playing";
  }

  return num_frames;
}

// ----------------------------------------------------------------------------
void spotify_t::play_token_lost_cb(sp_session *session)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::log_message_cb(sp_session *session, const char* data)
{
  LOG(INFO) << "spotify : " << data;
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
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::user_info_updated_cb(sp_session *session)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::start_playback_cb(sp_session *session)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::stop_playback_cb(sp_session *session)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
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
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::offline_error_cb(sp_session *session, sp_error error)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;
}

// ----------------------------------------------------------------------------
void spotify_t::credentials_blob_updated_cb(sp_session *session, const char* blob)
{
//  std::cout << "callback:  " << __FUNCTION__ << std::endl;
}

// ----------------------------------------------------------------------------
// NOTE: Not used.
void spotify_t::playlist_state_changed_cb(sp_playlist* pl, void* userdata)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;

#if 0
  spotify_t* self = reinterpret_cast<spotify_t*>(userdata);

  if ( sp_playlist_is_loaded(pl) )
  {
    LOG(INFO) << "playlist loaded " << sp_playlist_name(pl);
    self->set_playlist_callbacks(pl);
  }
#endif
}

// ----------------------------------------------------------------------------
void spotify_t::playlist_tracks_added_cb(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;

  spotify_t* self = reinterpret_cast<spotify_t*>(userdata);

  std::string pl_name = sp_playlist_name(pl);

  if ( pl_name.length() == 0 ) {
    pl_name = "Starred";
  }

  playlist_add_data data{pl_name, std::vector<std::shared_ptr<track_t>>{size_t(num_tracks)}, size_t(position)};

  for ( int i=0; i<num_tracks; ++i )
  {
    data.tracks[i] = make_track_from_sp_track(tracks[i]);
  }

  self->m_command_queue.push([=]()
    {
      LOG(INFO) << "queuing tracks to be added";
      self->m_tracks_to_add.push(data);
    });
}

// ----------------------------------------------------------------------------
void spotify_t::playlist_tracks_removed_cb(sp_playlist *pl, const int *tracks, int num_tracks, void *userdata)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;

  spotify_t* self = reinterpret_cast<spotify_t*>(userdata);

  std::string pl_name = sp_playlist_name(pl);

  if ( pl_name.length() == 0 ) {
    pl_name = "Starred";
  }

  playlist_remove_data data{pl_name, std::vector<size_t>{size_t(num_tracks)}};

  for ( int i=0; i<num_tracks; i++ )
  {
    data.positions[i] = tracks[i];
  }

  self->m_command_queue.push([=]()
  {
    LOG(INFO) << "queuing tracks to be removed";
    self->m_tracks_to_remove.push(data);
  });

#if 0
  for ( int i=0; i<num_tracks; ++i )
  {
    sp_track* const sp_track_ptr = sp_playlist_track(pl, tracks[i]);

    if ( sp_track_ptr )
    {
#if 0
//
// Argh! The tracks are already removed from the playlist which means that there is
// not way to map the index to a track pointer. I do not see how to implement remove
// without keeping a one-to-one mapping for all playlists.
//
      std::string pl_name = sp_playlist_name(pl);

      if ( pl_name.length() == 0 ) {
        pl_name = "Starred";
      }

      self->m_command_queue.push([=]()
        {
          LOG(INFO) << "queuing track " << static_cast<void *>(sp_track_ptr) << " to be removed from " << pl_name;
          self->m_tracks_to_remove.push(playlist_update_data{sp_track_ptr, pl_name});
        });
#endif
    }
    else {
      LOG(WARNING) << "index " << tracks[i] << " sp_track_ptr == 0";
    }
  }
#endif
}

// ----------------------------------------------------------------------------
void spotify_t::playlist_added_cb(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;

  spotify_t* self = reinterpret_cast<spotify_t*>(userdata);

  self->import_playlist(playlist);

#if 0
  self->m_command_queue.push([=]()
    {
      LOG(INFO) << "queuing playlist to be imported";
      self->m_playlists_for_import.push(playlist);
    });
#endif
}

// ----------------------------------------------------------------------------
void spotify_t::container_loaded_cb(sp_playlistcontainer *pc, void *userdata)
{
  LOG(INFO) << "callback:  " << __FUNCTION__;

  spotify_t* self = reinterpret_cast<spotify_t*>(userdata);

  int num = sp_playlistcontainer_num_playlists(pc);

  LOG(INFO) << "playlist container has " << num << " playlists";

  for ( int i=0; i<num; i++ )
  {
    sp_playlist* pl = sp_playlistcontainer_playlist(pc, i);

    self->import_playlist(pl);
  }
}

// ----------------------------------------------------------------------------
static std::string sp_track_id(sp_track* track)
{
  char buf[128];

  sp_link* link = sp_link_create_from_track(track, 0);
  int l = sp_link_as_string(link, buf, sizeof(buf));
  sp_link_release(link);

  if ( static_cast<size_t>(l) >= sizeof(buf) ) {
    LOG(ERROR) << "link string truncated";
  }

  std::string result(buf);

  // We want the id without 'spotify:track:'
  result = result.substr(result.rfind(':')+1);

  return std::move(result);
}

// ----------------------------------------------------------------------------
static std::shared_ptr<track_t> make_track_from_sp_track(sp_track* const sp_track_ptr)
{
  assert(sp_track_is_loaded(sp_track_ptr));

  sp_artist* artist;
  sp_album* album;

  sp_artist_add_ref(artist = sp_track_artist(sp_track_ptr, 0));
  sp_album_add_ref(album = sp_track_album(sp_track_ptr));

  auto track = std::make_shared<track_t>(track_t{});

  track->track_id(sp_track_id(sp_track_ptr));
  track->title(sp_track_name(sp_track_ptr));
  track->track_number(sp_track_index(sp_track_ptr));
  track->duration(sp_track_duration(sp_track_ptr));
  track->artist(sp_artist_name(artist));
  track->album(sp_album_name(album));

  sp_artist_release(artist);
  sp_album_release(album);

  //return std::move(track);
  return track;
}
