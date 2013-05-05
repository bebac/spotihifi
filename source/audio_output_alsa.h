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
class audio_buffer_t
{
public:
  audio_buffer_t(const void* buf, size_t len)
  {
    m_buf = new unsigned char[len];
    std::memcpy(m_buf, buf, len);
    m_len = len;
  }
public:
  ~audio_buffer_t()
  {
    delete [] m_buf;
  }
private:
  audio_buffer_t(const audio_buffer_t& other);
  void operator=(const audio_buffer_t& other);
public:
  void* data() { return m_buf; }
  const void* data() const { return m_buf; }
  size_t len() const { return m_len; }
private:
  unsigned char* m_buf;
  size_t         m_len;
};

// ----------------------------------------------------------------------------
class audio_output_t
{
public:
  audio_output_t(std::string device_name)
    :
    m_running(true),
    m_command_queue(),
    m_thr{&audio_output_t::main, this},
    m_handle(0),
    m_queued_frames(0),
    m_device_name(std::move(device_name))
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
  void write(const void* buffer, size_t len)
  {
    m_queued_frames += len/4;
    m_command_queue.push(std::bind(&audio_output_t::write_handler, this, std::make_shared<audio_buffer_t>(buffer, len)));
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

    //err = snd_pcm_open( &m_handle, "plughw:0,0", SND_PCM_STREAM_PLAYBACK, 0 );
    err = snd_pcm_open( &m_handle, m_device_name.c_str(), SND_PCM_STREAM_PLAYBACK, 0 );
    if ( err < 0 ) {
      LOG(ERROR) << "snd_pcm_open failed! " << snd_strerror(err);
    }
    else {
      LOG(DEBUG) << "snd_pcm_open ok";
    }

    err = snd_pcm_set_params(m_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 0, 500000);
    if ( err < 0 ) {
      LOG(ERROR) << "snd_pcm_set_params failed! " << snd_strerror(err);
    }
  }
private:
  void write_handler(std::shared_ptr<audio_buffer_t> buffer)
  {
    snd_pcm_sframes_t frames = snd_pcm_writei(m_handle, buffer->data(), buffer->len()/4);

    if ( frames < 0 ) {
      LOG(WARNING) << "underrun";
      frames = snd_pcm_recover(m_handle, frames, 0);
    }

    if ( frames < 0 ) {
      LOG(ERROR) << snd_strerror(frames);
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
      LOG(INFO) << "closing pcm (m_handle=" << m_handle << ")";
      snd_pcm_close(m_handle);
    }
  }
private:
  bool             m_running;
  cmdque_t         m_command_queue;
  std::thread      m_thr;
  snd_pcm_t*       m_handle;
  std::atomic<int> m_queued_frames;
  std::string      m_device_name;
};

// ----------------------------------------------------------------------------
#endif // __audio_output_alsa_h__
