#include <stdexcept>
#include <algorithm>
#include <spdlog/spdlog.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "common/signal.h"
#include "PlotsFile.h"
#include "poc/gpu_plotter.h"

#define NONCES_VECTOR           16
#define NONCES_VECTOR_LOG2      4
#define MESSAGE_CAP             64
#define NUM_HASHES              8192
#define HASH_SIZE_WORDS         8
#define NONCE_SIZE_WORDS        HASH_SIZE_WORDS * NUM_HASHES
#define Address(nonce,hash,word) \
              ((nonce >> NONCES_VECTOR_LOG2) * NONCES_VECTOR * NONCE_SIZE_WORDS \
              + (hash) * NONCES_VECTOR * HASH_SIZE_WORDS \
              + word * NONCES_VECTOR + \
              (nonce & (NONCES_VECTOR-1)))

namespace plotsFile {

void plotfile::init(const std::string& p_path, int32_t local_nonces, int32_t nonces_per_file) throw (std::exception) {
  if (m_buffer == nullptr)
    m_buffer = new uint8_t[local_nonces * gpu_plotter::SCOOP_SIZE];
  if (!Cfile.is_open()) {
    auto create_new = ! util::file::exists(p_path);
    Cfile.open(p_path, create_new);
    Cfile.allocate((size_t)nonces_per_file * gpu_plotter::PLOT_SIZE);
  }
}

bool plotfile::writetodisk(uint8_t* buffer, int32_t local_nonces, uint64_t nonce_written, uint64_t total_nonces){
  for (int cur_scoop=0; !signal::get().stopped() && cur_scoop<4096; ++cur_scoop) {
    uint32_t* src = (uint32_t*)buffer;
    uint32_t* des = (uint32_t*)m_buffer;
    auto offset = (nonce_written + cur_scoop * total_nonces) * 64;
    Cfile.seek(offset);
    for (auto nstart=0; nstart<local_nonces; ++nstart,des+=16) {
      des[0x00] = src[Address(nstart, cur_scoop * 2, 0)];
      des[0x01] = src[Address(nstart, cur_scoop * 2, 1)];
      des[0x02] = src[Address(nstart, cur_scoop * 2, 2)];
      des[0x03] = src[Address(nstart, cur_scoop * 2, 3)];
      des[0x04] = src[Address(nstart, cur_scoop * 2, 4)];
      des[0x05] = src[Address(nstart, cur_scoop * 2, 5)];
      des[0x06] = src[Address(nstart, cur_scoop * 2, 6)];
      des[0x07] = src[Address(nstart, cur_scoop * 2, 7)];
      des[0x08] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 0)];
      des[0x09] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 1)];
      des[0x0A] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 2)];
      des[0x0B] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 3)];
      des[0x0C] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 4)];
      des[0x0D] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 5)];
      des[0x0E] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 6)];
      des[0x0F] = src[Address(nstart, 8192 - (cur_scoop * 2 + 1), 7)];
    }
    if (!Cfile.write(m_buffer, local_nonces * 64)) {
      spdlog::error("write {} failed with code {}.", Cfile.filename(), Cfile.last_error());
      return false;
    }
  }
  return true;
}
}
