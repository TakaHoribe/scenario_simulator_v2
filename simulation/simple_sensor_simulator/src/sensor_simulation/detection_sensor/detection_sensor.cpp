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

#include <quaternion_operation/quaternion_operation.h>

#include <algorithm>
#include <autoware_auto_perception_msgs/msg/detected_objects.hpp>
#include <autoware_auto_perception_msgs/msg/tracked_objects.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <geometry/vector3/hypot.hpp>
#include <memory>
#include <simple_sensor_simulator/exception.hpp>
#include <simple_sensor_simulator/sensor_simulation/detection_sensor/detection_sensor.hpp>
#include <simulation_interface/conversions.hpp>
#include <string>
#include <vector>

namespace simple_sensor_simulator
{
auto DetectionSensorBase::isWithinRange(
  const geometry_msgs::Point & point1, const geometry_msgs::Point & point2,
  const double range) const -> bool
{
  auto distanceX = point1.x() - point2.x();
  auto distanceY = point1.y() - point2.y();
  auto distanceZ = point1.z() - point2.z();

  double distance = std::hypot(distanceX, distanceY, distanceZ);
  return distance <= range;
}

auto DetectionSensorBase::getSensorPose(
  const std::vector<traffic_simulator_msgs::EntityStatus> & statuses) const -> geometry_msgs::Pose
{
  for (const auto & status : statuses) {
    if (
      status.type().type() == traffic_simulator_msgs::EntityType::EGO &&
      status.name() == configuration_.entity()) {
      return status.pose();
    }
  }
  throw SimulationRuntimeError("Detection sensor can be attached only ego entity.");
}

auto DetectionSensorBase::getEntityPose(
  const std::vector<traffic_simulator_msgs::EntityStatus> & entity_statuses,
  const std::string & entity_string) const -> geometry_msgs::Pose
{
  for (const auto & entity_status : entity_statuses) {
    if (entity_status.name() == entity_string) {
      return entity_status.pose();
    }
  }

  throw SimulationRuntimeError(
    configuration_.detect_all_objects_in_range()
      ? "Filtered object is not includes in entity statuses"
      : "Detected object by lidar sensor is not included in lidar detected entity");
}

auto DetectionSensorBase::getDetectedObjects(
  const std::vector<traffic_simulator_msgs::EntityStatus> & statuses) const
  -> std::vector<std::string>
{
  std::vector<std::string> detected_objects;
  const auto pose = getSensorPose(statuses);

  for (const auto & status : statuses) {
    if (
      status.name() != configuration_.entity() &&
      isWithinRange(status.pose().position(), pose.position(), 300.0)) {
      detected_objects.emplace_back(status.name());
    }
  }

  return detected_objects;
}

auto DetectionSensorBase::filterObjectsBySensorRange(
  const std::vector<traffic_simulator_msgs::EntityStatus> & entity_statuses,
  const std::vector<std::string> & selected_entity_strings,
  const double detection_sensor_range) const -> std::vector<std::string>
{
  std::vector<std::string> detected_objects;
  const auto sensor_pose = getSensorPose(entity_statuses);

  for (const auto & selected_entity_status : selected_entity_strings) {
    const auto selected_entity_pose = getEntityPose(entity_statuses, selected_entity_status);
    if (
      selected_entity_status != configuration_.entity() &&
      isWithinRange(
        selected_entity_pose.position(), sensor_pose.position(), detection_sensor_range)) {
      detected_objects.emplace_back(selected_entity_status);
    }
  }
  return detected_objects;
}

template <>
auto DetectionSensor<autoware_auto_perception_msgs::msg::DetectedObjects>::applyPositionNoise(
  autoware_auto_perception_msgs::msg::DetectedObject detected_object)
  -> autoware_auto_perception_msgs::msg::DetectedObject
{
  auto position_noise_distribution =
    std::normal_distribution<>(0.0, configuration_.pos_noise_stddev());
  detected_object.kinematics.pose_with_covariance.pose.position.x +=
    position_noise_distribution(random_engine_);
  detected_object.kinematics.pose_with_covariance.pose.position.y +=
    position_noise_distribution(random_engine_);
  return detected_object;
}

template <typename To, typename... From>
auto make(From &&...) -> To;

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status) -> unique_identifier_msgs::msg::UUID
{
  static auto generate_uuid = boost::uuids::name_generator(boost::uuids::random_generator()());
  const auto uuid = generate_uuid(status.name());
  unique_identifier_msgs::msg::UUID message;
  std::copy(uuid.begin(), uuid.end(), message.uuid.begin());
  return message;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status)
  -> autoware_auto_perception_msgs::msg::ObjectClassification
{
  auto object_classification = autoware_auto_perception_msgs::msg::ObjectClassification();

  object_classification.label = [&]() {
    switch (status.subtype().value()) {
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_UNKNOWN:
        return autoware_auto_perception_msgs::msg::ObjectClassification::UNKNOWN;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_CAR:
        return autoware_auto_perception_msgs::msg::ObjectClassification::CAR;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_TRUCK:
        return autoware_auto_perception_msgs::msg::ObjectClassification::TRUCK;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_BUS:
        return autoware_auto_perception_msgs::msg::ObjectClassification::BUS;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_TRAILER:
        return autoware_auto_perception_msgs::msg::ObjectClassification::TRAILER;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_MOTORCYCLE:
        return autoware_auto_perception_msgs::msg::ObjectClassification::MOTORCYCLE;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_BICYCLE:
        return autoware_auto_perception_msgs::msg::ObjectClassification::BICYCLE;
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_PEDESTRIAN:
        return autoware_auto_perception_msgs::msg::ObjectClassification::PEDESTRIAN;
      default:
        return autoware_auto_perception_msgs::msg::ObjectClassification::UNKNOWN;
    }
  }();

  object_classification.probability = 1;

  return object_classification;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status) -> geometry_msgs::msg::Pose
{
  auto pose = geometry_msgs::msg::Pose();
  simulation_interface::toMsg(status.pose(), pose);

  auto center_point = geometry_msgs::msg::Point();
  simulation_interface::toMsg(status.bounding_box().center(), center_point);

  Eigen::Vector3d center = quaternion_operation::getRotationMatrix(pose.orientation) *
                           Eigen::Vector3d(center_point.x, center_point.y, center_point.z);

  pose.position.x = pose.position.x + center.x();
  pose.position.y = pose.position.y + center.y();
  pose.position.z = pose.position.z + center.z();

  return pose;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status) -> geometry_msgs::msg::Twist
{
  auto twist = geometry_msgs::msg::Twist();
  simulation_interface::toMsg(status.action_status().twist(), twist);
  return twist;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status)
  -> autoware_auto_perception_msgs::msg::DetectedObjectKinematics
{
  auto kinematics = autoware_auto_perception_msgs::msg::DetectedObjectKinematics();

  kinematics.pose_with_covariance.pose = make<geometry_msgs::msg::Pose>(status);

  // clang-format off
  kinematics.pose_with_covariance.covariance = {
    /*
       Row-major representation of the 6x6 covariance matrix. The orientation
       parameters use a fixed-axis representation. In order, the parameters
       are: (x, y, z, rotation about X axis, rotation about Y axis, rotation
       about Z axis)
    */
    1, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0,
    0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 1,
  };
  // clang-format on

  kinematics.twist_with_covariance.twist = make<geometry_msgs::msg::Twist>(status);

  kinematics.orientation_availability = [&]() {
    switch (status.subtype().value()) {
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_BICYCLE:
      case traffic_simulator_msgs::EntitySubtype_Enum::EntitySubtype_Enum_MOTORCYCLE:
        return autoware_auto_perception_msgs::msg::DetectedObjectKinematics::SIGN_UNKNOWN;
      default:
        return autoware_auto_perception_msgs::msg::DetectedObjectKinematics::UNAVAILABLE;
    }
  }();

  return kinematics;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status)
  -> autoware_auto_perception_msgs::msg::Shape
{
  auto shape = autoware_auto_perception_msgs::msg::Shape();
  simulation_interface::toMsg(status.bounding_box().dimensions(), shape.dimensions);
  shape.type = autoware_auto_perception_msgs::msg::Shape::BOUNDING_BOX;
  return shape;
}

