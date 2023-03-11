#pragma once

#include <atomic>
#include <signal.h>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <iostream>
#include <boost/stacktrace.hpp>
#include <boost/filesystem.hpp>
#endif

static void signal_handle(int sig);
static LONG crash_handle(EXCEPTION_POINTERS *pException);
static void CreateDumpFile(LPCWSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException);

class signal {
public:
  static signal& get() {
    static signal signal_;
	  return signal_;
  }

  signal() {
    #ifndef WIN32
    if (boost::filesystem::exists("./stacktrace.dump")) {
      std::ifstream ifs("./stacktrace.dump");
      boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
      std::cout << st << std::endl;
      ifs.close();
      boost::filesystem::remove("./stacktrace.dump");
    }
    #endif
  }

  void install_signal() {
    ::signal(SIGINT, signal_handle);
    ::signal(SIGTERM, signal_handle);
    ::signal(SIGSEGV, signal_handle);
    ::signal(SIGABRT, signal_handle);
    #ifdef WIN32
    ::SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)crash_handle);
    #endif
  }

  void signal_stop() { sig_stop_.store(true); }

  bool stopped() { return sig_stop_.load(); }

private:
  std::atomic<bool> sig_stop_;
};

static void signal_handle(int sig)
{
  signal::get().signal_stop();
  spdlog::info("signal handled {} stopping...", sig);
  #ifndef WIN32
  boost::stacktrace::safe_dump_to("./stacktrace.dump");
  #endif
}

#ifdef WIN32
static LONG crash_handle(EXCEPTION_POINTERS *pException) {
  HANDLE hDumpFile = CreateFile(_T("crash.dmp"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	dumpInfo.ExceptionPointers = pException;
	dumpInfo.ThreadId = GetCurrentThreadId();
	dumpInfo.ClientPointers = TRUE;

	::MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
	::CloseHandle(hDumpFile);

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif