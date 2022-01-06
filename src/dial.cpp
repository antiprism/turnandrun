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
#include "utils.h"

#include <algorithm>
#include <cstring>
#include <set>
#include <thread>

using std::string;
using std::vector;

long DialBands::get_mark(long kval, long cur_mark) const
{
  long mark = unset;
  auto it_after = bands.upper_bound(kval);
  if (bands.size() > 0) {
    std::pair<long, long> vals;
    if (it_after == bands.end())        // kval is beyond the highest key
      vals = bands.rbegin()->second;    // largest mark;
    else if (it_after == bands.begin()) // kval is before lowest mark
      vals = bands.begin()->second;
    else
      vals = (--it_after)->second; // use start of including band

    const long jump = vals.first;
    const long stay = vals.second;
    if (cur_mark == stay || cur_mark == jump)
      mark = cur_mark;
    else
      mark = jump;
  }
  return mark;
}

Status DialSettings::set_command(int dial_reading, std::string cmd_label,
                                 std::string cmd_command)
{
  if (dial_reading < 0)
    return Status::error("dial reading cannot be negative");

  if (cmd_label.empty())
    return Status::error("no command label given");

  if (cmd_command.empty())
    return Status::error("no command given");

  commands[dial_reading] = {cmd_label, cmd_command};

  return Status::ok();
}

DialSettings::Command DialSettings::get_command(long dial_reading) const
{
  auto it = commands.find(dial_reading);
  return (it == commands.end()) ? Command() : it->second;
}

Status DialSettings::set_setting(std::string setting, std::string value)
{
  if (setting.empty())
    return Status::error("no setting given");

  if (value.empty())
    return Status::error("no value given");

  string msg_prefix = "setting '" + setting + "': ";
  if (setting == "overlap" || setting == "command_delay" ||
      setting == "frequency") {
    string msg_prefix2 = msg_prefix + "value '" + value + "': ";
    double num;
    Status stat = read_double(value.c_str(), &num);
    if (!stat)
      return Status::error(msg_prefix2 + "not a number: " + stat.msg());

    int lim_low = (setting == "frequency") ? 1 : 0;
    int lim_high = (setting == "command_delay") ? 10 : 50;
    if (num < lim_low || num > lim_high)
      return Status::error(msg_prefix2 + "must be in range " +
                           std::to_string(lim_low) + " to " +
                           std::to_string(lim_high));

    if (setting == "overlap")
      overlap = num / 100;
    else if (setting == "command_delay")
      command_delay = num;
    else // setting == frquency
      frequency = num;
  }
  else if (setting == "enabled" || setting == "print_commands" ||
           setting == "run_commands" || setting == "turn_before_run") {
    if (value.size() != 1 || !strchr("01", value[0]))
      return Status::error(msg_prefix + "must be one digit, 0 or 1");
    int flag = (value[0] == '1');
    if (setting == "enabled")
      set_enabled(flag);
    else if (setting == "print_commands")
      set_print_commands(flag);
    else if (setting == "run_commands")
      set_run_commands(flag);
    else
      turn_before_run = flag;
  }
  else
    return Status::error(msg_prefix + "unknown setting");

  return Status::ok();
}

DialBands DialSettings::create_dial_bands() const
{
  DialBands dial_bands;
  auto overlap_frac = get_overlap();
  if (get_commands().size() >= 2) {
    long cur_mark;
    long prev_mark = DialBands::unset;
    for (auto kp : get_commands()) {
      cur_mark = kp.first;
      if (prev_mark == DialBands::unset) { // cur_mark is first mark
        dial_bands.add_band(cur_mark, cur_mark, cur_mark);
      }
      else {
        long mid_point = (prev_mark + cur_mark) / 2;
        if (overlap_frac > 0) {
          long overlap = (cur_mark - prev_mark) * overlap_frac;
          dial_bands.add_band(mid_point - overlap / 2, prev_mark, cur_mark);
          dial_bands.add_band(mid_point, cur_mark, prev_mark);
          dial_bands.add_band(mid_point + overlap / 2, cur_mark, cur_mark);
        }
        else
          dial_bands.add_band(mid_point, cur_mark, cur_mark);
      }
      prev_mark = cur_mark;
    }

    dial_bands.add_band(prev_mark, prev_mark, prev_mark);
  }

  return dial_bands;
};

