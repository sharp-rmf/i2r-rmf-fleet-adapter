#include "i2r_driver/i2r_driver.hpp"

namespace i2r_driver {
      

std::string send_i2r_line_following_mission(rclcpp::Node* node, std::string& task_id,
    const std::vector<rmf_fleet_msgs::msg::Location>& path)
{
    int _task_id = std::move(std::stoi(task_id)); 
    std::vector <rmf_fleet_msgs::msg::Location> i2r_waypoint;
    RCLCPP_INFO(
        node->get_logger(), "---------------> Task id (line following): [%i]", task_id);
    
    for (const auto& location : path)
    {
        rmf_fleet_msgs::msg::Location _i2r_waypoint;
        transform_rmf_to_i2r(node, location, _i2r_waypoint);
        i2r_waypoint.emplace_back(_i2r_waypoint);
    }
    return mrccc_utils::mission_gen::line_following(_task_id, i2r_waypoint);
}

std::string send_i2r_docking_mission(rclcpp::Node* node, std::string task_id)
{

    RCLCPP_INFO(
        node->get_logger(), "---------------> Task id (docking): [%i]", task_id);
    return std::string("Some docking string for i2r");
}


void get_map_transfomation_param(rclcpp::Node* node, std::vector<double>& map_coordinate_transformation)
{
    node->get_parameter("map_coordinate_transformation", map_coordinate_transformation);
    
    /*
    RCLCPP_INFO(
    node->get_logger(), "map_coordinate_transformation: (%.3f, %.3f, %.3f, %.3f)",
        map_coordinate_transformation.at(0), map_coordinate_transformation.at(1),map_coordinate_transformation.at(2),map_coordinate_transformation.at(3));
    */
}

//transformation i2r-->rmf
  
void transform_i2r_to_rmf(
    rclcpp::Node* node,
    const rmf_fleet_msgs::msg::Location& _fleet_frame_location,
    rmf_fleet_msgs::msg::Location& _rmf_frame_location) 
{
    std::vector<double> map_coordinate_transformation;
    get_map_transfomation_param(node, map_coordinate_transformation);

    const Eigen::Vector2d translated =
        Eigen::Vector2d(_fleet_frame_location.x, _fleet_frame_location.y)
        - Eigen::Vector2d(
            map_coordinate_transformation.at(0), map_coordinate_transformation.at(1));

    const Eigen::Vector2d rotated =
        Eigen::Rotation2D<double>(-map_coordinate_transformation.at(2)) * translated;

    const Eigen::Vector2d scaled = 1.0 / map_coordinate_transformation.at(3) * rotated;
        
    _rmf_frame_location.x = scaled[0];
    _rmf_frame_location.y = scaled[1];
    _rmf_frame_location.yaw = 
        _fleet_frame_location.yaw - map_coordinate_transformation.at(2);

    _rmf_frame_location.t = _fleet_frame_location.t;
    _rmf_frame_location.level_name = _fleet_frame_location.level_name;
}

void transform_i2r_to_rmf(
    const std::vector<double>& map_coordinate_transformation,
    const rmf_fleet_msgs::msg::Location& _fleet_frame_location,
    rmf_fleet_msgs::msg::Location& _rmf_frame_location) 
{   
    const Eigen::Vector2d translated =
        Eigen::Vector2d(_fleet_frame_location.x, _fleet_frame_location.y)
        - Eigen::Vector2d(
            map_coordinate_transformation.at(0), map_coordinate_transformation.at(1));

    const Eigen::Vector2d rotated =
        Eigen::Rotation2D<double>(-map_coordinate_transformation.at(2)) * translated;

    const Eigen::Vector2d scaled = 1.0 / map_coordinate_transformation.at(3) * rotated;
        
    _rmf_frame_location.x = scaled[0];
    _rmf_frame_location.y = scaled[1];
    _rmf_frame_location.yaw = 
        _fleet_frame_location.yaw - map_coordinate_transformation.at(2) - 2 * M_PI;

    auto clamp_yaw = [&](double val) ->double
    {
        if ( val <= - M_PI )
        {
            val += 2*M_PI;
        }
        return val;
    }; 
    _rmf_frame_location.yaw  = clamp_yaw(_rmf_frame_location.yaw);

    _rmf_frame_location.t = _fleet_frame_location.t;
    _rmf_frame_location.level_name = _fleet_frame_location.level_name;
}
  
//transformation rmf-->i2r

void transform_rmf_to_i2r(
    rclcpp::Node* node,
    const rmf_fleet_msgs::msg::Location& _rmf_frame_location,   // RMF frame
    rmf_fleet_msgs::msg::Location& _fleet_frame_location)       // I2R Robot frame
{

    std::vector<double> map_coordinate_transformation;
    get_map_transfomation_param(node, map_coordinate_transformation);

    const Eigen::Vector2d scaled = 
        map_coordinate_transformation.at(3) * 
        Eigen::Vector2d(_rmf_frame_location.x, _rmf_frame_location.y);

    const Eigen::Vector2d rotated =
        Eigen::Rotation2D<double>(map_coordinate_transformation.at(2)) * scaled;

    const Eigen::Vector2d translated =
        rotated + 
        Eigen::Vector2d(
            map_coordinate_transformation.at(0), map_coordinate_transformation.at(1));

    _fleet_frame_location.x = translated[0];
    _fleet_frame_location.y = translated[1];
    _fleet_frame_location.yaw = 
        _rmf_frame_location.yaw + map_coordinate_transformation.at(2);

    auto clamp_yaw = [&](double val) ->double
    {
        if ( val <= - M_PI )
        {
            val += 2*M_PI;
        }
        return val;
    }; 
    _fleet_frame_location.yaw  = clamp_yaw(_fleet_frame_location.yaw);


    _fleet_frame_location.t = _rmf_frame_location.t;
    _fleet_frame_location.level_name = _rmf_frame_location.level_name;
}

tf2::Quaternion get_quat_from_yaw(double _yaw)
{
  tf2::Quaternion quat_tf;
  quat_tf.setRPY(0.0, 0.0, _yaw);
  quat_tf.normalize();

  return quat_tf;

}


} //namespace i2r driver