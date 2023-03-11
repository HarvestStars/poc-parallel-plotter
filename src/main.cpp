#include <string>
#include <cstdint>
#include <numeric>
#include <algorithm>

#include <boost/compute.hpp>

namespace compute = boost::compute;

#include <OptionParser.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "config.h"
#include "plotter.h"
#include "common/utils.h"

namespace opt = optparse;
using namespace std;

#ifdef WIN32
#define PAUSE() system("pause")
#else
#define std::getc()
#endif

#define COLOR_PRINT_R SPDLOG_ERROR
#define COLOR_PRINT_G SPDLOG_INFO
#define COLOR_PRINT_Y SPDLOG_WARN
#define COLOR_PRINT_N SPDLOG_DEBUG

const string usage = "usage: %prog [OPTION]... [DIRECTORY]...";

const string version = "%prog " PARALLEL_PLOTTER_VER " " GIT_BRANCH "-" GIT_COMMIT_HASH "\n"
  "Copyright (C) 2019-2020 The FreeStyle developers\n"
  "Distributed under the MIT software license, see the accompanying\n"
  "file COPYING or http://www.opensource.org/licenses/mit-license.php.\n";

const string desc = "The Parallel poc2 gpu plotter.";

int main(int argc, char* argv[]) {
  opt::OptionParser parser = opt::OptionParser()
    .usage(usage)
    .version(version)
    .description(desc);

  static_assert(sizeof(unsigned long long) == 8, "unsigned long long must be equal to 8 bytes.");
  parser.add_option("-V", "--verbose").action("count").help("verbose, default: %default");
  parser.add_option("-t", "--test").action("count").help("test mode, default: %default");
  parser.add_option("-i", "--id").action("store").type("uint64_t").set_default(0).help("plot id, default: %default");
  parser.add_option("-s", "--sn").action("store").type("uint64_t").set_default(0).help("start nonce, default: %default");
  parser.add_option("-n", "--num").action("store").type("uint32_t").set_default(10000).help("number of nonces, default: %default");
  parser.add_option("-w", "--weight").action("store").type("double").set_default(1).help("plot file weight, default: %default (GB)");
  parser.add_option("-m", "--mem").action("store").type("double").set_default(0).help("memory to use, default: %default (GB)");
  parser.add_option("-p", "--plot").action("count").help("run plots generation, default: %default");
  parser.add_option("-d", "--diskbench").action("count").help("run disk bench, default: %default");

  parser.add_option("--step").action("store").type("uint32_t").set_default(8192).help("hash calc batch, default: %default");
  parser.add_option("--gws").action("store").type("uint32_t").set_default(0).help("global work size, default: %default");
  parser.add_option("--lws").action("store").type("uint32_t").set_default(0).help("local work size, default: %default");

  char const* const slevels[] = { "trace", "debug", "info", "warning", "error", "critical", "off" };
  parser.add_option("-l", "--level").choices(&slevels[0], &slevels[7]).action("store").type("string").set_default("info").help("log level, default: %default");
  char const* const benchmode[] = { "disabled", "debug", "info", "warning", "error", "critical", "off" };
  parser.add_option("-b", "--bench").action("store").type("uint32_t").set_default(0).help("bench mode, default: %default");

  try {
    opt::Values& options = parser.parse_args(argc, argv);
    vector<string> dirs = parser.args();
    options["argv"] = accumulate(argv, argv+argc, string{"cmdline:"},
                                [](string a, const string b) -> decltype(auto) {
                                  return  std::move(a) + " " + b;
                                });
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("parallel-plotter.log");
    auto sinks = spdlog::sinks_init_list{ console_sink, file_sink };
    auto default_logger = std::make_shared<spdlog::logger>("logger", sinks);
    spdlog::set_default_logger(default_logger);   
    spdlog::set_pattern("%^%v%$");
    spdlog::info(parser.get_version());
    spdlog::info(options["argv"]);

    if ((int)options.get("verbose") > 0) {
      spdlog::set_level(spdlog::level::trace);

      std::vector<compute::platform> platforms = compute::system::platforms();
      spdlog::debug("Listing platforms/devices...");
      std::vector<compute::device> cpu_devices;
      std::vector<compute::device> gpu_devices;
      for(auto& platform : platforms) {
        COLOR_PRINT_Y("Platform {}", platform.name());
        std::vector<compute::device> devices = platform.devices();
        for(auto& device : devices) {
          std::string type;
          if(device.type() & compute::device::gpu)
            gpu_devices.push_back(device), (type = "GPU");
          else if(device.type() & compute::device::cpu)
            cpu_devices.push_back(device), (type = "CPU");
          else if(device.type() & compute::device::accelerator)
            type = "Accelerator";
          else
            type = "Unknown";

          COLOR_PRINT_Y("    {}: {}", type, device.name());
          COLOR_PRINT_Y("        clock_frequency: {}", device.clock_frequency());
          COLOR_PRINT_Y("        compute_units: {}", device.compute_units());
          COLOR_PRINT_Y("        driver_version: {}", device.driver_version());
          if ((int)options.get("verbose") > 1) {
            auto&& ex = device.extensions();
            auto&& sex = accumulate(std::next(ex.begin()), ex.end(), *ex.begin(),
                                    [](string &a, const string &b) -> decltype(auto) {
                                      return  std::move(a) + ", " + b;
                                    });
            COLOR_PRINT_Y("        extensions: {}", sex);
          }
          COLOR_PRINT_Y("        global_memory_size: {}", device.global_memory_size());
          COLOR_PRINT_Y("        local_memory_size: {}", device.local_memory_size());
          COLOR_PRINT_Y("        max_memory_alloc_size: {}", device.max_memory_alloc_size());
          COLOR_PRINT_Y("        max_work_group_size: {}", device.max_work_group_size());
          COLOR_PRINT_Y("        max_work_item_dimensions: {}", device.max_work_item_dimensions());
          COLOR_PRINT_Y("        profile: {}", device.profile());
          COLOR_PRINT_Y("        vendor: {}", device.vendor());
          COLOR_PRINT_Y("        version: {}", device.version());
        }
      }
    }

    if (!dirs.empty()){
      auto&& directories = accumulate(std::next(dirs.begin()), dirs.end(), *dirs.begin(),
                                      [](string &a, const string &b) -> decltype(auto) {
                                        return  std::move(a) + ", " + b;
                                      });
      options["drivers"] = directories;
    }

    #ifdef WIN32
    if (((int)options.get("plot") || (int)options.get("diskbench"))
      && !util::acquire_manage_volume_privs()) {
      spdlog::error("\n\t\t\t************************************************"
                    "\n\t\t\t*** RUN WITHOUT PRIVILEGES will be very slow ***"
                    "\n\t\t\t***    restart with administrative rights.   ***"
                    "\n\t\t\t************************************************\n");
    }
    #endif

    spdlog::set_level(spdlog::level::from_str((string)options.get("level")));
    spdlog::set_pattern("[%H:%M:%S.%f][%t] %^%v%$");

    plotter(options).run();
    spdlog::info("Done!!!");
  } catch(compute::opencl_error& e) {
      spdlog::error("opencl error: [{}] {}", e.error_code(), e.error_string());
  } catch (std::exception& e) {
    spdlog::error("prog exception: `{}`", e.what());
  } catch (...) {
    return -1;
  }
  PAUSE();
  return 0;
}