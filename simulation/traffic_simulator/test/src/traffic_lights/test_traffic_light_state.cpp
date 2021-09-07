// Copyright 2015-2020 Tier IV, Inc. All rights reserved.
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

#include <gtest/gtest.h>

#include <regex>
#include <scenario_simulator_exception/exception.hpp>
#include <traffic_simulator/traffic_lights/traffic_light_state.hpp>

TEST(TrafficLights, makeLampState)
{
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightColor::RED).type,
    autoware_perception_msgs::msg::LampState::RED);
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightColor::GREEN).type,
    autoware_perception_msgs::msg::LampState::GREEN);
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightColor::YELLOW).type,
    autoware_perception_msgs::msg::LampState::YELLOW);
  EXPECT_THROW(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightColor::NONE),
    std::out_of_range);
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightArrow::STRAIGHT).type,
    autoware_perception_msgs::msg::LampState::UP);
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightArrow::RIGHT).type,
    autoware_perception_msgs::msg::LampState::RIGHT);
  EXPECT_EQ(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightArrow::LEFT).type,
    autoware_perception_msgs::msg::LampState::LEFT);
  EXPECT_THROW(
    traffic_simulator::makeLampState(traffic_simulator::TrafficLightArrow::NONE),
    std::out_of_range);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}