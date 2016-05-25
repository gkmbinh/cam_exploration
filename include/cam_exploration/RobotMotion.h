
/*
 * MIT License
 *
 * Copyright (c) 2016 Jordi Soler Busquets
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef CAM_EXPLORATION_ROBOT_MOTION_H
#define CAM_EXPLORATION_ROBOT_MOTION_H


/**
 * @file RobotMotion.h
 * @brief This file hosts the class RobotMotion
 * @author Jordi Soler
 * @version 1.0
 * @date 2016-04-21
 */



#include <ros/ros.h>

#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>

#include "geometry_msgs/PoseStamped.h"

#include "tf/transform_datatypes.h"
#include "tf/transform_listener.h"


namespace cam_exploration {

/**
 * @brief Object handling the robot motion.
 *
 * It interfaces with the move_base node and provides a simple interface for managing goals and geting information regarding the robot status
 *
 * Exaples:
 *
 * 1) Sending a goal to the robot:
 * @code
 *  // Initialization
 *  RobotMotion robot;
 *  ros::NodeHandle nh;
 *  robot.init();
 *
 *  // Sending a goal
 *  geometry_msgs::Pose goal;
 *  // --- Seteng proper values for goal --
 *  robot.goto(goal)
 * @endcode
 *
 * 2) Geting robot pose:
 *
 *    If the the initialisation has run, simply:
 * @code
 *  geometry_msgs::Pose robot_pose = RobotMotion::Pose()
 * @endcode
 */
class RobotMotion
{
public:
    RobotMotion() { initialised = false;}	///< Constructor. To be able to use this object, init() should be called once the caller has initialized the ros node
    void init();				///< Initial routine. Should be called once the caller node is initialised

    // Posse information
    static geometry_msgs::Pose pose() {return robot_pose;} 		 ///< Get robot pose @return Robot pose
    static geometry_msgs::Point position() {return robot_pose.position;} ///< Get robot position @return robot position

    // Goal management
    bool goTo(const geometry_msgs::Pose& goal);		///< Send a new goal to the robot @return True if the goal could be sent properly
    void cancelGoal(){ ac_->cancelGoal();}		///< Cancel the current robot goal
    static geometry_msgs::Pose current_goal(){ return current_goal_;}	///< Get current robot goal

    // Robot status interface
    static int status() {return robot_status;}				///< Gets current robot status. @return current robot status.
    static bool isMoving() { return robot_status == MOVING;}		///< Check if robot is moving. @return True if it is moving.
    bool refreshPose() { return refreshRobotPosition();}		///< Updates the robot pose. @return True if there was no error.

    // Possible robot_status
    static const int MOVING = 0;	///< The robot is moving towards the goal
    static const int SUCCEEDED = 1;	///< The robot reached the current goal
    static const int ERROR = 2;		///< Error state (e.g. the rest of the cases)

private:
    actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> * ac_;	///< Action client to interact with move_base
    static int number_of_goals_reached;						///< The amount of reached goals
    static int number_of_goals_sent;						///< The amount of sent goals
    static int robot_status;							///< Robot status. Can be MOVING, SUCCEDED or ERROR

    static geometry_msgs::Pose robot_pose;		///< Current robot position
    static geometry_msgs::Pose prev_robot_pose;		///< Previous (in the previous call to refreshRobotPosition() robot position
    static geometry_msgs::Pose current_goal_;		///< Current robot goal

    bool initialised;					///< True if RobotMotion has been initialised

    // Navigation functions
    /**
     * @brief Callback for move_base. Called when the robot stops chasing the goal for some reason.
     *
     * @param state Goal state
     * @param result move_base result. Unused
     */
    void move_baseDone(const actionlib::SimpleClientGoalState& state, const move_base_msgs::MoveBaseResultConstPtr& result);
    /**
     * @brief Callback for move_base. Called when move base is set to chase a goal
     */
    void move_baseActive();
    /**
     * @copydoc refreshPose()
     */
    bool refreshRobotPosition();

    tf::TransformListener * listener;			///< Transform listener for robot position
};



} // namespace cam_exploration


#endif
