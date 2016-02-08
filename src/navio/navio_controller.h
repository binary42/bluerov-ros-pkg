/*
 * File: bluerov/src/navio_controller.h
 * Author: J. Neilan <jimbolysses@gmail.com>
 * Date: October 2015
 * Description: Sends thruster commands in microseconds to the Navio+ RPi
 *      sheild via the Navio+ API.
 */

#include <ros/ros.h>
#include <ros/console.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Joy.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/NavSatFix.h>
#include <std_msgs/Bool.h>
#include "navio_interface.cpp"
#include <pthread.h>

class NavioController
{
 public:
    NavioController();
    virtual ~NavioController();

    // Attributes
    NavioInterface                      *p_interface;
    ros::Publisher                      m_imuPub;
    ros::Publisher                      m_gpsPub;

    ros::Publisher                      m_adcPub;
    ros::Publisher                      m_ahrsPub;

    ros::Publisher                      m_baroPub;
    bool								m_isDone;

    // Methods
    void Spin();
    void InitNavioInterface();

 private:
    // Attributes
    ros::NodeHandle                     _nodeHandle;
    ros::Subscriber                     _cmdVelSub;

    pthread_t							_dataThread;

    // Methods
    void VelCallback( const geometry_msgs::Twist::ConstPtr &cmdVel );
    void SetServo( int index, float value );

    static void *PublishNavioData( void *controllerIn );

    // Signal handling
	void SignalHandler( int sigNumIn );
	static void StaticSignalHandler( int sigNumIn );
};

