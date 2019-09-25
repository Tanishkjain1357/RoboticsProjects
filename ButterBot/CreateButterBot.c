/**
* Code for ButterbotBABE of Winchester High School Botball Robotics Team
* This code was created for the Botball competition during 2017
*
* @team: Tanishk Jain, Ziyi Zheng, Andrew Sun, Natalie Daly, Roy Xing
*
* Sensors used: 1x ET sensor, 2x button sensors
* Actuators: 2x servos, 2x motors
* Meant for the Wallaby controller from KIPR (external battery type)
*
* Seeding type robot, meant to grab and stack 6 haybales in its onboard hopper
* and score in the barn area
*/

#include <kipr/botball.h>

#define ARM_PORT 3
#define CLAW_PORT 1
#define LEFT_MOTOR_PORT 3
#define RIGHT_MOTOR_PORT 1
#define ET_SENSOR_PORT 1
#define ARM_POS_DOWN 1800
#define ARM_POS_MID 600
#define ARM_POS_CUBE 0
#define CLAW_POS_OPEN 1100
#define CLAW_POS_CLOSE 1550  //Do not have the claw go over 1580
#define CLAW_POS_WALL 90

void setup(){
    printf("ButterbotBabe 2\n");
    enable_servos();
    set_servo_position(ARM_PORT, ARM_POS_CUBE);
    set_servo_position(CLAW_PORT, CLAW_POS_OPEN);
    printf("Setup Done\n");
}

void setDriveMotors(int left, int right){
    motor(LEFT_MOTOR_PORT, left);
    motor(RIGHT_MOTOR_PORT, right);
}

void interpolateServo(int port, int start, int end, int time, int interval){
    float currentTime = 0;
    while(currentTime < time){
        set_servo_position(port, start + (end - start) * (currentTime / time));
        currentTime += interval;
        msleep(interval);
    }
}


int values[] = {2900, 2850, 2900, 2900, 2900, 2900};

void pickUpCube(int index){
    set_servo_position(ARM_PORT, ARM_POS_DOWN);
    set_servo_position(CLAW_PORT, CLAW_POS_OPEN);
    msleep(1000);
    int time = 0;
    while(time < 400){
    	int sensorValue = analog_et(ET_SENSOR_PORT);
    	printf("ET Sensor: %d\n", sensorValue);
        if(sensorValue < values[index]){
            setDriveMotors(100, 100);
        	time = 0;
        }else{
            setDriveMotors(0, 0);
        	time += 2;
        }
        msleep(2);
    }
    setDriveMotors(0, 0);
        if(index == 1 || index == 2){
        //disable servos and then drop the cube
    }
    else{
    	interpolateServo(CLAW_PORT, CLAW_POS_OPEN, CLAW_POS_CLOSE, 200, 2);
    	setDriveMotors(-100, -100);
    	interpolateServo(ARM_PORT, ARM_POS_DOWN, ARM_POS_CUBE, 1500, 2);
    	setDriveMotors(0, 0);
    }
}

void drive (int leftPower, int rightPower, int time){
    motor (LEFT_MOTOR_PORT, leftPower);   // left power * .7
    motor (RIGHT_MOTOR_PORT, rightPower);
    msleep (time);
    motor (LEFT_MOTOR_PORT, 0);
    motor (RIGHT_MOTOR_PORT, 0);
}

void point_turn_left_90()
{
  motor(LEFT_MOTOR_PORT, -100);
  motor(RIGHT_MOTOR_PORT, 100);
  msleep(700);
}


void point_turn(int direction){
    int magnitude = abs(direction);
    direction = direction / magnitude;
	setDriveMotors(100 * direction, -100 * direction);
    msleep(700 * magnitude);
}

void left_point_turn_better()
{
	motor(LEFT_MOTOR_PORT, 80);
    motor(RIGHT_MOTOR_PORT, 70);
    msleep(250);
    motor(LEFT_MOTOR_PORT, -100);
    msleep(580);
  	motor(LEFT_MOTOR_PORT, -50);
    motor(RIGHT_MOTOR_PORT, -40);
  	msleep(4300);
    
    //motor(RIGHT_MOTOR_PORT, 50);
    //msleep(1000);
}

