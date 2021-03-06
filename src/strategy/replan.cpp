
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

#include <cam_exploration/replan.h>
#include <cam_exploration/MapServer.h>


/**
 * @file replan.cpp
 * @brief Implementation of the replan.h file
 * @author Jordi Soler
 * @version 1. 0
 *@date 2016-04-21
 */


using namespace cam_exploration;
using namespace cam_exploration::replan;
using namespace std;


/*-----------------------------------------------------------------------------
  *
  *	TOO MUCH TIME NEAR GOAL
  *
  -----------------------------------------------------------------------------*/
TooMuchTimeNearGoal::TooMuchTimeNearGoal(std::map<std::string, std::string> params)
{
    ROS_INFO("TooMuchTimeNearGoal: Number of parameters: %lu", params.size());
    orientation_th = 0.5;
    for(std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it){
	std::string param = it->first, value = it->second;
	ROS_INFO("TooMuchTimeNearGoal: Parameter: %s set to value %s", param.c_str(), value.c_str());

	if (param == "time_threshold"){
	    float th = std::atof(value.c_str());
	    time_th_ = ros::Duration(th);
	}
	else if(param == "distance_threshold"){
	    distance_th_ = std::atof(value.c_str());
	}
	else if(param == "orientation_threshold"){
	    orientation_th = atof(value.c_str());
	}
	else{
	    ROS_ERROR("String %s does not match any parameter in 'TooMuchTimeNearGoal'", param.c_str());
	}

    }
    prev = ros::Time::now();
    near_goal = isNearGoal();
}

bool TooMuchTimeNearGoal::replan()
{
    bool near_goal_now = isNearGoal();
    if (!near_goal && near_goal_now)
    	prev = ros::Time::now();

    near_goal = near_goal_now;


    return near_goal_now ? ros::Time::now() - prev > time_th_  && isOriented() : false;
}


bool TooMuchTimeNearGoal::isOriented()
{
    geometry_msgs::Quaternion robot_orintation = RobotMotion::pose().orientation;
    geometry_msgs::Quaternion current_goal = RobotMotion::current_goal().orientation;

    tf::Quaternion quat_robot, quat_goal;
    tf::quaternionMsgToTF(robot_orintation, quat_robot);
    tf::quaternionMsgToTF(current_goal, quat_goal);

    tfScalar diff = M_PI - quat_robot.angle(quat_goal);

    //ROS_INFO("Diff: %f. Threshold: %f. Is oriented? %d", diff, orientation_th, diff < orientation_th);
    return diff < orientation_th;
}


bool TooMuchTimeNearGoal::isNearGoal()
{
    geometry_msgs::Point robot_position = RobotMotion::position();
    geometry_msgs::Point current_goal = RobotMotion::current_goal().position;

    double robot_distance = hypot(robot_position.x-current_goal.x, robot_position.y-current_goal.y);

    return robot_distance < distance_th_;
}



/*-----------------------------------------------------------------------------
  *
  *	THE GOAL IS NOT CLOSE TO ANY FRONTIER
  *
  -----------------------------------------------------------------------------*/
IsolatedGoal::IsolatedGoal(std::map<std::string, std::string> params)
{
    ROS_INFO("IsolatedGoal: Number of parameters: %lu", params.size());
    depth_ = 5;
    for(std::map<std::string, std::string>::iterator it = params.begin(); it != params.end(); ++it){
	std::string param = it->first, value = it->second;
	ROS_INFO("IsolatedGoal: Parameter: %s set to value %s", param.c_str(), value.c_str());

	if (param == "depth"){
	    depth_ = atoi(value.c_str());
	}
	else{
	    ROS_ERROR("String %s does not match any parameter in 'IsolatedGoal'", param.c_str());
	}

    }

    // Publish goal neighbourhood
    MarkerPublisher mpublisher;
    std_msgs::ColorRGBA color;
    color.r = 0.8; color.g = 0.2; color.b = 0.0; color.a = 1.0;

    mpublisher.add("goal_neighbour_cells", "goal_neighbour_cells");
    mpublisher.setProperty("goal_neighbour_cells", color);
}

bool IsolatedGoal::replan()
{
    return !isNearFrontier();
}


bool IsolatedGoal::isNearFrontier()
{
    geometry_msgs::Point current_goal = RobotMotion::current_frontier_target_point();

    MapServer maps;
    vector<int> neighs = maps.getNeighbours(current_goal, depth_);

    MarkerPublisher mpublisher;
    mpublisher.publish("goal_neighbour_cells", neighs);

    for (vector<int>::iterator it = neighs.begin(); it != neighs.end(); ++it){
    	if (maps.isFrontierCell(*it))
    	    return true;
    }

    return false;
}




void Replaner::addCause(std::string name, std::map<std::string, std::string> parameters)
{
    ROS_INFO("Replaner: Adding cause %s", name.c_str());
    if (name == "not_moving")
    {
	replaning_causes.push_back(new NotMoving());
    }
    else if (name == "too_much_time_near_goal")
    {
        replaning_causes.push_back(new TooMuchTimeNearGoal(parameters));
    }
    else if (name == "isolated_goal")
    {
        replaning_causes.push_back(new IsolatedGoal(parameters));
    }
    else
    {
    	ROS_ERROR("String %s does not match any replaning condition.", name.c_str());
    }
}

bool Replaner::replan()
{
    bool replan = false;
    for (vector<replaningCause*>::iterator it = replaning_causes.begin(); it != replaning_causes.end(); ++it){
    	if(verbosity > 0 && (**it).replan())
    	    ROS_INFO("    Replaning because: %s", (**it).name());

    	replan = replan || (**it).replan();
    }
    return replan;
}


