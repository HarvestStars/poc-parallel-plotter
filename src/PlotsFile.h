#ifndef CRYO_GPU_PLOT_GENERATOR_PLOTS_FILE_H
#define CRYO_GPU_PLOT_GENERATOR_PLOTS_FILE_H

#include <string>
#include <fstream>
#include <exception>

#include "common/file.h"

namespace plotsFile {

class plotfile {
private:
  util::file Cfile;
  uint8_t* m_buffer{nullptr};

public:
  plotfile() = default;
  ~plotfile() = default;
  void init(const std::string& p_path, int32_t local_nonces, int32_t nonces_per_file) throw (std::exception);
  bool writetodisk(uint8_t* buffer, int32_t local_nonces, uint64_t nonce_written, uint64_t total_nonces);
  void close(){ 
    Cfile.close(); 
    if (m_buffer) delete []m_buffer;
  };
};

}

#endif
