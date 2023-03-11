#pragma once

#include "sph_shabal.h"

struct Shabal256 {
  Shabal256() {
    sph_shabal256_init(&context_);
  }

  void update(const uint8_t* data, size_t len) {
    sph_shabal256(&context_, data, len);
  }

  void digest(uint8_t* out) {
    sph_shabal256_close(&context_, out);
    sph_shabal256_init(&context_);
  }
  
  void close(uint8_t* out) {
    sph_shabal256_close(&context_, out);
  }

private:
  sph_shabal256_context context_;
};