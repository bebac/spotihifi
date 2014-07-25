// ----------------------------------------------------------------------------
//
//        Filename:  audio_output_alsa.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __audio_output_alsa_h__
#define __audio_output_alsa_h__

// ----------------------------------------------------------------------------
#include <cmdque.h>
#include <log.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <memory>
#include <cstring>
#include <thread>
#include <atomic>
#include <string>

// ----------------------------------------------------------------------------
#include <alsa/asoundlib.h>

// ----------------------------------------------------------------------------
class audio_output_t
{
public:
  audio_output_t(const std::string& device_name)
    :
    m_running(true),
    m_command_queue(),
    m_handle(0),
    m_queued_frames(0),
    m_device_name(device_name),
    m_thr{&audio_output_t::main, this}
  {
  }
public:
  ~audio_output_t()
  {
    if ( m_running ) {
      stop();
    }
    m_thr.join();
  }
public:
  void write_s16_le_i(const void* frames, size_t num_frames)
  {
    size_t len = num_frames * sizeof(int16_t) * 2 /* channels */;
    std::shared_ptr<char> buffer(new char[len], std::default_delete<char[]>());

    std::memcpy(buffer.get(), frames, len);

    m_command_queue.push([=]{
      write_s16_le_i_handler(std::move(buffer), num_frames);
    });

    m_queued_frames += num_frames;
  }
public:
  void stop()
  {
    m_command_queue.push([this]() {
      this->m_running = false;
    });
  }
public:
  int queued_frames()
  {
    return m_queued_frames;
  }
private:
  void init()
  {
    int err;
    int open_retries = 0;

    while ( open_retries < 10 )
    {
      err = snd_pcm_open( &m_handle, m_device_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0 );
      if ( err < 0 ) {
        _log_(error) << "snd_pcm_open failed! " << snd_strerror(err);
        open_retries++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      else {
        _log_(debug) << "snd_pcm_open ok";
        break;
      }
    }

    err = snd_pcm_set_params(m_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 0, 500000);
    if ( err < 0 ) {
      _log_(error) << "snd_pcm_set_params failed! " << snd_strerror(err);
    }
  }
private:
  void write_s16_le_i_handler(std::shared_ptr<char> buf, size_t num_frames)
  {
    snd_pcm_sframes_t frames = snd_pcm_writei(m_handle, buf.get(), num_frames);

    if ( frames < 0 ) {
      _log_(warning) << "underrun";
      frames = snd_pcm_recover(m_handle, frames, 0);
    }

    if ( frames < 0 ) {
      _log_(error) << snd_strerror(frames);
    }
    else {
      m_queued_frames -= frames;
    }
  }
private:
  void main()
  {
    init();
    while ( m_running )
    {
      auto cmd = m_command_queue.pop(std::chrono::seconds(1));
      cmd();
    }

    if (m_handle) {
      _log_(info) << "closing pcm (m_handle=" << m_handle << ")";
      snd_pcm_close(m_handle);
    }
  }
private:
  bool             m_running;
  cmdque_t         m_command_queue;
  snd_pcm_t*       m_handle;
  std::atomic<int> m_queued_frames;
  std::string      m_device_name;
  std::thread      m_thr;
};

// ----------------------------------------------------------------------------
#endif // __audio_output_alsa_h__
