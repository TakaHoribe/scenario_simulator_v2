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

#include <quaternion_operation/quaternion_operation.h>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <cpp_mock_scenarios/catalogs.hpp>
#include <cpp_mock_scenarios/cpp_scenario_node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <traffic_simulator/api/api.hpp>
#include <traffic_simulator_msgs/msg/driver_model.hpp>

// headers in STL
#include <memory>
#include <string>
#include <vector>

class RequestSpeedChangeScenario : public cpp_mock_scenarios::CppScenarioNode
{
public:
  explicit RequestSpeedChangeScenario(const rclcpp::NodeOptions & option)
  : cpp_mock_scenarios::CppScenarioNode(
      "request_speed_change",
      ament_index_cpp::get_package_share_directory("kashiwanoha_map") + "/map",
      "private_road_and_walkway_ele_fix/lanelet2_map.osm", __FILE__, false, option)
  {
    start();
  }

private:
  void onUpdate() override
  {
    if (api_.getEntityStatus("front").action_status.twist.linear.x < 10.0) {
      stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (
      api_.getCurrentTime() <= 0.9 &&
      api_.getEntityStatus("ego").action_status.twist.linear.x > 10.0) {
      stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (
      api_.getCurrentTime() >= 1.0 &&
      api_.getEntityStatus("ego").action_status.twist.linear.x <= 10.0) {
      stop(cpp_mock_scenarios::Result::SUCCESS);
    }
  }

  void onInitialize() override
  {
    api_.spawn("ego", getVehicleParameters());
    api_.setEntityStatus(
      "ego", traffic_simulator::helper::constructLaneletPose(34741, 0, 0),
      traffic_simulator::helper::constructActionStatus(0));
    api_.requestSpeedChange(
      "ego", 10.0, traffic_simulator::SpeedChangeTransition::LINEAR,
      traffic_simulator::SpeedChangeConstraint(
        traffic_simulator::SpeedChangeConstraint::Type::LONGITUDINAL_ACCELERATION, 10.0),
      true);
    api_.spawn("front", getVehicleParameters());
    api_.setEntityStatus(
      "front", traffic_simulator::helper::constructLaneletPose(34741, 10, 0),
      traffic_simulator::helper::constructActionStatus(0));
    api_.requestSpeedChange(
      "front", 10.0, traffic_simulator::SpeedChangeTransition::STEP,
      traffic_simulator::SpeedChangeConstraint(
        traffic_simulator::SpeedChangeConstraint::Type::LONGITUDINAL_ACCELERATION, 10.0),
      true);
  }
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  auto component = std::make_shared<RequestSpeedChangeScenario>(options);
  rclcpp::spin(component);
  rclcpp::shutdown();
  return 0;
}