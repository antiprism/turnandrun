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

#include <limits>
#include <map>
#include <string>
#include <vector>

class DialBands {
public:
  static const long unset = std::numeric_limits<long>::min();
  void add_band(long k, long v) { bands[k] = v; }
  long get_mark(long val) const;
  const std::map<long, long> &get_bands() const { return bands; }

private:
  std::map<long, long> bands;
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

  char get_channel() const { return channel; }
  bool get_turn_before_run() const { return turn_before_run; }
  double get_command_delay() const { return command_delay; }
  double get_dead_zone() const { return dead_zone; }
  double get_frequency() const { return frequency; }
  int get_channel_num() const { return channel - 'a'; }
  std::map<long, Command> get_commands() const { return commands; }
  DialBands create_dial_bands(double dead_frac) const;
  std::string dial_bands_report(const DialBands &dial_bands) const;

  std::string settings_report() const;
  void set_dry_run(bool dry = true) { dry_run = dry; }
  bool get_dry_run() const { return dry_run; }

private:
  bool turn_before_run = true; // turn dial before first command is run
  char channel = 'a';          // AD1X15 channel (a, b, c, or d)
  double command_delay = 1;    // secs stopped before command is run
  double dead_zone = 0.05;     // dead fraction between bands
  double frequency = 10.0;     // polling frequency
  std::map<long, Command> commands;

  bool dry_run = false; // verbose output, do not run commands
};

#endif // DIAL_H
