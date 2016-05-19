/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2016 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <swirly/fig/Mock.hpp>

#include "RestServ.hpp"

#include <swirly/fir/Rest.hpp>

#include <swirly/ash/Conf.hpp>
#include <swirly/ash/Log.hpp>
#include <swirly/ash/System.hpp>
#include <swirly/ash/Time.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#pragma GCC diagnostic pop

#include <iomanip>
#include <iostream>
#include <system_error>

#include <cerrno>

#include <fcntl.h> // open()
#include <syslog.h>
#include <unistd.h> // dup2()

using namespace std;
using namespace swirly;

namespace fs = boost::filesystem;

namespace {

volatile sig_atomic_t sig_{0};

void sigHandler(int sig) noexcept
{
  sig_ = sig;

  // Re-install signal handler.
  signal(sig, sigHandler);
}

void openLogFile(const char* path)
{
  const int fd{open(path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP)};
  if (fd < 0) {
    throw system_error(errno, system_category(), "open failed");
  }

  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  close(fd);
}

struct Opts {
  fs::path confFile;
  bool daemon{true};
  bool testMode{false};
};

void printUsage(ostream& os)
{
  os << R"==(Usage: swirlyd [options]

Options:
  -h                Show this help message.
  -f path           Path to configuration file.
  -n                Do not daemonise. I.e. run in the foreground.
  -t                Run in functional test mode.

Report bugs to: support@swirlycloud.com
)==";
}

void getOpts(int argc, char* argv[], Opts& opts)
{
  opterr = 0;
  int ch;
  while ((ch = getopt(argc, argv, ":f:hnt")) != -1) {
    switch (ch) {
    case 'f':
      opts.confFile = optarg;
      break;
    case 'h':
      printUsage(cout);
      exit(0);
    case 'n':
      opts.daemon = false;
      break;
    case 't':
      opts.testMode = true;
      break;
    case ':':
      cerr << "Option '" << static_cast<char>(optopt) << "' requires an argument\n";
      printUsage(cerr);
      exit(1);
    case '?':
    default:
      cerr << "Unknown option '" << static_cast<char>(optopt) << "'\n";
      printUsage(cerr);
      exit(1);
    }
  }

  if (optind < argc) {
    cerr << "Unknown argument '" << argv[optind] << "'\n";
    printUsage(cerr);
    exit(1);
  }
}

} // anonymous

int main(int argc, char* argv[])
{
  int ret = 1;
  try {

    Opts opts;
    getOpts(argc, argv, opts);

    Conf conf;
    if (!opts.confFile.empty()) {
      fs::ifstream is{opts.confFile};
      if (!is.is_open()) {
        throw Exception{errMsg() << "open failed: " << opts.confFile};
      }
      conf.read(is);
    }

    const char* const logLevel{conf.get("log_level")};
    if (logLevel) {
      setLogLevel(atoi(logLevel));
    }

    // Restrict file creation mask if specified. The umask function is always successful.
    const char* const fileMode{conf.get("file_mode")};
    if (fileMode) {
      // Zero base to auto-detect: if the prefix is 0, the base is octal, if the prefix is 0x or 0X.
      umask(static_cast<mode_t>(strtol(fileMode, nullptr, 0)));
    } else if (opts.daemon) {
      umask(0027);
    }

    fs::path runDir{conf.get("run_dir", "")};
    if (!runDir.empty()) {
      // Change the current working directory if specified.
      runDir = fs::canonical(runDir, fs::current_path());
      if (chdir(runDir.c_str()) < 0) {
        throw system_error(errno, system_category(), "chdir failed");
      }
    } else if (opts.daemon) {
      // Default to root directory if daemon.
      runDir = fs::current_path().root_path();
    } else {
      // Otherwise, default to current directory.
      runDir = fs::current_path();
    }

    fs::path logFile{conf.get("log_file", "")};
    if (opts.daemon) {

      // Daemonise process.
      daemon();

      // Daemon uses syslog by default.
      if (logFile.empty()) {
        openlog("swirlyd", LOG_PID | LOG_NDELAY, LOG_LOCAL0);
        setlogmask(LOG_UPTO(LOG_DEBUG));
        setLogger(sysLogger);
      }
    }

    if (!logFile.empty()) {

      // Log file is relative to working directory. We use absolute, rather than canonical here,
      // because canonical requires that the file exists.
      if (logFile.is_relative()) {
        logFile = fs::absolute(logFile, runDir);
      }

      fs::create_directory(logFile.parent_path());

      SWIRLY_NOTICE(logMsg() << "opening log file: " << logFile);
      openLogFile(logFile.c_str());
    }

    unique_ptr<Journ> journ;
    unique_ptr<Model> model;
    if (!opts.testMode) {
      journ = swirly::makeJourn(conf);
      model = swirly::makeModel(conf);
    } else {
      // Use Mock classes for functional testing.
      journ = make_unique<MockJourn>();
      model = make_unique<MockModel>();
    }
    const char* const httpPort{conf.get("http_port", "8080")};
    const char* const httpUser{conf.get("http_user", "Swirly-User")};

    Rest rest{*journ};
    rest.load(*model);
    model = nullptr;

    mg::RestServ rs{rest, httpUser, opts.testMode ? "Swirly-Time" : nullptr};

    auto& conn = rs.bind(httpPort);
    mg_set_protocol_http_websocket(&conn);

    SWIRLY_NOTICE(logMsg() << "started swirlyd server on port " << httpPort);

    SWIRLY_INFO(logMsg() << "conf_file: " << opts.confFile);
    SWIRLY_INFO(logMsg() << "daemon:    " << (opts.daemon ? "yes" : "no"));
    SWIRLY_INFO(logMsg() << "test_mode: " << (opts.testMode ? "yes" : "no"));

    SWIRLY_INFO(logMsg() << "file_mode: " << setfill('0') << setw(3) << oct << swirly::fileMode());
    SWIRLY_INFO(logMsg() << "run_dir:   " << runDir);
    SWIRLY_INFO(logMsg() << "log_file:  " << logFile);
    SWIRLY_INFO(logMsg() << "log_level: " << getLogLevel());
    SWIRLY_INFO(logMsg() << "http_port: " << httpPort);
    SWIRLY_INFO(logMsg() << "http_user: " << httpUser);

    signal(SIGHUP, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    bool quit{false};
    do {
      rs.poll(1000);

      int sig{sig_};
      sig_ = 0;
      switch (sig) {
      case SIGHUP:
        SWIRLY_INFO("received SIGHUP"_sv);
        if (!logFile.empty()) {
          SWIRLY_NOTICE(logMsg() << "reopening log file: " << logFile);
          openLogFile(logFile.c_str());
        }
        break;
      case SIGINT:
        SWIRLY_INFO("received SIGINT"_sv);
        quit = true;
        break;
      case SIGTERM:
        SWIRLY_INFO("received SIGTERM"_sv);
        quit = true;
        break;
      }
    } while (!quit);

    ret = 0;

  } catch (const exception& e) {
    SWIRLY_ERROR(logMsg() << "exception: " << e.what());
  }
  SWIRLY_NOTICE("stopped swirlyd server"_sv);
  return ret;
}