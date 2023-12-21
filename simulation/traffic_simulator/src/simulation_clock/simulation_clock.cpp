// Copyright 2015 TIER IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <scenario_simulator_exception/exception.hpp>
#include <traffic_simulator/simulation_clock/simulation_clock.hpp>

namespace traffic_simulator
{
SimulationClock::SimulationClock(bool use_sim_time, double realtime_factor, double frame_rate)
: rclcpp::Clock(RCL_ROS_TIME),
  use_raw_clock_(!use_sim_time),
  realtime_factor_(realtime_factor),
  frame_rate_(frame_rate),
  time_on_initialize_(use_sim_time ? 0 : now().nanoseconds())
{
}

auto SimulationClock::update() -> void { time_ += realtime_factor_ / frame_rate_; }

auto SimulationClock::getCurrentRosTimeAsMsg() -> rosgraph_msgs::msg::Clock
{
  rosgraph_msgs::msg::Clock clock;
  clock.clock = getCurrentRosTime();
  return clock;
}

auto SimulationClock::getCurrentRosTime() -> rclcpp::Time
{
  if (use_raw_clock_) {
    return now();
  } else {
    return time_on_initialize_ + rclcpp::Duration::from_seconds(getCurrentSimulationTime());
  }
}

auto SimulationClock::start() -> void
{
  if (started()) {
    THROW_SIMULATION_ERROR(
      "npc logic is already started. Please check simulation clock instance was destroyed.");
  } else {
    time_offset_ = time_;
  }
}
}  // namespace traffic_simulator
