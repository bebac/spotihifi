// ----------------------------------------------------------------------------
#ifndef __audio_output_alsa_h__
#define __audio_output_alsa_h__

// ----------------------------------------------------------------------------
#include <cmdque.h>

// ----------------------------------------------------------------------------
#include <iostream>
#include <memory>
#include <cstring>
#include <thread>
#include <atomic>

// ----------------------------------------------------------------------------
#include <alsa/asoundlib.h>

// ----------------------------------------------------------------------------
class audio_buffer_t
{
public:
#if 0
  audio_buffer_t(size_t len)
  {
    m_buf = new unsigned char[len];
    m_len = len;
  }
#endif
public:
  audio_buffer_t(const void* buf, size_t len)
  {
    //std::cout << __FUNCTION__ << ": buf=" << reinterpret_cast<const void *>(buf) << ", len=" << len << std::endl;
    m_buf = new unsigned char[len];
    std::memcpy(m_buf, buf, len);
    m_len = len;
    //std::cout << __FUNCTION__ << ": m_buf=" << reinterpret_cast<void *>(m_buf) << ", m_len=" << m_len << std::endl;
  }
public:
#if 0
  audio_buffer_t(audio_buffer_t&& other)
  {
    std::cout << __FUNCTION__ << ": other.m_buf=" << reinterpret_cast<void *>(other.m_buf) << ", other.m_len=" << other.m_len << std::endl;
    m_buf = std::move(other.m_buf);
    m_len = std::move(other.m_len);
    other.m_buf = 0;
    other.m_len = 0;
    std::cout << __FUNCTION__ << ": m_buf=" << reinterpret_cast<void *>(m_buf) << ", m_len=" << m_len << std::endl;
  }
#endif
public:
  ~audio_buffer_t()
  {
    //std::cout << __FUNCTION__ << ": m_buf=" << reinterpret_cast<void *>(m_buf) << ", m_len=" << m_len << std::endl;
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
    audio_output_t()
      :
      m_running(true),
      m_command_queue(),
      m_thr{&audio_output_t::main, this},
      m_handle(0),
      m_queued_frames(0)
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
    err = snd_pcm_open( &m_handle, "default", SND_PCM_STREAM_PLAYBACK, 0 );
    if ( err < 0 ) {
      //throw std::runtime_error(snd_strerror(err));
      std::cout << "snd_pcm_open failed! " << snd_strerror(err) << std::endl;
    }
    else {
      std::cout << "snd_pcm_open ok" << std::endl;
    }

    err = snd_pcm_set_params(m_handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, 44100, 0, 500000);
    if ( err < 0 ) {
      //throw std::runtime_error(snd_strerror(err));
      std::cout << "snd_pcm_set_params failed! " << snd_strerror(err) << std::endl;
    }
  }
private:
  void write_handler(std::shared_ptr<audio_buffer_t> buffer)
  {
      //std::cout << "write" << std::endl;
      snd_pcm_sframes_t frames = snd_pcm_writei(m_handle, buffer->data(), buffer->len()/4);

      if ( frames < 0 ) {
        std::cout << "underrun" << std::endl;
        frames = snd_pcm_recover(m_handle, frames, 0);
      }

      if ( frames < 0 ) {
        //throw std::runtime_error(snd_strerror(written));
        std::cout << snd_strerror(frames) << std::endl;
      }
      else {
        m_queued_frames -= frames;
      }

      //std::cout << "wrote " << frames << " frames" << std::endl;
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
      snd_pcm_close(m_handle);
    }
  }
private:
  bool             m_running;
  cmdque_t         m_command_queue;
  std::thread      m_thr;
  snd_pcm_t*       m_handle;
  std::atomic<int> m_queued_frames;
};

// ----------------------------------------------------------------------------
#endif // __audio_output_alsa_h__
