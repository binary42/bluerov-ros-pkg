/*
 * File: bluerov/src/navio_controller.cpp
 * Author: J. Neilan <jimbolysses@gmail.com>
 * Date: October 2015
 * Description: Sends thruster commands in microseconds to the Navio+ RPi
 *	sheild via the Navio+ API.
 */

#include "navio_controller.h"

static NavioController		*staticControllerObject;

NavioController::NavioController()
{
  p_interface 	= new NavioInterface();
  _cmdVelSub 	= _nodeHandle.subscribe<geometry_msgs::Twist>( "cmd_vel", 1, &NavioController::VelCallback, this );

  m_imuPub 	= _nodeHandle.advertise<sensor_msgs::Imu>( "imu_raw", 1 );
  m_gpsPub 	= _nodeHandle.advertise<sensor_msgs::NavSatFix>( "gps_status", 1 );

  //adcPub	= _nodeHandle.publish<sensor_ma -- create new message for adc --
  m_ahrsPub	= _nodeHandle.advertise<sensor_msgs::Imu>( "imu_fused", 1 );

  //baroPub	= _nodeHandle.publish< -- create new message for barometer --

}

NavioController::~NavioController()
{
	delete( p_interface );

	pthread_exit( nullptr);
}

void NavioController::InitNavioInterface()
{
  ROS_INFO( "Initializing Interface" );

  p_interface->Initialize();
  // 50Hz
  p_interface->SetFrequency( 50 );
}

void NavioController::Spin()
{
  if( !pthread_create( &_dataThread, nullptr, PublishNavioData, (void*)this ) )
  {
	  // 200Hz
	   ros::Rate loopRate( 200 );

	   while( ros::ok() && !m_isDone )
	   {
	     ros::spinOnce();

	     loopRate.sleep();
	   }
  }else
  {
	  ROS_INFO( ">>>> Error Creating Publisher Thread, Exiting <<<< " );
	  return;
  }
}

void NavioController::SetServo( int index, float value )
{
  int step = 3; // output 1 on navio, servo1 is channel 3, etc...

  // thruster values should be between 1100 and 1900 microseconds (us)
  // values less than 1500 us are backwards; values more than are forwards
  int pulseWidth = (value + 1) * 400 + 1100;

  p_interface->SendPWM( index + step, pulseWidth );
}

void NavioController::VelCallback (const geometry_msgs::Twist::ConstPtr &cmdVelIn )
{
  // Get velocity commands
  float roll     = cmdVelIn->angular.x;
  float pitch    = cmdVelIn->angular.y;

  float yaw      = cmdVelIn->angular.z;
  float forward  = cmdVelIn->linear.x;

  float strafe   = cmdVelIn->linear.y;
  float vertical = cmdVelIn->linear.z;

  // Build thruster commands (expected to be between -1 and 1)
  float thruster[5];

  // TODO - work out logic for 4 vectored and 1 vert thruster arrangement --

  thruster[0] = forward;

  //ROS_INFO( "%f", thruster[0] );

  for( size_t i = 0; i < 1; ++i )
  {
    SetServo( i, thruster[i] );
  }
}

void * NavioController::PublishNavioData( void *controllerIn )
{
  NavioController *controller = (NavioController*)controllerIn;

  sensor_msgs::Imu 				imuRawMessage;
  sensor_msgs::Imu 				imuaFusedMessage;
  sensor_msgs::NavSatFix		gpsMessage;

  std::vector<float>			data;

  // A few things horrid about this block. Imu raw used the float[9] of the 
  // ROS Imu message type. No raw IMU message exists, so we need to create one.
  // For now, using existing messages for testing.
  while( !controller->m_isDone )
  {
    // IMU 
    data = controller->p_interface->GetIMU();

    imuRawMessage.orientation.x 	= -1;
    imuRawMessage.linear_acceleration.x = -1;
    imuRawMessage.angular_velocity.x 	= -1;

    imuRawMessage.angular_velocity_covariance[0] = data[0];
    imuRawMessage.angular_velocity_covariance[1] = data[1];
    imuRawMessage.angular_velocity_covariance[2] = data[2];

    imuRawMessage.angular_velocity_covariance[3] = data[3];
    imuRawMessage.angular_velocity_covariance[4] = data[4];
    imuRawMessage.angular_velocity_covariance[5] = data[5];

    imuRawMessage.angular_velocity_covariance[6] = data[6];
    imuRawMessage.angular_velocity_covariance[7] = data[7];
    imuRawMessage.angular_velocity_covariance[8] = data[8];

    controller->m_imuPub.publish( imuRawMessage );

    // GPS
    //data = controller-GetGPS();

    // IMU fused
    //data = controller->GetAHRS();
  }

  return nullptr;
}

// Signal handling
void NavioController::StaticSignalHandler( int sigNumIn )
{
	staticControllerObject->SignalHandler( sigNumIn );
}
void NavioController::SignalHandler( int sigNumIn )
{
	ROS_INFO( ">>>> Caught Signal: %d Exiting <<<<", sigNumIn);

	m_isDone = true;
}

//------- Main -------------------------------
int main( int argc, char **argv )
{
  int sigret;

  ros::init( argc, argv, "navio_controller" );

  std::unique_ptr<NavioController> controller( new NavioController );

  controller->InitNavioInterface();

  ROS_INFO( "Navio+ Controller Online" );

  try
  {
	  controller->Spin();
  }
  catch( SignalException &except )
  {
	  sigret = EXIT_FAILURE;
	  return sigret;
  }


  ROS_INFO( "Shutting Down Navio+ Controller" );

  sigret = EXIT_SUCCESS;

  return sigret;
}