std::string DialSettings::dial_bands_report(const DialBands &dial_bands) const
{
  string str;
  const auto &bands = dial_bands.get_bands();

  const char *tab = "        ";
  if (bands.size() < 2)
    str = "  no command bands (need at least two commands)\n";
  else {
    vector<long> vals = {0L, 0L};
    auto cmd = commands.begin();
    for (auto band : bands) {
      vals.push_back(band.first);
      // str += msg_str("< %7ld | %7ld | %7ld >\n", band->first,
      //               band->second.first, band->second.second);
      if (vals.size() == 3) {
        if (vals[0] == 0)
          str += msg_str("%s<%6ld | <%6ld | <%6ld\n", tab, 0L, 0L, 0L);
        else
          str += msg_str("%s %6ld |  %6ld |  %6ld\n", tab, vals[0], vals[1],
                         vals[2]);
        str += msg_str("  %6ld: %s: %s\n", cmd->first,
                       cmd->second.label.c_str(), cmd->second.command.c_str());
        cmd = std::next(cmd);
        vals.clear();
      }
    }
    str += msg_str("%s>%6ld | >%6ld | >%6ld\n", tab, vals[0], vals[0], vals[0]);
  }

  return str;
}

std::string DialSettings::settings_report() const
{
  string str;
  str += msg_str("  enabled = %d\n", enabled);
  str += msg_str("  turn_before_run = %d\n", turn_before_run);
  str += msg_str("  command_delay = %g\n", command_delay);
  str += msg_str("  overlap = %g\n", overlap * 100); // convert to %
  str += msg_str("  frequency = %g\n", frequency);
  str += msg_str("  run_commands = %d\n", run_commands);
  str += msg_str("  print_commands = %d\n", print_commands);
  str += "\n";
  if (commands.size()) {
    for (auto kp : commands)
      str += msg_str("  %7ld = %s, %s\n", kp.first, kp.second.label.c_str(),
                     kp.second.command.c_str());
    str += "\n";
  }

  return str;
}

Status Ads1x15::init(const DialSettings &default_settings, int num_channels)
{
  context = iio_create_local_context();
  device = iio_context_find_device(context, "ads1015");
  if (device == nullptr)
    return Status::error("could not open ADS1X15 device");
  dials.clear();
  for (int i = 0; i < num_channels; i++) {
    dials.push_back(std::make_unique<Dial>());
    *dials.back()->get_settings() = default_settings;
  }
  return Status::ok();
};

std::string Ads1x15::config_report() const
{
  string report;
  for (size_t idx = 0; idx < dials.size(); idx++) {
    const auto settings = dials[idx]->get_settings();
    if (!settings->is_enabled())
      continue; // Don't report disabled channels

    report += msg_str("\n== CHANNEL %c ==\n\n", channel_idx_to_char(idx));
    report += "-- Configuration Settings --\n";
    lock();
    report += settings->settings_report();
    unlock();

    report += "-- Band Settings --\n";

    lock();
    auto dial_bands = settings->create_dial_bands();

    report += settings->dial_bands_report(dial_bands);
    unlock();
  }

  return report;
}

Status Ads1x15::read_raw(const std::string &attr, long long *raw)
{
  Status stat;
  lock();
  auto adc_read_ret = iio_device_attr_read_longlong(device, attr.c_str(), raw);
  unlock();

  if (adc_read_ret != 0)
    stat.set_error("could not read values from ADS1X15 device from " + attr);
  return stat;
}

Status Ads1x15::start_dial_loop(int idx)
{
  Status stat;

  auto dial = dials[idx].get();
  const auto settings = dial->get_settings();
  bool run_commands = settings->get_run_commands();

  string attr_v_raw = "in_voltage" + std::to_string(idx) + "_raw";

  auto dial_bands = settings->create_dial_bands();

  // dial postion mark for last raw value
  long mark_last = DialBands::unset;

  // check whether to execute current command on start, or wait for dial change
  // (by setting a long intial delay on the same band before running command)
  double initial_delay = (settings->get_turn_before_run())
                             ? 10000000                       // long time
                             : settings->get_command_delay(); // usual time
  Timer timer(initial_delay);

  bool first_loop = true;
  while (true) {
    long long raw;
    if ((stat = read_raw(attr_v_raw, &raw)).is_error())
      return stat;

    dial->set_raw(raw);

    auto mark_now =
        dial_bands.get_mark(raw, mark_last); // mark for current raw value
    if (first_loop) {
      mark_last = mark_now; // initially no change
      first_loop = false;
    }

    // If current mark has changed then restart the timer
    if (mark_now != mark_last)
      timer.set_timer(settings->get_command_delay());

    // check if the dial...
    //    has been in the current band for the delay time, AND
    //    is no longer in the last stop band, AND
    //    the reading is set
    if (timer.finished() && mark_now != dial->get_mark_stop() &&
        mark_now != DialBands::unset) {
      // dial has stopped in a new band
      dial->set_mark_stop(mark_now);
      auto cmd = settings->get_command(dial->get_mark_stop());

      if (settings->get_print_commands()) {
        printf("\nCOMMAND (mark: %-10ld) %s: %s\n", dial->get_mark_stop(),
               cmd.label.c_str(), cmd.command.c_str());
        fflush(stdout);
      }

      if (run_commands)
        system(cmd.command.c_str());
    }

    mark_last = mark_now;

    usleep(1000000 / settings->get_frequency());
  }

  return stat;
}