void pvc_follow()
{
  set_servo_position(ARM_PORT, ARM_POS_DOWN);
  msleep(200);
  set_servo_position(CLAW_PORT, CLAW_POS_WALL);
  msleep(200);
  int time = 0;
  int sensorValue = analog_et(ET_SENSOR_PORT);
  while(time < 15){
   	printf("ET Sensor: %d\n", sensorValue);
      if(sensorValue > 2000){
          while (sensorValue > 2000){
              motor(RIGHT_MOTOR_PORT, 0);
          	  motor(LEFT_MOTOR_PORT, 70);
              sensorValue = analog_et(ET_SENSOR_PORT);
          }
      }
      else if (sensorValue < 2000){
          while (sensorValue < 2000){
              motor(RIGHT_MOTOR_PORT, 70);
          	  motor(LEFT_MOTOR_PORT, 0);
              sensorValue = analog_et(ET_SENSOR_PORT);
          }
      }
      time++;
   }
    if((sensorValue < 1650) && (sensorValue > 1750)){
        setDriveMotors(0, 0);
    }
}

int main(){
    setup();
    shut_down_in(30);
    setDriveMotors(100, 100);
    interpolateServo(ARM_PORT, ARM_POS_CUBE, ARM_POS_MID, 3000, 2);
    setDriveMotors(-100, -100);
    msleep(1700);
    
    
    //setDriveMotors(100, -100);
    //msleep(100);
    setDriveMotors(100, 100);
    interpolateServo(ARM_PORT, ARM_POS_MID, ARM_POS_DOWN, 300, 3);
    msleep(1600);
    //setDriveMotors(-100, 100);
    //msleep(100);
    drive(100, 100, 200);
    
    setDriveMotors(100, 100);
    interpolateServo(CLAW_PORT, CLAW_POS_OPEN, CLAW_POS_CLOSE, 300, 2); //Change msleep for right side to 500
    interpolateServo(ARM_PORT, ARM_POS_DOWN, ARM_POS_CUBE, 1500, 2);
    disable_servo(ARM_PORT);
    msleep(50);
    setDriveMotors(0, 0);
    interpolateServo(CLAW_PORT, CLAW_POS_CLOSE, CLAW_POS_OPEN, 200, 2);
    //setDriveMotors(0, 100);
    msleep(800);
    enable_servo(ARM_PORT);
    msleep(50);
    msleep(600);
    //setDriveMotors(100, 100);
    /*setDriveMotors(0, 0);
    msleep(2000);
    //setDriveMotors(0, 0);
    point_turn(1);
    setDriveMotors(-100, -100);
    msleep(3000);

	setDriveMotors(100, 100);
    msleep(80);    
    //point_turn_left_90();
    left_point_turn_better();  */
    /*setDriveMotors(100, 100);
    msleep(700);
    interpolateServo(ARM_PORT, ARM_POS_CUBE, ARM_POS_DOWN, 1500, 2);
    msleep(100);
    interpolateServo(CLAW_PORT, CLAW_POS_OPEN, CLAW_POS_WALL, 200, 2);*/
    //pvc_follow();
    //msleep(400);
    setDriveMotors(100, 100);
    msleep(1500);
    /*int sensorValue = analog_et(ET_SENSOR_PORT);
    while(!((sensorValue < 1650) && (sensorValue > 1750))){
        setDriveMotors(-100, -100);
        msleep(2);
        sensorValue = analog_et(ET_SENSOR_PORT);
	}
    */
    //setDriveMotors(-100, -100);
    //msleep(200);

    
    //setDriveMotors(-100,-50);
    //msleep(300);
    
    //msleep(5000);
    //drive(-100, -100, 8000);
    /*
    setDriveMotors(-100, -100);
    msleep(3900);
    setDriveMotors(0, 0);
    setDriveMotors(100, 100);
    msleep(400);
    //point_turn_right();
    setDriveMotors(100, 100);
    msleep(800);
    */
    
    return 0;
}
