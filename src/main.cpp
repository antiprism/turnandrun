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
#include "utils.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <thread>
#include <vector>

using std::string;

class DialOpts : public ProgramOpts {
public:
  DialSettings dial_settings;
  std::string config_file_name = "/etc/turnandrun.conf"; // config file name
  bool dry_run = false;
  bool report = false;
  double monitor_freq = 0;

  DialOpts() : ProgramOpts("turnandrun", "0.02") {}
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
  -c <file>  configuration file name (default: /etc/turnandrun.conf)
             Format
               One to four sections, each section starting CHANNEL followed
               by a letter a, b, c, d (corresponding to the ADS1X15 channel
               used for the readings)
                    e.g. CHANNEL a

               The section heading is followed by settings lines
                 turn_before_run = bool       (default: 1, valid: 0, 1)
                    e.g. turn_before_run = false
                 command_delay = seconds      (default: 1, range: 0 - 10)
                    e.g. command_delay = 0.5
                 frequency = per_second       (default: 10 range: 1 - 100)
                    e.g. frequency = 20
                 overlap = percent_band       (default: 5, range: 0 - 50)
                    e.g. overlap = 1
                 enable = bool                (default: 1, valid: 0, 1)
                    e.g. enable = 0
                 print_commands               (default: 0, valid: 0, 1)
                    e.g. print_commands = 1
                 run_commands                 (default: 1, valid: 0, 1)
                    e.g. run_commands = 0

             and should contain two or more command lines
                 dial_reading_number = command_label, command_to_run
                    e.g.  1245 = Play, mpc -q play
  -r         report, print configuration report on startup
  -m <freq>  monitor , print current dial readings to screen with frequency
             freq
  -d         dry run, no commands are run, -r and -m 10 are set, configuration
             file errors do not cause the program to exit
)",
          get_program_name().c_str(), help_ver_text);
}

void DialOpts::process_command_line(int argc, char **argv)
{
  opterr = 0;
  int c;

  handle_long_opts(argc, argv);

  while ((c = getopt(argc, argv, ":hc:rm:d")) != -1) {
    if (common_opts(c, optopt))
      continue;

    switch (c) {
    case 'c':
      config_file_name = optarg;
      break;

    case 'd':
      dry_run = true;
      report = true;
      monitor_freq = 10;
      break;

    case 'm':
      print_status_or_exit(read_double(optarg, &monitor_freq), c);
      if (monitor_freq <= 0)
        error("monitor frequency must be a positive number", c);
      break;

    case 'r':
      report = true;
      break;

    default:
      error("unknown command line error");
    }
  }

  if (argc - optind > 0)
    error(msg_str("invalid option or parameter: '%s'", argv[optind]));
}

int main(int argc, char **argv)
{
  DialOpts opts;
  opts.process_command_line(argc, argv);

  DialSettings default_settings;
  default_settings.set_run_commands(!opts.dry_run);
  default_settings.set_print_commands(opts.report);

  Ads1x15 adc;
  Status stat = adc.read_config_file(opts.config_file_name, default_settings);

  if (opts.report) {
    printf("\n== Configuration File ==\n");
    printf("  file name: '%s':\n", opts.config_file_name.c_str());
    if (stat.is_ok())
      printf("  ok\n");
    else if (stat.is_warning()) // warn never returned, report all if any added
      printf("  warning: %s\n", stat.c_msg());
    else {
      printf("  error: %s\n", stat.c_msg());
      printf("  (only first error reported, file may contain more errors)\n");
    }
    if (stat.is_error()) {
      if (opts.dry_run) {
        printf("\n  Because of configuration file error, using default values\n"
               "for all channels for dry run\n");
        default_settings.set_enabled();
        adc.init(default_settings);
      }
      else
        exit(1);
    }
    printf("%s", adc.config_report().c_str());
    printf("\n== Processing Loop ==\n\n");
  }
  else {
    opts.print_status_or_exit(stat,
                              "config file '" + opts.config_file_name + "'");
  }

  std::thread monitor;
  if (opts.monitor_freq)
    monitor = std::thread(&Ads1x15::monitor_loop, &adc, opts.monitor_freq);

  // opts.print_status_or_exit(adc.start_loop_dry_run('b'));
  opts.print_status_or_exit(adc.start_loop());

  monitor.join();
  return 0;
}