template <>
auto make(const traffic_simulator_msgs::EntityStatus & status)
  -> autoware_auto_perception_msgs::msg::DetectedObject
{
  auto detected_object = autoware_auto_perception_msgs::msg::DetectedObject();
  // clang-format off
  detected_object.classification.push_back(make<autoware_auto_perception_msgs::msg::ObjectClassification    >(status));
  detected_object.kinematics             = make<autoware_auto_perception_msgs::msg::DetectedObjectKinematics>(status);
  detected_object.shape                  = make<autoware_auto_perception_msgs::msg::Shape                   >(status);
  // clang-format on
  return detected_object;
}

template <>
auto make(
  const traffic_simulator_msgs::EntityStatus & status,
  const autoware_auto_perception_msgs::msg::DetectedObject & detected_object)
  -> autoware_auto_perception_msgs::msg::TrackedObject
{
  // ref: https://github.com/autowarefoundation/autoware.universe/blob/main/common/perception_utils/src/conversion.cpp
  auto tracked_object = autoware_auto_perception_msgs::msg::TrackedObject();
  // clang-format off
  tracked_object.object_id                           = make<unique_identifier_msgs::msg::UUID>(status);
  tracked_object.existence_probability               = detected_object.existence_probability;
  tracked_object.classification                      = detected_object.classification;
  tracked_object.kinematics.orientation_availability = detected_object.kinematics.orientation_availability;
  tracked_object.kinematics.pose_with_covariance     = detected_object.kinematics.pose_with_covariance;
  tracked_object.kinematics.twist_with_covariance    = detected_object.kinematics.twist_with_covariance;
  tracked_object.shape                               = detected_object.shape;
  // clang-format on
  return tracked_object;
};

