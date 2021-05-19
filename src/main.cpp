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

#include "dial.h"
#include "programopts.h"
#include "timer.h"
#include "utils.h"

#include <iio.h>

#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

using std::string;

class DialOpts : public ProgramOpts {
public:
  DialSettings dial_settings;
  std::string config_file_name = "/etc/turnandrun.conf"; // config file name
  bool dry_run = false;
  bool just_raw_values = false;

  DialOpts() : ProgramOpts("turnandrun", "0.01") {}
  void process_command_line(int argc, char **argv);
  void usage();
};

void DialOpts::usage()
{
  fprintf(stdout, R"(
Usage: %s [options]

A 10k potentiometer is connected to an ADS1115 or ADS1015 ADC and a
simple configuration file determines commands to run at certain
positions.

Options
%s
  -c <file>  configuration file name (default: /etc/turnandrun.conf), may
             contain the following lines (with two or more command lines)

               dial_reading_number = command_label, command_to_run

               channel = letter             (default: a, valid: a, b, c, d)
               turn_before_run = bool       (default: 1, valid: 0, 1)
               command_delay = seconds      (default: 1, range: 0 - 10)
               frequency = per_second       (default: 10 range: 1 - 100)
               dead_zone = percent_band    (default: 5, range: 0 - 50)
  -d         dry run, no commands are run, print a report to the screen of
             the configuration options, the current dial value and mark it
             corresponds to, and each time a command would be run.
)",
          get_program_name().c_str(), help_ver_text);
}

void DialOpts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hc:d")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'c':
      config_file_name = optarg;
      break;

    case 'd':
      dry_run = true;
      break;

    default:
      error("unknown command line error");
    }
  }

  if (argc - optind > 0)
    error(msg_str("invalid option or parameter: '%s'", argv[optind]));
}

int read_line(FILE *file, char **line)
{

  int linesize = 128;
  *line = (char *)malloc(linesize);
  if (!*line)
    return -1;

  int offset = 0;
  while (true) {
    if (!fgets(*line + offset, linesize - offset, file)) {
      if (offset != 0)
        return 0;
      else {
        *(*line + offset) = '\0'; // terminate the line
        return (ferror(file)) ? -1 : 1;
      }
    }
    int len = offset + strlen(*line + offset);
    if ((*line)[len - 1] == '\n') {
      (*line)[len - 1] = 0;
      return 0;
    }
    offset = len;

    char *newline = (char *)realloc(*line, linesize * 2);
    if (!newline)
      return -1;
    *line = newline;
    linesize *= 2;
  }
}

// Trim whitespace - https://stackoverflow.com/a/17976541
std::string trim(const std::string &s)
{
  auto wsfront = std::find_if_not(s.begin(), s.end(),
                                  [](int c) { return std::isspace(c); });
  auto wsback = std::find_if_not(s.rbegin(), s.rend(),
                                 [](int c) { return std::isspace(c); })
                    .base();
  return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
}

Status read_config_file(const string &file_name, DialSettings &dial_settings)
{
  dial_settings = DialSettings();

  FILE *file = fopen(file_name.c_str(), "r");
  if (file == NULL)
    return Status::error("could not open file");

  // config file has two kinds of lines:
  //    setting = value
  //    dial_reading = command_id , command

  char *line;
  int line_no = 0;
  int ret;
  while ((ret = read_line(file, &line)) == 0) {
    line_no++;
    auto line_str = trim(line); // to ignore whitespace-only lines
    free(line);                 // memory freed before any return

    if (!line_str.empty()) {
      string msg_prefix_line = "line " + std::to_string(line_no) + ": ";

      // split line on first '='
      auto pos_equal = line_str.find('=');
      if (pos_equal == string::npos)
        return Status::error(msg_prefix_line + "did not include '='");

      auto setting = trim(line_str.substr(0, pos_equal));

      // check if setting is setting string or dial reading number
      int dial_reading;
      if (read_int(setting.c_str(), &dial_reading)) {
        // line: dial_reading = command_id , command
        string msg_prefix_cmd = msg_prefix_line + "dial command: ";

        // split value on first ','
        auto pos_comma = line_str.find(',', pos_equal + 1);
        if (pos_comma == string::npos)
          return Status::error(msg_prefix_cmd + "did not include a ','");

        // label before first ','
        string cmd_label =
            trim(line_str.substr(pos_equal + 1, pos_comma - pos_equal - 1));

        // command after first ','
        string cmd_command = trim(line_str.substr(pos_comma + 1));

        Status stat =
            dial_settings.set_command(dial_reading, cmd_label, cmd_command);
        if (!stat)
          return Status::error(msg_prefix_line + "dial command: " + stat.msg());
      }
      else {
        // line: setting = value
        auto value = trim(line_str.substr(pos_equal + 1));
        Status stat = dial_settings.set_setting(setting, value);
        if (!stat)
          return Status::error(msg_prefix_line + "setting: " + stat.msg());
      }
    }
  }
  fclose(file);

  if (dial_settings.get_commands().size() < 2)
    return Status::error("included less than two commands");

  if (ret < 0)
    return Status::error("memory allocation error while reading file");

  return Status::ok();
}

