#include "Agent.h"
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <sensor_msgs/LaserScan.h>
#include <sstream>
#include "math.h"
#include <cmath>
#include <stdlib.h>
#include "se306_project1/ResidentMsg.h"
#include "se306_project1/AssistantMsg.h"
#include <vector>

using namespace std;

/**
*	@brief Stops the agent from moving by setting flags and the variables that get sent to stage.
*/
void Agent::stopMoving(){
	isMoving = false;
	linear_x = 0;
	angular_z = 0;
	isFacingCorrectly = false;
}

/* -- Stage callbacks -- */

/**
 *	@brief Updates the agent's x position, y position, and angle to reflect its current pose.
 *	@param msg Odometry message from odom topic
 */
void Agent::StageOdom_callback(nav_msgs::Odometry msg) {

	//Converting from quaternion to radians
	currentAngle = acos(msg.pose.pose.orientation.w) * 2;
	if (msg.pose.pose.orientation.z > 0) {
		currentAngle = (2*M_PI)-acos(msg.pose.pose.orientation.w) * 2;
	}
	//Rounding to 3 decimal places
	currentAngle = ((int)(currentAngle * 1000 + .5) / 1000.0);


	//Update the current position
	px = msg.pose.pose.position.x;
	py = msg.pose.pose.position.y;
	if (isMoving == true){
		// move(originName); // Only correct if this doesn't get called
		move(originName);//TEMPORARY!
	}
}

/*
 * MOVE METHODS {START}
 */

/**
*	@brief Sets the path for the agent.
*	@param start The name of the start checkpoint
*	@param end The name of the end checkpoint
*/
void Agent::setPath(std::string start, std::string end) {
	shortestPath = g.shortestPath(start, end);
	shortestPathIndex = 0;
}

/**
*	@brief Moves the agent along a path to the specified goal.
*	@param goalName The name of a checkpoint (see CheckPointGraph.cpp)
*/
void Agent::move(std::string goalName){

	if (isMoving == false){
		isMoving = true;

		//Get the path stuff);

		std::string cname = g.getCheckpointName(currentCheckpoint);

		Agent::setPath(cname, goalName);

	}

	pair<double, double> nextCheckpoint = shortestPath.at(shortestPathIndex);
	if (currentCheckpoint.first == nextCheckpoint.first &&
			currentCheckpoint.second == nextCheckpoint.second){
		shortestPathIndex++;

		if (shortestPathIndex >= shortestPath.size() ){
			shortestPathIndex = 0;
			isMoving = false;
			return;
		}else{

			nextCheckpoint = shortestPath.at(shortestPathIndex);

			checkpointAngle = calculateGoalAngle(nextCheckpoint);
			isClockwise = isTurnClockwise();
		}

	}

	if (!isFacingCorrectly){
		turn();
	}else{
		moveForward(nextCheckpoint);
	}
}

/**
*	@brief Turns the agent to face the checkpoint, using the angular_z (angular velocity) set elsewhere. 
*	When facing the correct direction this function will set a flag to say as much.
*/
void Agent::turn(){
	//	if (a % 10 == 0){
	//		ROS_INFO("Goal Angle: %f", checkpointAngle * 180 / M_PI);
	//		ROS_INFO("Current Angle: %f", currentAngle * 180 / M_PI);
	//		ROS_INFO("Angle Difference, %f", (currentAngle - checkpointAngle) * 180 / M_PI);
	//	}

	angular_z = 1.5;
	double minAngularZ = 0.05;

	if (isClockwise){
		angular_z = angular_z * -1;
		minAngularZ = minAngularZ * -1;
	}


	double angleDifference = fabs(checkpointAngle - currentAngle);

	if (angleDifference > M_PI) {
		angleDifference = 2 * M_PI - angleDifference;
	}

	angular_z = minAngularZ + angular_z * cos(M_PI/2 - angleDifference);

	if (fabs(angleDifference) <= 0.02){
		angular_z = 0.0049;
		if (isTurnClockwise()){
			angular_z = angular_z * -1;
		}
		if (fabs(angleDifference) <= 0.005){
			angular_z = 0;
			isFacingCorrectly = true;
		}
	}
}

/**
*	@brief Moves the agent forwards until it is on the checkpoint.
*	This function uses a margin of error (0.5), within which the agent can be said to be facing the correct direction.
*/
void Agent::moveForward(pair<double,double> nextCheckpoint){

	linear_x = 5;
	double minLinearX = 1;

	double distanceFromCheckpoint = sqrt(pow((nextCheckpoint.first - px),2) + pow((nextCheckpoint.second - py),2));



	// Check to ensure that linear velocity doesn't decrease if the distance between the checkpoints is higher than 40.
	double distanceRatio = (distanceFromCheckpoint / 40);
	if (distanceRatio > 1){
		distanceRatio = 1;
	}

	linear_x = minLinearX + linear_x * sin (distanceRatio * M_PI /2);

	if (distanceFromCheckpoint <= 0.5){
		currentCheckpoint = nextCheckpoint;
		isFacingCorrectly = false;
		linear_x = 0;
	}

}

/**
*	@brief Decides whether to turn anticlockwise or clockwise, based on which angle is the smallest.
*	Used in turning to face the checkpoint.
*	@returns true if the agent should turn clockwise, and false if it should turn anticlockwise.
*/
bool Agent::isTurnClockwise(){

	double angleDifference = checkpointAngle - currentAngle;
	if (angleDifference > 0){
		if (fabs(angleDifference) < M_PI){
			return true;
		}
	}else{
		if (fabs(angleDifference) > M_PI){
			return true;
		}
	}

	return false;
}

/**
 *	This function calculates the angle to the goal.
 */
double Agent::calculateGoalAngle(pair<double, double> goalCheckpoint){

	//Finding the vector that the robot is facing and the goal vector
	double goalVectorX = goalCheckpoint.first - px;
	double goalVectorY = goalCheckpoint.second - py;


	double goalAngle = atan2(goalVectorY, goalVectorX); //pi <= goalAngle < -pi

	if (goalAngle < 0) {
		goalAngle = goalAngle * -1;
	} else if (goalAngle == 2 * M_PI) {
		goalAngle = 0;
	} else {
		goalAngle = 2 * M_PI - goalAngle;
	}

	//rounding goalAngle to three decimal places
	goalAngle = ((int)(goalAngle * 1000 + .5) / 1000.0);

	return goalAngle;
}

/*
 * MOVE METHODS {END}
 */
