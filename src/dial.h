/*
   Copyright (c) 2021, Adrian Rossiter

   Antiprism - http://www.antiprism.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included
      in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

#ifndef DIAL_H
#define DIAL_H

#include "status_msg.h"
#include "timer.h"

#include <iio.h>

#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

class DialBands {
public:
  static const long unset = std::numeric_limits<long>::min();
  void add_band(long k, long select, long stay) { bands[k] = {select, stay}; }
  long get_mark(long kval, long cur_val) const;
  const std::map<long, std::pair<long, long>> &get_bands() const
  {
    return bands;
  }

private:
  std::map<long, std::pair<long, long>> bands;
};

class DialSettings {
public:
  struct Command {
    std::string label;
    std::string command;
  };

  Command get_command(long dial_reading) const;
  Status set_command(int dial_reading, std::string cmd_label,
                     std::string cmd_command);
  Status set_setting(std::string setting, std::string value);

  bool get_turn_before_run() const { return turn_before_run; }
  double get_command_delay() const { return command_delay; }
  double get_overlap() const { return overlap; }
  double get_frequency() const { return frequency; }
  void set_print_commands(bool flag = true) { print_commands = flag; }
  bool get_print_commands() const { return print_commands; }
  void set_run_commands(bool flag = true) { run_commands = flag; }
  bool get_run_commands() const { return run_commands; }
  void set_enabled(bool flag = true) { enabled = flag; }
  bool is_enabled() { return enabled; }
  std::map<long, Command> get_commands() const { return commands; }
  DialBands create_dial_bands() const;
  std::string dial_bands_report(const DialBands &dial_bands) const;
  std::string settings_report() const;

private:
  std::map<long, Command> commands; // dial setting to command
  double command_delay = 1;         // secs stopped before command is run
  double overlap = 0.05;            // dead fraction between bands
  double frequency = 10.0;          // polling frequency
  bool turn_before_run = true;      // turn dial before first command is run
  bool print_commands = false;      // print selected command to screen
  bool run_commands = true;         // run selected command
  bool enabled = false;             // is enabled
};

class Dial {
public:
  void lock() const { dial_reading_mutex.lock(); }
  void unlock() const { dial_reading_mutex.unlock(); }

  long get_mark_stop() const
  {
    lock();
    auto mark_copy = mark_stop;
    unlock();
    return mark_copy;
  }
  void set_mark_stop(long mark)
  {
    lock();
    mark_stop = mark;
    unlock();
  }
  long get_raw() const
  {
    lock();
    auto raw_copy = raw;
    unlock();
    return raw_copy;
  }
  void set_raw(long raw_val)
  {
    lock();
    raw = raw_val;
    unlock();
  }
  DialBands get_dial_bands() const
  {
    lock();
    auto bands = settings.create_dial_bands();
    unlock();
    return bands;
  }

  DialSettings *get_settings() { return &settings; }
  const DialSettings *get_settings() const { return &settings; }
  void set_status(Status stat) { status = stat; }
  Status get_status() const { return status; }

private:
  DialSettings settings;                 // Configuration settings
  long mark_stop = DialBands::unset;     // dial mark that was last stopped on
  long long raw = 999999;                // last raw reading (init to dummy)
  mutable std::mutex dial_reading_mutex; // mutex for accessing readings
  Status status;                         // last error
};

class Ads1x15 {
private:
  static const int num_channels_default = 4;
  mutable std::mutex adc_lock;
  iio_context *context = nullptr;
  iio_device *device = nullptr;
  std::vector<std::unique_ptr<Dial>> dials;
  void lock() const { adc_lock.lock(); }
  void unlock() const { adc_lock.unlock(); }

public:
  static int channel_char_to_idx(char channel) { return channel - 'a'; }
  static char channel_idx_to_char(int idx) { return 'a' + idx; }

  Status init(const DialSettings &default_settings,
              int num_channels = num_channels_default);
  Status read_config_file(const std::string &file_name,
                          const DialSettings &default_settings,
                          int num_channels = num_channels_default);
  std::string config_report() const;

  Status read_raw(const std::string &attr, long long *raw);

  Status start_loop();
  Status start_dial_loop(int channel);
  Status monitor_loop(double frequency);

  Dial *get_dial(int idx) { return dials[idx].get(); }
};

#endif // DIAL_H
