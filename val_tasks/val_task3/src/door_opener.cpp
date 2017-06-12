#include "val_task3/door_opener.h"


doorOpener::doorOpener(ros::NodeHandle nh)
    :nh_(nh),armTraj_(nh_),gripper_(nh_), task3_(nh_), walker_(nh){

    robot_state_ = RobotStateInformer::getRobotStateInformer(nh_);
}

doorOpener::~doorOpener(){

}

void doorOpener::openDoor(geometry_msgs::Pose &valveCenterWorld){

    //Alligning relative to the centre of the valve
    geometry_msgs::Pose   pelvisPose;
    geometry_msgs::Pose2D preDoorOpenGoal;
    std::vector<float> x_offset, y_offset;
    geometry_msgs::Pose valveCenter;

    ROS_INFO("Valve centre world x: %f", valveCenterWorld.position.x);
    //Walking back a little

    bool result = robot_state_->transformPose(valveCenterWorld,valveCenter,
                                VAL_COMMON_NAMES::WORLD_TF,VAL_COMMON_NAMES::PELVIS_TF);
    ros::Duration(1.0).sleep();
    robot_state_->getCurrentPose(VAL_COMMON_NAMES::PELVIS_TF, pelvisPose);
    ROS_INFO_STREAM(" The result is:" << result);
    ROS_INFO("Valve centre prlvis x: %f", valveCenter.position.x);

    valveCenter.position.x  -= 0.85;
    valveCenter.position.y  -= 0.12;

    //Converting back to world
    robot_state_->transformPose(valveCenter, valveCenter, VAL_COMMON_NAMES::PELVIS_TF);

    preDoorOpenGoal.x        = valveCenter.position.x;
    preDoorOpenGoal.y        = valveCenter.position.y;
    preDoorOpenGoal.theta    = tf::getYaw(pelvisPose.orientation);

    ROS_INFO("openDoor: Walking back");
    walker_.walkToGoal(preDoorOpenGoal);

    //Opening hands
    ROS_INFO("openDoor: Openign arms");
    task3_.beforDoorOpenPose();

    gripper_.closeGripper(LEFT);
    gripper_.closeGripper(RIGHT);

    //Walking close to the door
    robot_state_->transformPose(valveCenterWorld,valveCenter,
                                VAL_COMMON_NAMES::WORLD_TF,VAL_COMMON_NAMES::PELVIS_TF);
    robot_state_->getCurrentPose(VAL_COMMON_NAMES::PELVIS_TF, pelvisPose);

    valveCenter.position.x  -= 0.35;

    //Converting back to world
    robot_state_->transformPose(valveCenter, valveCenter, VAL_COMMON_NAMES::PELVIS_TF);

    preDoorOpenGoal.x        = valveCenter.position.x;
    preDoorOpenGoal.y        = valveCenter.position.y;
    preDoorOpenGoal.theta    = tf::getYaw(pelvisPose.orientation);

    ROS_INFO("openDoor: Walking close to the door");
    walker_.walkToGoal(preDoorOpenGoal);

    ROS_INFO("Sleeping for 0.5 seconds");
    ros::Duration(0.5).sleep();

     //Walk straight few small steps
    ROS_INFO("Walking again");
    x_offset = {0.3,0.3,0.5,0.5};
    y_offset = {0.0,0.0,-0.1,-0.1};
    walker_.walkLocalPreComputedSteps(x_offset,y_offset,RIGHT);
    ros::Duration(1.0).sleep();

    ROS_INFO("Walking last step");
    x_offset = {0.3,0.3,0.5,0.5};
    y_offset = {0.0, 0.0, 0.0, 0.0};
    walker_.walkLocalPreComputedSteps(x_offset,y_offset,LEFT);

    // @todo spread left arm
    ros::Duration(1.0).sleep();

}
