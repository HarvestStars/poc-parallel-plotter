
#include <spdlog/spdlog.h>

#include "plotter.h"
#include "worker.h"
#include "task_hasher.h"
#include "worker_hasher.h"
#include "worker_writer.h"
#include "poc/gpu_plotter.h"
#include "common/queue.h"
#include "common/timer.h"
#include "common/signal.h"

void hasher_worker::run() {
  spdlog::info("thread hasher worker [{}] starting.", plotter_->info());
  auto bench_mode = ctx_.bench_mode();
  while (! signal::get().stopped()) {
    auto task = hasher_tasks_.pop();
    if (!task)
      continue;
    if (task->current_write_task == -1)
      break;
    if (!task->block || !task->writer)
      break;

    // calc plot
    if ((bench_mode & 0x02) == 0) {
      util::timer timer;
      plotter_->plot( task->pid
                    , task->sn
                    , task->nonces
                    , task->block->data()
                    );
      task->npm = task->nonces * 60ull * 1000 / timer.elapsed();
    }
    report(task);
  }

  spdlog::error("thread hasher worker [{}] stopped.", plotter_->info());
}

void hasher_worker::report(std::shared_ptr<hasher_task>& task) {
  auto writer = std::dynamic_pointer_cast<writer_worker>(task->writer);
  spdlog::debug("establised hash [{}][{} {}) {}", task->current_write_task
                                                , task->sn
                                                , task->sn + task->nonces
                                                , task->writer->info());
  writer->push_fin_hasher_task(std::move(task));
}