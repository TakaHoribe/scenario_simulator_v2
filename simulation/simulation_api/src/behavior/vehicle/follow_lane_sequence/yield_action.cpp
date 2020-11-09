// Copyright 2015-2020 Autoware Foundation. All rights reserved.
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

#include <simulation_api/behavior/vehicle/follow_lane_sequence/yield_action.hpp>
#include <simulation_api/behavior/vehicle/behavior_tree.hpp>

#include <boost/algorithm/clamp.hpp>

#include <string>
#include <vector>
#include <memory>

namespace entity_behavior
{
namespace vehicle
{
namespace follow_lane_sequence
{
YieldAction::YieldAction(
  const std::string & name,
  const BT::NodeConfiguration & config)
: entity_behavior::VehicleActionNode(name, config) {}

boost::optional<double> YieldAction::calculateTargetSpeed(
  std::vector<std::int64_t> following_lanelets)
{
  if (entity_status.coordinate == simulation_api::entity::CoordinateFrameTypes::WORLD) {
    return boost::none;
  }
  auto distance_to_stop_target = getYieldStopDistance(following_lanelets);
  if (!distance_to_stop_target) {
    return boost::none;
  }
  double rest_distance = distance_to_stop_target.get() -
    (vehicle_parameters->bounding_box.dimensions.length);
  if (rest_distance < calculateStopDistance()) {
    if (rest_distance > 0) {
      return std::sqrt(2 * 5 * rest_distance);
    } else {
      return 0;
    }
  }
  return entity_status.twist.linear.x;
}

BT::NodeStatus YieldAction::tick()
{
  getBlackBoardValues();
  if (request != "none" && request != "follow_lane") {
    return BT::NodeStatus::FAILURE;
  }
  if (entity_status.coordinate == simulation_api::entity::CoordinateFrameTypes::WORLD) {
    return BT::NodeStatus::FAILURE;
  }
  auto following_lanelets = hdmap_utils->getFollowingLanelets(entity_status.lanelet_id, 50);
  const auto right_of_way_entities = getRightOfWayEntities(following_lanelets);
  if (right_of_way_entities.size() == 0) {
    if (!target_speed) {
      target_speed = hdmap_utils->getSpeedLimit(following_lanelets);
    }
    setOutput("updated_status", calculateEntityStatusUpdated(target_speed.get()));
    return BT::NodeStatus::SUCCESS;
  }
  target_speed = calculateTargetSpeed(following_lanelets);
  if (!target_speed) {
    target_speed = hdmap_utils->getSpeedLimit(following_lanelets);
  }
  setOutput("updated_status", calculateEntityStatusUpdated(target_speed.get()));
  return BT::NodeStatus::RUNNING;
}
}  // namespace follow_lane_sequence
}  // namespace vehicle
}  // namespace entity_behavior