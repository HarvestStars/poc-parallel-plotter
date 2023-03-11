#pragma once

#include <spdlog/spdlog.h>

#ifdef WIN32
#include <Windows.h>

namespace util {

class file
{
public:
  static bool exists(const std::string filepath) {
    DWORD attr = ::GetFileAttributesA(filepath.c_str());
    return (INVALID_FILE_ATTRIBUTES != attr) && ((FILE_ATTRIBUTE_DIRECTORY & attr) == 0);
  }

public:
  file() = default;
  ~file() { close(); }
  file(file&) = delete;
  file(file&&) = delete;
  file& operator=(file&) = delete;
  file& operator=(file&&) = delete;

  bool is_open() {return handle_ != INVALID_HANDLE_VALUE; }

  bool open(const std::string filepath, bool create=true, bool nobuf=false) {
    if (handle_ != INVALID_HANDLE_VALUE)
      return true;

    DWORD access_flag = GENERIC_READ | GENERIC_WRITE | FILE_ATTRIBUTE_NORMAL;
    DWORD create_flag = create ? CREATE_ALWAYS : OPEN_EXISTING;
    DWORD attr_lag = nobuf ? (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH): FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS;
    handle_ = ::CreateFileA(filepath.c_str(), access_flag, 0, NULL, create_flag, attr_lag, NULL);
    if (INVALID_HANDLE_VALUE == handle_) {
      error_ = ::GetLastError();
      return false;
    }

    this->file_path_ = filepath;
    return true;
  }

  void close() {
    if (handle_ != INVALID_HANDLE_VALUE) {
      flush();
      ::CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
  }

  bool seek(size_t offset, uint64_t mode = FILE_BEGIN) {
    if (handle_ == INVALID_HANDLE_VALUE)
      return false;

    LARGE_INTEGER li;
    li.QuadPart = offset;
    li.LowPart = ::SetFilePointer(handle_, li.LowPart, &li.HighPart, (DWORD)mode);
    error_ = ::GetLastError();
    if (li.LowPart == INVALID_SET_FILE_POINTER && error_ != NO_ERROR)
        return false;
    if (li.QuadPart != offset)
        return false;

    return true;
  }

  bool read(uint8_t* buff, size_t bytes_to_read) {
    if (handle_ == INVALID_HANDLE_VALUE)
      return false;

    DWORD rd_bytes = 0;
    if (!::ReadFile(handle_, buff, (DWORD)bytes_to_read, &rd_bytes, 0))
      rd_bytes = 0;

    if (rd_bytes == DWORD(bytes_to_read))
      return true;

    error_= ::GetLastError();
    return false;
  }

  bool write(const uint8_t* data, size_t bytes_to_write) {
    if (handle_ == INVALID_HANDLE_VALUE)
      return false;

    BOOL ret = 0;
    DWORD wr_bytes = 0;

    do {
      ret = ::WriteFile(handle_, data, (DWORD)bytes_to_write, &wr_bytes, NULL);
      bytes_to_write -= wr_bytes;
      data += wr_bytes;
    } while (ret && bytes_to_write > 0);

    if (!ret || bytes_to_write > 0) {
      error_ = ::GetLastError();
      return false;
    }

    return true;
  }

  bool allocate(size_t bytes, bool sparse=true) {
    if (handle_ == INVALID_HANDLE_VALUE)
      return false;

    size_t file_size = size();
    if (bytes <= file_size)
      return true;

    LARGE_INTEGER li;
    li.QuadPart = bytes;
    li.LowPart = ::SetFilePointer(handle_, li.LowPart, &li.HighPart, FILE_BEGIN);
    error_ = ::GetLastError();
    if (li.LowPart == INVALID_SET_FILE_POINTER && error_ != NO_ERROR)
      return false;
    if (li.QuadPart != bytes)
      return false;
    if (!::SetEndOfFile(handle_)) {
      error_ = ::GetLastError();
      return false;
    }
    
    // TODO: check success
    if (sparse && !::SetFileValidData(handle_, bytes)) {
      error_ = ::GetLastError();
      return false;
    }
    return true;
  }

  bool flush() {
    if (handle_ == INVALID_HANDLE_VALUE)
      return false;

    if (::FlushFileBuffers(handle_))
      return true;
    error_ = ::GetLastError();
    return false;
  }

  size_t size() {
    if (handle_ == INVALID_HANDLE_VALUE)
      return 0;

    DWORD hi, lo;
    lo = ::GetFileSize(handle_, &hi);
    return (((size_t)hi) << 32) + lo;
  }

  const std::string& filename() const {
    return file_path_;
  }
  
  uint64_t last_error() const { return error_; }

private:
  HANDLE handle_{INVALID_HANDLE_VALUE};
  std::string file_path_;
  uint64_t error_{0};
};

}

#endif