#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <spdlog/spdlog.h>

#include "worker.h"
#include "task_writer.h"
#include "task_hasher.h"
#include "common/signal.h"
#include "plotter.h"
#include "PlotsFile.h"

void writer_worker::run() {
  spdlog::info("thread writer worker [{}] starting.", driver_);
  auto bench_mode = ctx_.bench_mode();
  while (! signal::get().stopped()) {
    auto task = fin_hasher_tasks_.pop();
    if (!task)
      continue;
    if (task->current_write_task == -1 || task->current_write_task >= writer_tasks_.size())
      break;
    if (!task->block || !task->writer)
      break;

    // write plot
    auto& wr_task = writer_tasks_[task->current_write_task];
    auto& full_path = wr_task->plot_file();

    try{
      wr_task->Cplotfile.init(full_path, task->nonces, wr_task->total_nonces);
    }catch(std::exception& e){
      spdlog::info("error when init(preallocate) write task file [{}], Err: {}."
                   , full_path
                   , e.what());
      break;
    }
    util::timer timer;
    if ((bench_mode & 0x01) == 0){
      bool writtenOK = wr_task->Cplotfile.writetodisk(task->block->data(), task->nonces, task->nonces_written, wr_task->total_nonces);
      if ((task->nonces_written + task->nonces) >= wr_task->total_nonces){
        wr_task->Cplotfile.close();
      }
      if (!writtenOK){
        spdlog::info("error when writing worker [{}].", driver_);
        break;
      }
    }
    task->mbps = task->nonces * 1000ull * plotter_base::PLOT_SIZE / 1024 / 1024 / timer.elapsed();
    spdlog::debug("write nonce [{}][{}, {}) ({}) to `{}`"
                  , task->current_write_task
                  , task->sn
                  , task->sn+task->nonces
                  , plotter_base::btoh(task->block->data(), 32)
                  , full_path);
    ctx_.report(std::move(task));
  }
  spdlog::error("thread writer worker [{}] stopped.", driver_);
}