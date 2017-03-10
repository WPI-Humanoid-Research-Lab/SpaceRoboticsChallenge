#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/robot_state/robot_state.h>
#include <moveit/move_group_interface/move_group.h>
#include <moveit/planning_scene_interface/planning_scene_interface.h>
#include <moveit/planning_scene_monitor/planning_scene_monitor.h>
#include <moveit_msgs/DisplayRobotState.h>
#include <moveit_msgs/DisplayTrajectory.h>
#include <val_control/val_arm_navigation.h>
#include <ros/ros.h>


int main(int argc, char** argv){

  ros::init(argc, argv, "test_moveit");
  ros::NodeHandle node_handle;

  /**************************************
     * ALWAYS START SPINNER IF USING MOVEIT
     *************************************/
  ros::AsyncSpinner spinner(1);
  spinner.start();

  moveit::planning_interface::MoveGroup group("rightShoulderPalm");

  // set the start state to the current state of the robot
  group.setStartStateToCurrentState();

  // get the cuurent joints and their positions
  std::vector<std::string> jNames;
  jNames = group.getActiveJoints();
  std::vector<double> jValues;
  jValues = group.getCurrentJointValues();
  std::vector<std::string>::iterator it;
  std::vector<double>::iterator itd;
  ROS_INFO("current state");
  for (it=jNames.begin(), itd=jValues.begin(); it<jNames.end(); it++, itd++){
    std::cout << *it << ": " << *itd << std::endl;
  }

  ROS_INFO("Reference frame: %s", group.getPlanningFrame().c_str());
  ROS_INFO("Reference frame: %s", group.getEndEffectorLink().c_str());

  /**********************************************************************
     * These Parameters will alter the behaviour significantly
     *
     * RTT gives promising results
     * Planning attempts should be one
     * Planning time depends on the gola location (7 should be enough)
     * Goal tolerance is required as we dont have a defined orientation
     **********************************************************************/
  group.setPlannerId("RRTkConfigDefault");
  group.setNumPlanningAttempts(1);
  group.setPlanningTime(7);
  group.setGoalTolerance(0.1);

  // set the target location
  geometry_msgs::Pose t_pose_1;
  moveit::planning_interface::MoveGroup::Plan my_plan;

  // arm trajectory object
  armTrajectory armTraj(node_handle);

  // set first pose point
  t_pose_1.orientation.w = 1.0;
  t_pose_1.position.x = 0.28;
  t_pose_1.position.y = -0.7;
  t_pose_1.position.z = 1.0;
  group.setPoseTarget(t_pose_1);
  group.setGoalTolerance(0.1);

  if (group.plan((my_plan)))
  {
    std::cout<<"sucessfully planned the trajectory"<<std::endl;
    std::vector<trajectory_msgs::JointTrajectoryPoint>::iterator itp;

    std::cout << "joint names" << std::endl;
    for (it = my_plan.trajectory_.joint_trajectory.joint_names.begin(); it < my_plan.trajectory_.joint_trajectory.joint_names.end(); it++)
      std::cout << *it << std::endl;
    std::cout << "joint trajectories" << std::endl;
    for (itp = my_plan.trajectory_.joint_trajectory.points.begin(); itp < my_plan.trajectory_.joint_trajectory.points.end(); itp++)
      std::cout << *itp << std::endl;

    /* Sleep to give Rviz time to visualize the plan. */
    sleep(2.0);
    ROS_INFO("executing on robot\n");

    armTraj.moveArmTrajectory(RIGHT, my_plan.trajectory_.joint_trajectory);
  }
  else
  {
    ROS_INFO("planning failed");
  }

  // sleep for 20 sec before executing next point
  sleep(20.0);
  ROS_INFO("planning for second point");

  // set second target location
  t_pose_1.orientation.w = 1.0;
  t_pose_1.position.x = 0.28;
  t_pose_1.position.y = -1.0;
  t_pose_1.position.z = 1.3;
  group.setPoseTarget(t_pose_1);

  group.setPlanningTime(10);
  if (group.plan((my_plan)))
  {

    /* Sleep to give Rviz time to visualize the plan. */
    sleep(2.0);
    ROS_INFO("executing on robot\n");
    armTrajectory armTraj(node_handle);
    armTraj.moveArmTrajectory(RIGHT, my_plan.trajectory_.joint_trajectory);
  }
  else
  {
    ROS_INFO("planning failed");
  }


  ros::spin();

}
