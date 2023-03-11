#pragma once

#include <memory>

class worker : public std::enable_shared_from_this<worker> {
public:
  worker() = default;
  virtual ~worker() = default;
  worker(worker&) = delete;
  worker(worker&&) = delete;
  worker& operator=(worker&) = delete;
  worker& operator=(worker&&) = delete;

  virtual void run() = 0;
  virtual void stop() = 0;
  virtual std::string info(bool detail = false) = 0;
  virtual size_t task_queue_size() = 0;
};