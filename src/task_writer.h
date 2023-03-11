#pragma once

#include <string>
#include <sstream>

#include "task.h"
#include "PlotsFile.h"

struct writer_task {
  explicit writer_task(uint64_t _pid, uint64_t _sn, int32_t _nonces, std::string _driver)
    : pid(_pid), sn(_sn), nonces(_nonces), prev_(_sn), start_nonce(_sn), total_nonces(_nonces) {
      std::stringstream ss;
      ss << _driver << pid << "_" << sn << "_" << nonces;
      pf_ = ss.str();
  }

  uint64_t pid;
  uint64_t sn;
  uint64_t start_nonce;
  int32_t  nonces;
  int32_t  total_nonces;
  plotsFile::plotfile Cplotfile;
  
  int32_t next(int32_t gws, int32_t& nonces_written) {
    if (nonces == 0)
      return -1;
    auto n = std::min(gws, nonces);
    nonces -= n;
    sn = prev_;
    prev_ += n;
    nonces_written = sn - start_nonce;
    return n;
  }

  std::string plot_file() { return pf_; }

private:
  uint64_t prev_{0};
  std::string pf_;
};