Status loop(iio_device *device, const DialSettings &dial_settings)
{
  string attr_v_raw =
      "in_voltage" + std::to_string(dial_settings.get_channel_num()) + "_raw";

  auto dial_bands =
      dial_settings.create_dial_bands(dial_settings.get_dead_zone());

  if (dial_settings.get_dry_run()) {
    printf("\n== Configuration Settings ==\n");
    printf("%s\n", dial_settings.settings_report().c_str());
    printf("\n== Band Settings ==\n");
    printf("%s\n", dial_settings.dial_bands_report(dial_bands).c_str());
  }

  // dial position mark that was last stopped on
  long mark_stop = DialBands::unset;

  // dial postion mark for last raw value
  long mark_last = DialBands::unset;

  // check whether to execute current command on start, or wait for dial change
  // (by setting a long intial delay on the same band before running command)
  double initial_delay = (dial_settings.get_turn_before_run())
                             ? 10000000                           // long time
                             : dial_settings.get_command_delay(); // usual time
  Timer timer(initial_delay);

  if (dial_settings.get_dry_run())
    printf("\n== Processing Loop ==\n");

  bool first_loop = true;
  while (true) {
    long long raw;
    if (iio_device_attr_read_longlong(device, attr_v_raw.c_str(), &raw) != 0) {
      return Status::error("could not read values from ADS1X15 device from " +
                           attr_v_raw);
    }

    auto mark_now = dial_bands.get_mark(raw); // mark for current raw value
    if (first_loop) {
      mark_last = mark_now; // initially no change
      first_loop = false;
    }

    // If current mark has changed then restart the timer
    if (mark_now != mark_last)
      timer.set_timer(dial_settings.get_command_delay());

    // check if the dial...
    //    has been in the current band for the delay time, AND
    //    is no longer in the last stop band, AND
    //    is not in a dead zone
    if (timer.finished() && mark_now != mark_stop &&
        mark_now != DialBands::unset) {
      // dial has stopped in a new band
      mark_stop = mark_now;
      auto cmd = dial_settings.get_command(mark_stop);
      if (dial_settings.get_dry_run())
        printf("COMMAND (mark: %-10ld) %s: %s\n", mark_stop, cmd.label.c_str(),
               cmd.command.c_str());
      else
        system(cmd.command.c_str());
    }

    if (dial_settings.get_dry_run()) {
      string str = msg_str("raw: %-7lld, mark: ", raw);
      if (mark_now == DialBands::unset)
        str += "dead zone";
      else {
        auto label = dial_settings.get_command(mark_now).label;
        str += msg_str("%-7ld, label: %s ", mark_now,
                       label.substr(0, std::min(40u, label.size())).c_str());
      }
      printf("%-70s\r", str.c_str());
      fflush(stdout);
    }

    mark_last = mark_now;

    usleep(1000000 / dial_settings.get_frequency());
  }

  return Status::ok();
}

int main(int argc, char **argv)
{
  DialOpts opts;
  opts.process_command_line(argc, argv);

  DialSettings dial_settings;
  Status stat = read_config_file(opts.config_file_name, dial_settings);
  if (opts.dry_run) {
    dial_settings.set_dry_run();
    // print result but don't exit on error
    printf("\n== Read Configuration File ==\n");
    printf("File name: '%s': ", opts.config_file_name.c_str());
    if (stat.is_ok())
      printf("ok");
    else if (stat.is_warning())
      printf("warning: %s", stat.c_msg());
    else
      printf("error: %s", stat.c_msg());
    printf("\n");
  }
  else
    opts.print_status_or_exit(stat,
                              "config file '" + opts.config_file_name + "'");

  auto iio_context = iio_create_local_context();
  auto device = iio_context_find_device(iio_context, "ads1015");
  if (device == nullptr)
    opts.error("could not open ADS1X15 device");

  opts.print_status_or_exit(loop(device, dial_settings));

  return 0;
}
