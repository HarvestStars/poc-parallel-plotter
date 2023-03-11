#pragma once

#include <sstream>
#include <boost/compute.hpp>

#include <spdlog/spdlog.h>

#include "plotter_base.h"

namespace compute = boost::compute;

struct gpu_plotter : public plotter_base {
  struct args_t {
    size_t lws;
    size_t gws;
    int step;
  };
  using args_t = struct args_t;

  gpu_plotter(gpu_plotter&) = delete;
  gpu_plotter(gpu_plotter&&) = delete;
  gpu_plotter& operator=(gpu_plotter&) = delete;
  gpu_plotter& operator=(gpu_plotter&&) = delete;

  gpu_plotter(compute::device& device, args_t args)
    : device_{device}
    , context_{device} {
    step_ = args.step ? args.step : 32;
    auto cus = device_.compute_units();
    auto dims = device.max_work_item_dimensions();
    local_work_size_ = args.lws;
    args.lws = args.lws ? args.lws : device_.max_work_group_size();
    local_work_size2_ = args.lws;
    global_work_size_ = args.gws ? args.gws : cus * args.lws;
    // TODO: check host free memory?
    auto max_nonces = size_t(device_.max_memory_alloc_size() / PLOT_SIZE);
    global_work_size_ = std::min(max_nonces, global_work_size_);
    global_work_size_  = global_work_size_ / args.lws * args.lws;
    assert(global_work_size_ >= 16);
    if (global_work_size_ < 16)
      global_work_size_ = 16;
  }

  bool init(const std::string& kernel, const std::string& name) {
    spdlog::debug("loading opencl kernel...");
    program_ = compute::program::create_with_source_file(kernel, context_);
    try {
      spdlog::debug("compiling openc kernel...");
      program_.build();
    } catch(...) {
      return false;
    }
    cqueue_ = compute::command_queue{context_, device_};
    dev_buff_.emplace_back(context_, global_work_size_ * PLOT_SIZE);
    dev_buff_.emplace_back(context_, global_work_size_ * PLOT_SIZE);
    kernel_ = compute::kernel{program_, name};
    return true;
  }

  void plot(uint64_t plot_id, uint64_t start_nonce, size_t nonces, uint8_t* host_buff) {
    nonces = std::min(nonces, global_work_size_);
    kernel_.set_arg(0, dev_buff_[0]);
    kernel_.set_arg(1, start_nonce);
    kernel_.set_arg(2, hton_ull(plot_id));
    kernel_.set_arg(5, nonces);

    try {
      int start, end;
      for (int i = 8192; i > 0; i -= step_) {
        start = i;
        end = i - step_ - !(i ^ step_);
        kernel_.set_arg(3, start);
        kernel_.set_arg(4, end);
        cqueue_.enqueue_1d_range_kernel(kernel_, 0, global_work_size_, local_work_size_);
      }
      cqueue_.finish();
      cqueue_.enqueue_read_buffer(dev_buff_[0], 0, global_work_size_*PLOT_SIZE, host_buff);
    } catch(compute::opencl_error& e) {
      spdlog::error("opencl error: [{}] {}", e.error_code(), e.error_string());
      throw e;
    }
  }

  std::string to_string(uint8_t* buff, size_t len) const { return btoh(buff, len); }

  const compute::device& device() const { return device_; }
  const compute::context& context() const { return context_; }
  const compute::program& program() const { return program_; }
  size_t global_work_size() const { return global_work_size_; }

  std::string info() {
    std::stringstream ss;
    ss << "[" <<device_.name() << "] " << device_.compute_units()<< "-"
       << global_work_size_ << "-"
       << local_work_size_ << "("
       << local_work_size2_ << ")";
    return ss.str();
  }

private:
  compute::device device_;
  compute::context context_;
  compute::program program_;
  compute::kernel kernel_;
  compute::command_queue cqueue_;
  std::vector<compute::buffer> dev_buff_;
  int working_dev_buff_{0};

  int step_{ 32 };
  size_t global_work_size_{0};
  size_t local_work_size_{0};
  size_t local_work_size2_{0};
};