template <>
auto DetectionSensor<autoware_auto_perception_msgs::msg::DetectedObjects>::update(
  const double current_simulation_time,
  const std::vector<traffic_simulator_msgs::EntityStatus> & statuses,
  const rclcpp::Time & current_ros_time, const std::vector<std::string> & lidar_detected_entities)
  -> void
{
  if (
    current_simulation_time - previous_simulation_time_ - configuration_.update_duration() >=
    -0.002) {
    const std::vector<std::string> detected_objects = filterObjectsBySensorRange(
      statuses,
      configuration_.detect_all_objects_in_range() ? getDetectedObjects(statuses)
                                                   : lidar_detected_entities,
      configuration_.range());

    autoware_auto_perception_msgs::msg::DetectedObjects msg;
    msg.header.stamp = current_ros_time;
    msg.header.frame_id = "map";

    autoware_auto_perception_msgs::msg::TrackedObjects ground_truth_msg;
    ground_truth_msg.header = msg.header;

    previous_simulation_time_ = current_simulation_time;

    for (const auto & status : statuses) {
      if (
        std::find(detected_objects.begin(), detected_objects.end(), status.name()) !=
          detected_objects.end() and
        status.type().type() != traffic_simulator_msgs::EntityType_Enum::EntityType_Enum_EGO) {
        const auto object = make<autoware_auto_perception_msgs::msg::DetectedObject>(status);
        msg.objects.push_back(object);
        ground_truth_msg.objects.push_back(
          make<autoware_auto_perception_msgs::msg::TrackedObject>(status, object));
      }
    }

    static std::queue<std::pair<autoware_auto_perception_msgs::msg::DetectedObjects, double>>
      queue_objects;

    static std::queue<std::pair<autoware_auto_perception_msgs::msg::TrackedObjects, double>>
      queue_ground_truth_objects;

    queue_objects.emplace(msg, current_simulation_time);

    queue_ground_truth_objects.emplace(ground_truth_msg, current_simulation_time);

    autoware_auto_perception_msgs::msg::DetectedObjects delayed_msg;

    autoware_auto_perception_msgs::msg::TrackedObjects delayed_ground_truth_msg;

    if (
      current_simulation_time - queue_objects.front().second >=
      configuration_.object_recognition_delay()) {
      delayed_msg = queue_objects.front().first;
      delayed_ground_truth_msg = queue_ground_truth_objects.front().first;
      queue_objects.pop();
    }

    if (
      current_simulation_time - queue_ground_truth_objects.front().second >=
      configuration_.object_recognition_ground_truth_delay()) {
      delayed_ground_truth_msg = queue_ground_truth_objects.front().first;
      queue_ground_truth_objects.pop();
    }

    autoware_auto_perception_msgs::msg::DetectedObjects noised_msg;
    noised_msg.header = delayed_msg.header;
    noised_msg.objects.reserve(delayed_msg.objects.size());
    for (const auto & object : delayed_msg.objects) {
      if (auto probability_of_lost = std::uniform_real_distribution();
          probability_of_lost(random_engine_) > configuration_.probability_of_lost()) {
        noised_msg.objects.push_back(applyPositionNoise(object));
      }
    }

    publisher_ptr_->publish(noised_msg);

    static rclcpp::Publisher<autoware_auto_perception_msgs::msg::TrackedObjects>::SharedPtr
      ground_truth_publisher = std::dynamic_pointer_cast<
        rclcpp::Publisher<autoware_auto_perception_msgs::msg::TrackedObjects>>(
        ground_truth_publisher_base_ptr_);

    ground_truth_publisher->publish(delayed_ground_truth_msg);
  }
}
}  // namespace simple_sensor_simulator
