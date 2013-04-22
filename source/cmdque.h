// ----------------------------------------------------------------------------
#ifndef __cmdque_h__
#define __cmdque_h__

// ----------------------------------------------------------------------------
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <iostream>

// ----------------------------------------------------------------------------
class cmdque_t
{
public:
  cmdque_t()
  {
  };
private:
  cmdque_t(const cmdque_t& other) = delete;
public:
  virtual ~cmdque_t()
  {
  };
public:
  void push(std::function<void()>&& command)
  {
    std::lock_guard<std::mutex> _(lock);
    q.push(std::move(command));
    rdy.notify_all();
  }
public:
  std::function<void()> pop(std::chrono::milliseconds wait_for, std::function<void()> timeout_cb=[]{})
  //std::function<void()> pop()
  {
    std::unique_lock<std::mutex> _(lock);

    if ( q.empty() )
    {
      rdy.wait_for(_, wait_for);
    }

    if ( q.empty() )
    {
      return timeout_cb;
    }
    else
    {
      std::function<void()> command = std::move(q.front());
      q.pop();
      return std::move(command);
    }
  }
private:
  std::mutex lock;
  std::condition_variable rdy;
  std::queue<std::function<void()>> q;
};

// ----------------------------------------------------------------------------
#endif // __cmdque_h__
