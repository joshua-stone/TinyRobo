#include <sim_robots/sim_world.h>

SimWorld::SimWorld(ros::NodeHandle node)
{
	//Set up the publishers
	worldClock = node.advertise<std_msgs::Header>("sim_world_clock", 10);
	worldState = node.advertise<sensor_msgs::Image>("sim_world_view", 10);

	//Set up the size of the world. This is used for making images as well as coordinate stuff, so is in pixels
	worldSizeX = 640;
	worldSizeY = 480;

	//Get a parameter list of all the robots names
	node.param("robot_list", robotNames, std::vector<std::string>());
	if(robotNames.size() == 0)
	{
		ROS_ERROR("The world received no robots in the robot_list parameter.");
	}
	else
	{
		ROS_INFO("Subscribing to %lu robots", robotNames.size());
	}

	//Subscribe to all the robots
	for(std::vector<std::string>::iterator lit = robotNames.begin(); lit != robotNames.end(); lit++)
	{
		std::string topicName = "/" + *(lit) + "/pose";
		ROS_INFO("Subscribing to %s", topicName.c_str());
		ros::Subscriber tmp = node.subscribe(topicName, 1, &SimWorld::update, this);
		robotSubscriptions.push_back(tmp);
	}
}

void SimWorld::renderWorld()
{
	//Create an image of the specified world size
	Magick::Image blankWorld(Magick::Geometry(worldSizeX, worldSizeY), "white");

	//Publish the world image
	sensor_msgs::Image rosImg;
	rosImg.height = worldSizeY;
	rosImg.width = worldSizeX;
	rosImg.encoding = "rgba8";
	rosImg.step = 4 * worldSizeX; //R, G, B, A = 4, at 8 bits per channel, times width

	//Copy the ImageMagick image into the message
	//TODO There may be a faster way to do this...
	for(int ii = 0; ii < worldSizeX; ii++)
	{
		for(int jj = 0; jj < worldSizeY; jj++)
		{
			Magick::Color pColor = blankWorld.pixelColor(ii, jj);
			rosImg.data.push_back(pColor.redQuantum());
			rosImg.data.push_back(pColor.greenQuantum());
			rosImg.data.push_back(pColor.blueQuantum());
			rosImg.data.push_back(pColor.alphaQuantum());
		}
	}

	//Tell the world!
	worldState.publish(rosImg);
}

void SimWorld::step()
{
	/* Publish a clock message, which is just a ROS Header.
	 * The header automatically has a timestamp, which is what the sim robots
	 * need to update themselves.
	 */
	std_msgs::Header msg;
	msg.stamp = ros::Time::now();
	worldClock.publish(msg);
}

void SimWorld::update(const geometry_msgs::Pose)
{
	//For now, do nothing
	ROS_INFO("Got an update");
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "sim_world");
	ros::NodeHandle node("~");

	SimWorld world = SimWorld(node);

	ros::Rate r(100); //Update frequency for the world, in Hz
	int rateCounter = 0;
	while(ros::ok())
	{
		world.step();
		//Don't publish the world view at 100Hz
		rateCounter++;
		if(rateCounter > 30)
		{
			world.renderWorld();
			rateCounter = 0;
		}
		r.sleep();
	}

}
