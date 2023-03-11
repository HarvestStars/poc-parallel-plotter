#pragma once

#include <unordered_map>
#include <stdint.h>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "utils.h"

namespace util {

class paged_block {
public:
  static constexpr std::align_val_t page_size{4096};
  static constexpr size_t page_mask{((uint64_t)-1) ^ (4095)};

public:
  explicit paged_block() : data_(nullptr), len_(0) {}
  explicit paged_block(uint8_t* data_, size_t len) : data_(data_), len_(len) {}
  
  virtual ~paged_block() {
    data_ = nullptr;
    len_ = 0;
  }

  paged_block(paged_block& rh) {
    data_ = rh.data_;
    len_ = rh.len_;
  }

  paged_block& operator=(paged_block& rh) {
    data_ = rh.data_;
    len_ = rh.len_;
    return *this;
  }

  paged_block(paged_block&& rh) {
    data_ = rh.data_;
    len_ = rh.len_;
    rh.data_ = nullptr;
    rh.len_ = 0;
  }

  paged_block& operator=(paged_block&& rh) {
    data_ = rh.data_;
    len_ = rh.len_;
    rh.data_ = nullptr;
    rh.len_ = 0;
    return *this;
  }

  uint8_t* data() const { return data_; }
  size_t len() const { return len_; }
  operator bool() const { return data_ && len_; }

private:
  uint8_t* data_;
  size_t len_;
};

class block_allocator {
  block_allocator() = default;
  block_allocator(block_allocator&) = delete;
  block_allocator(block_allocator&&) = delete;
  block_allocator& operator=(block_allocator&) = delete;
  block_allocator& operator=(block_allocator&&) = delete;

public:
  block_allocator(size_t max_mem) : max_mem_{max_mem} {}

  ~block_allocator() {
    for (auto& block : free_blocks_)
      operator delete[](block.second.data(), paged_block::page_size);
    for (auto& block : used_blocks_)
      operator delete[](block.second.data(), paged_block::page_size);
    free_blocks_.swap(decltype(free_blocks_)());
    used_blocks_.swap(decltype(used_blocks_)());
    total_allocated_ = 0;
  }

  paged_block& allocate(size_t len) {
    auto it = free_blocks_.begin();
    for (; it != free_blocks_.end(); ++it) {
      if (it->second.len() == len) {
        break;
      }
    }

    try {
      if (it == free_blocks_.end()) {
        auto allocated_mem = total_allocated_ + len;
        if (allocated_mem > max_mem_ || sys_free_mem_bytes() < len)
          return dummy_block;
        auto nbytes = (uint8_t*) operator new[](len, paged_block::page_size);
        auto test = (uint64_t(nbytes) & 0xfff);
        assert(test == 0);
        auto nb = paged_block(nbytes, len);
        total_allocated_ = allocated_mem;
        auto key = nb.data();
        used_blocks_.emplace(key, std::move(nb));

        spdlog::debug("allocate new, free block: {}, used block: {}.",free_blocks_.size(), used_blocks_.size());
        return used_blocks_[key];
      } else {
        auto key = it->second.data();
        used_blocks_.emplace(key, std::move(it->second));
        free_blocks_.erase(it);
        spdlog::debug("allocate free, free block: {}, used block: {}.",free_blocks_.size(), used_blocks_.size());
        return used_blocks_[key];
      }
    } catch(std::exception& e) {
      spdlog::error("allocation exception: {}.", e.what());
      return dummy_block;
    }
  }

  void retain(paged_block& pb) {
    auto key = pb.data();
    free_blocks_.emplace(key, std::move(pb));
    used_blocks_.erase(key);
  }

  size_t size() {
    return free_blocks_.size() + used_blocks_.size();
  }

private:
  std::unordered_map<uint8_t*, paged_block> free_blocks_;
  std::unordered_map<uint8_t*, paged_block> used_blocks_;
  size_t total_allocated_{0};
  size_t max_mem_{0};
  paged_block dummy_block{nullptr, 0};
};

}