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

#include <cstring>

using std::string;

long DialBands::get_mark(long kval) const
{
  auto it_after = bands.upper_bound(kval);
  if (bands.size() == 0)
    return unset;
  else if (it_after == bands.end())   // kval is beyond the highest key
    return bands.rbegin()->second;    // largest mark;
  else if (it_after == bands.begin()) // kval is before lowest mark
    return bands.begin()->second;
  else
    return (--it_after)->second; // use start of including band
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
  if (setting == "dead_zone" || setting == "command_delay" ||
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

    if (setting == "dead_zone")
      dead_zone = num / 100;
    else if (setting == "command_delay")
      command_delay = num;
    else // setting == frquency
      frequency = num;
  }
  else if (setting == "channel") {
    if (value.size() != 1 || !strchr("abcd", value[0]))
      return Status::error(msg_prefix +
                           "must be one letter, from a, b, c or d");
    channel = value[0];
  }
  else if (setting == "turn_before_run") {
    if (value.size() != 1 || !strchr("01", value[0]))
      return Status::error(msg_prefix + "must be one digit, 0 or 1");
    turn_before_run = (value[0] == '1');
  }
  else
    return Status::error(msg_prefix + "unknown setting");

  return Status::ok();
}

DialBands DialSettings::create_dial_bands(double dead_frac) const
{
  DialBands dial_bands;

  if (get_commands().size() >= 2) {
    long cur_mark;
    long prev_mark = DialBands::unset;
    for (auto kp : get_commands()) {
      cur_mark = kp.first;
      if (prev_mark == DialBands::unset) { // cur_mark is first mark
        dial_bands.add_band(cur_mark, cur_mark);
      }
      else {
        long mid_point = (prev_mark + cur_mark) / 2;
        if (dead_frac > 0) {
          long dead_zone = (cur_mark - prev_mark) * dead_frac;
          dial_bands.add_band(mid_point - dead_zone / 2, DialBands::unset);
          dial_bands.add_band(mid_point + dead_zone / 2, cur_mark);
        }
        else
          dial_bands.add_band(mid_point, cur_mark);
      }
      prev_mark = cur_mark;
    }

    dial_bands.add_band(prev_mark, prev_mark);
  }

  return dial_bands;
};

std::string DialSettings::dial_bands_report(const DialBands &dial_bands) const
{
  string str;
  const auto &bands = dial_bands.get_bands();
  const auto unset = DialBands::unset;

  if (bands.size() < 2)
    str = "no command bands (need at least two commands)\n";
  else {
    auto band = bands.begin();
    auto next = bands.end();
    auto next_again = bands.begin();
    while (next_again != bands.end()) {
      string start_str =
          msg_str("%2s%7ld", (next == bands.end()) ? "<" : "", band->first);

      next = std::next(band);
      next_again = (next == bands.end()) ? bands.end() : std::next(next);

      string end_str = (next_again == bands.end())
                           ? msg_str("%7ld%2s", next->first, ">")
                           : msg_str("%7ld%2s", next->first - 1, "");

      auto cmd = get_command(band->second);
      string cmd_str = (band->second == unset) ? "-------------" : cmd.label;
      str += "(" + start_str + " , " + end_str + ") : " + cmd_str + "\n";
      band = next;
    }
  }
  return str;
}

std::string DialSettings::settings_report() const
{
  string str;
  str += msg_str("turn_before_run = %d\n", turn_before_run);
  str += msg_str("channel = %c\n", channel);
  str += msg_str("command_delay = %g\n", command_delay);
  str += msg_str("dead_zone = %g\n", dead_zone);
  str += msg_str("frequency = %g\n", frequency);
  str += "\n";
  for (auto kp : commands)
    str += msg_str("%7ld = %s, %s\n", kp.first, kp.second.label.c_str(),
                   kp.second.command.c_str());

  return str;
}
