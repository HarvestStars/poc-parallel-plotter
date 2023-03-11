#pragma once

#include <vector>
#include <memory>
#include <regex>
#include <chrono>
#include <boost/compute.hpp>

#include <OptionParser.h>
#include <spdlog/spdlog.h>

#include "common/paged_block.h"
#include "common/queue.h"
#include "common/timer.h"
#include "common/signal.h"
#include "common/utils.h"
#include "poc/cpu_plotter.h"
#include "poc/gpu_plotter.h"
#include "task_hasher.h"
#include "task_writer.h"
#include "worker_hasher.h"
#include "worker_writer.h"
#include "report.h"
#include "PlotsFile.h"

namespace compute = boost::compute;
namespace plotsfile = plotsFile;

class plotter {
  plotter() = delete;
  plotter(gpu_plotter&) = delete;
  plotter(gpu_plotter&&) = delete;
  plotter& operator=(gpu_plotter&) = delete;
  plotter& operator=(gpu_plotter&&) = delete;

public:
  explicit plotter(optparse::Values& args);

  void run();

  void report(std::shared_ptr<hasher_task>&& task);
  
  int bench_mode() { return (int)args_.get("bench"); }

private:
  void run_test();

  void run_plotter();

  void run_disk_bench();

private:
  optparse::Values& args_;
  std::vector<std::shared_ptr<worker>> workers_;
  util::queue<std::shared_ptr<hasher_task>> reporter_;
};
