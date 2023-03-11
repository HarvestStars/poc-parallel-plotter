#pragma once

#include <memory>
#include <functional>

#include "task.h"
#include "common/paged_block.h"

class worker;

struct hasher_task : public task {
  explicit hasher_task(uint64_t _pid, uint64_t _sn, int32_t _nonces, int32_t _nonces_written
                      ,std::shared_ptr<worker> _writer
                      ,int cur_wr_task
                      ,util::paged_block* _block)
    : pid(_pid), sn(_sn), nonces(_nonces)
    , writer(_writer)
    , current_write_task(cur_wr_task)
    , block(_block)
    , nonces_written(_nonces_written){}

  uint64_t pid{0};
  uint64_t sn{0};
  int32_t  nonces{0};
  int32_t  nonces_written{0};
  std::shared_ptr<worker> writer;
  int current_write_task{-1};
  util::paged_block* block;
  int mbps{0};
  int npm{0};
};