Status Ads1x15::start_loop()
{
  Status stat;
  int num_channels = dials.size();
  vector<std::thread> threads(num_channels);
  for (int idx = 0; idx < num_channels; idx++) {
    if (dials[idx]->get_settings()->is_enabled())
      threads[idx] = std::thread(&Ads1x15::start_dial_loop, this, idx);
  }

  for (int idx = 0; idx < num_channels; idx++) {
    if (threads[idx].joinable()) {
      threads[idx].join();
      auto dial_status = dials[idx]->get_status();
      // final status should be first occurence of worst status
      if ((stat.is_ok() && !dial_status.is_ok()) ||
          (stat.is_warning() && dial_status.is_error()))
        stat = dial_status;
    }
  }

  return stat;
}

Status Ads1x15::monitor_loop(double frequency)
{
  while (true) {
    string line;
    int num_channels = dials.size();
    for (int idx = 0; idx < num_channels; idx++) {
      auto dial = dials[idx].get();
      if (!dial->get_settings()->is_enabled())
        continue; // Don't report disabled channels

      char channel_char = channel_idx_to_char(idx);
      long long raw = dial->get_raw();
      long mark_stop = dial->get_mark_stop();
      if (mark_stop == DialBands::unset)
        mark_stop = -1;

      line += msg_str("%c:%6lld (%6ld)  ", channel_char, raw, mark_stop);
    }

    printf("%-80s\r", line.c_str());
    fflush(stdout);
    usleep(1000000 / frequency);
  }

  return Status::ok();
}

namespace {
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
}; // namespace

Status Ads1x15::read_config_file(const string &file_name,
                                 const DialSettings &default_settings,
                                 int num_channels)
{
  auto stat = init(default_settings, num_channels);
  if (!stat)
    return stat;

  // FILE pointer that closes when leaving scope
  auto file = std::unique_ptr<FILE, decltype(&fclose)>(
      fopen(file_name.c_str(), "r"), &fclose);
  if (file.get() == NULL)
    return Status::error("could not open file");

  // config file has three kinds of lines:
  //    CHANNEL a, b, c or d
  //    setting = value
  //    dial_reading = command_id , command

  int channel_char = '\0';          // current channel letter
  DialSettings *settings = nullptr; // current dial settings

  char *line;
  int line_no = 0;
  int ret;
  while ((ret = read_line(file.get(), &line)) == 0) {
    line_no++;
    const auto line_str = trim(line); // to ignore whitespace-only lines
    free(line);                       // memory freed before any return
    if (line_str.empty())
      continue;

    std::set<char> channels_seen;
    string msg_prefix_line = "line " + std::to_string(line_no) + ": ";
    // Check for CHANNEL
    const string channel_label = "CHANNEL";
    if (line_str.substr(0, channel_label.size()) == channel_label) {
      string msg_prefix_channel = msg_prefix_line + "CHANNEL section start: ";
      auto channel_str = trim(line_str.substr(channel_label.size()));
      if (channel_str.size() == 0)
        return Status::error(msg_prefix_channel + "letter not given");
      if (channel_str.size() > 1)
        return Status::error(msg_prefix_channel + "more than one letter given");
      channel_char = tolower(channel_str[0]);
      const string channel_letters = "abcd";
      if (channel_letters.find(channel_char) == string::npos)
        return Status::error(msg_prefix_channel + "unknown channel letter '" +
                             channel_str + "'");
      const auto already_seen = !channels_seen.insert(channel_char).second;
      if (already_seen)
        return Status::error(msg_prefix_channel + "channel '" + channel_str +
                             "' section already given");
      int channel_idx = channel_char_to_idx(channel_char);
      settings = dials[channel_idx]->get_settings();
      settings->set_enabled();

      continue;
    }

    if (!channel_char)
      return Status::error(msg_prefix_line +
                           "first line is not a CHANNEL section start");

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

      Status stat = settings->set_command(dial_reading, cmd_label, cmd_command);
      if (!stat)
        return Status::error(msg_prefix_line + "dial command: " + stat.msg());
    }
    else {
      // line: setting = value
      auto value = trim(line_str.substr(pos_equal + 1));
      Status stat = settings->set_setting(setting, value);
      if (!stat)
        return Status::error(msg_prefix_line + "setting: " + stat.msg());
    }
  }
  if (ret < 0)
    return Status::error("memory allocation error while reading file");

  int enabled_count = 0;
  for (size_t idx = 0; idx < dials.size(); idx++) {
    const auto settings = dials[idx]->get_settings();
    enabled_count += settings->is_enabled();
    if (settings->is_enabled() && settings->get_run_commands() &&
        settings->get_commands().size() < 2)
      return Status::error(
          msg_str("channel '%c' included less than two commands",
                  channel_idx_to_char(idx)));
  }
  if (enabled_count == 0)
    return Status::error("file contained no CHANNEL sections");

  return Status::ok();
}
