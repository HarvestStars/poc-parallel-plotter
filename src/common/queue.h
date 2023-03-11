#pragma once

#include <mutex>
#include <deque>
#include <atomic>
#include <type_traits>

namespace util {

template<typename T>
class queue {
public:
  queue() = default;
  queue(queue&) = delete;
  queue(queue&&) = delete;
  queue& operator=(queue&) = delete;
  queue& operator=(queue&&) = delete;

  virtual ~queue() {
    std::deque<T> q;
    exit_.store(true);
    size_.store(0);
    {
      std::unique_lock<std::mutex> lock(mux_);
      q_.swap(q);
    }
    cond_.notify_all();
  }

  void push(T& v) {
    {
      std::unique_lock<std::mutex> lock(mux_);
      q_.push_back(v);
    }
    size_.fetch_add(1);
    cond_.notify_all();
  }

  void push(T&& v) {
    {
      std::unique_lock<std::mutex> lock(mux_);
      q_.push_back(std::move(v));
    }
    size_.fetch_add(1);
    cond_.notify_all();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mux_);
    while(!exit_) {
      if (q_.empty()) {
        cond_.wait(lock, [this](){
          return !q_.empty() || exit_;
        });
        continue;
      }
      auto v = q_.front();
      q_.pop_front();
      size_.fetch_sub(1);
      return v;
    }
    return T();
  }

  T pop_for(std::chrono::milliseconds ms) {
    std::unique_lock<std::mutex> lock(mux_);
    while(!exit_) {
      if (q_.empty()) {
        if(! cond_.wait_for(lock, ms, [this](){
            return !q_.empty() || exit_;
          }))
          return T();
        continue;
      }
      auto v = q_.front();
      q_.pop_front();
      size_.fetch_sub(1);
      return v;
    }
    return T();
  }

  size_t size() { return size_.load(); }

  void stop() {
    exit_.store(true);
    cond_.notify_all();
  }
  
private:
  std::mutex mux_;
  std::condition_variable cond_;
  std::deque<T> q_;
  std::atomic<size_t> size_{0};
  std::atomic<bool> exit_{false};
};

}