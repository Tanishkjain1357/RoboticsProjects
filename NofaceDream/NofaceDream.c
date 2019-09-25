#include <kipr/botball.h>

#define FRONTREFLECT   	0
#define ARM           	0  
#define CLAW 			1 
#define BIN 			2

#define ARM_UP 			300
#define ARM_DRIVE 		1400
#define ARM_BOTGUY 		1720
#define ARM_DUMP 		650
#define ARM_DOWN 		1889
#define CLAW_OPEN 		830
#define CLAW_SWEEP      665
#define CLAW_MID 		1360
#define CLAW_CLOSE 		1950
#define CLAW_CUBE       1650
#define CLAW_BOTGUY		1640		
#define BIN_COLLECT 	1400
#define BIN_SHAKE 		1150
#define BIN_DUMP 		350

#define REFLECTANCE 	0 //The line following sensor
#define LEVER_SENSOR 	3 // Sensing pipes to the left 
#define START_LIGHT 	5 // for the start using the light sensors and flashlight

// Analog sensors start from the left and not from the right (0 - bottom one is left)

void drive(int speed, int millis);
void turn(int speed, int millis);
void back_until_bump();
void drive_until_black();
void setup();
void deposit();
inline int over_black();
void setup()
{
  set_servo_position(CLAW, CLAW_OPEN);
  set_servo_position(BIN, BIN_COLLECT);  
  enable_servo(CLAW);
  enable_servo(BIN);
}

void drive(int speed, int millis)
{
  create_drive_direct(speed, speed);
  msleep(millis);
  create_drive_direct(0, 0);
}

void turn(int speed, int millis)
{
  create_drive_direct(speed,-speed);						
  msleep(millis);
  create_drive_direct(0,0);
}

inline void stop_create()
{
  create_drive_direct(0, 0);
}

void set_servo_position_stepping(int srv, int pos, int step_size)
{
  int cur_pos = get_servo_position(srv);
  int delta_remain = abs(pos - cur_pos);  
  int direction = pos > cur_pos ? 1 : -1;

  while(delta_remain >= 5)
  {
    //Make the final step fit so that the servo goes to its desired position
    if(delta_remain < step_size)
      step_size = delta_remain;

    set_servo_position(srv, cur_pos + direction * step_size);
    msleep(50);

    //Update the necessary variable
    cur_pos = get_servo_position(srv);
    delta_remain = abs(pos - cur_pos);
  }
}

void lift_and_dump_sequence()
{
  set_servo_position(ARM, ARM_UP);
  msleep(1100);  
  set_servo_position_stepping(CLAW, CLAW_OPEN,70);
  msleep(200);     
  set_servo_position_stepping(BIN, BIN_SHAKE, 30);
  msleep(300);
  set_servo_position(BIN, BIN_COLLECT);  
}
  
void grab_poms_sequence()
{
  set_servo_position(ARM, ARM_DOWN);
  msleep(100);
  set_servo_position_stepping(CLAW, CLAW_CLOSE, 100);
  
  thread pid = thread_create(lift_and_dump_sequence);
  thread_start(pid);
}

void deposit()
{
  create_drive_direct(100, 0);
  msleep(420);
  create_drive_direct(0, 0);
  set_servo_position_stepping(BIN, BIN_DUMP, 110);
  msleep(900);
  set_servo_position(BIN, BIN_COLLECT);
  set_servo_position_stepping(BIN, BIN_DUMP, 126);
  msleep(300);
  set_servo_position(BIN, BIN_COLLECT);
}
inline int over_black()
{
  printf("REFLECTANCE: %d %d\n", analog(REFLECTANCE), analog(REFLECTANCE) > 2000);
  return analog(REFLECTANCE) > 1000; //Changed by Damian from 700
}

void line_follow_fast(int nms)
{
  int t = 0, interval = 10;
  for(; t * interval < nms; t++)
  {
    if(!over_black())
      create_drive_direct(-150, -250); 
    else
      create_drive_direct(-250, -150); 

    msleep(interval);
  }

  stop_create();
}

void line_follow_millis(int nms){
  int t = 0, interval = 10;
  for(; t * interval < nms; t++){
    if(!over_black())
      create_drive_direct(-90, -150); 
    else
      create_drive_direct(-150, -90); 

    msleep(interval);
  }
  stop_create();
}

void turn90(int dir)
{
  turn(abs(dir)/dir*200, abs(dir)*1000);
}

void drive_until_black()
{
  while(!over_black())
    create_drive_direct(-200, -200);
  stop_create();
}

void turn_reflectance(int ontoline, int speed)
{
  while(1)
  {
    turn(speed, 10);
    if(ontoline)
    {
      if(over_black(REFLECTANCE))
      {
        break;
      }
    }
    if(!ontoline)
    {
      if(!over_black(REFLECTANCE))
      {
        break;
      }
    }
  }
}

void seeding()
{
  set_servo_position(ARM, ARM_DOWN);
  drive(-300, 1200); 
  grab_poms_sequence();
  set_servo_position(ARM, ARM_DRIVE);
  turn(-200, 10);
  drive(-200, 950);
  turn_reflectance(1, 250);
  turn_reflectance(0, -100);
  line_follow_millis(1200);
  
  //grab group 1
  turn(200, 260);
  drive(-200, 200);
  set_servo_position(ARM, ARM_DOWN); 
  drive(-200, 1016);
  grab_poms_sequence();
  msleep(200);
  
  //grab group 2
  turn(200, 290);
  drive(200, 600);
  turn(-200, 400);
  turn_reflectance(1, -250);
  turn(-200, 15);
  turn_reflectance(0, -250);
  turn(-200, 40);
  set_servo_position(ARM, ARM_DOWN);
  drive(-200, 1420);
  set_servo_position_stepping(CLAW, CLAW_MID, 90);
  set_servo_position_stepping(CLAW, 1000, 80);
  drive(-200, 200);
  set_servo_position_stepping(CLAW, CLAW_MID, 70);
  drive(-200, 200);
  grab_poms_sequence();
  
  //group 3
  turn(-100, 150);
  drive(200, 600);
  turn_reflectance(1, 250);
  line_follow_millis(830);
  turn(200, 300);
  set_servo_position(ARM, ARM_DOWN);
  msleep(500);
  set_servo_position(CLAW, CLAW_OPEN); 
  drive(-200, 1200);
  grab_poms_sequence();
  msleep(300);
  drive(200, 100);
  
  //group 4
  turn(-200, 270);
  while(!over_black (REFLECTANCE))
    create_drive_direct(-250, -250);
  drive(-200, 150);
  set_servo_position(ARM, ARM_DUMP);
  drive(-200, 500);
  turn(200, 700);
  turn_reflectance(1, 250);
  line_follow_millis(600);
  turn_reflectance(1, 50);
  line_follow_millis(600);
  drive(200, 1000);
  turn(-200, 135);
  set_servo_position(ARM, ARM_DOWN);
  drive(-200, 700);
  set_servo_position_stepping(CLAW, CLAW_MID, 90);
  set_servo_position_stepping(CLAW, 1000, 80);
  drive(-100, 300);
  set_servo_position_stepping(CLAW, CLAW_MID, 70);
  drive(-200, 300);
  grab_poms_sequence();
  
  //Deposit 1
  turn(-200, 80);
  back_until_bump();  
  turn(-200, 220);
  drive(-200, 800);
  turn_reflectance(1, 250);
  line_follow_fast(450);
  turn_reflectance(0, 100);
  line_follow_millis(600);
  drive(200, 1100);  
  turn(-200, 920);
  drive(-300, 500);
  set_servo_position(ARM, ARM_DUMP);
  create_drive_direct(-100, 0);
  msleep(200);
  create_drive_direct(0, 0);
  deposit();
  
  //Making way through the valley
  drive(200, 200);
  turn(200, 900);
  turn_reflectance(1, 200);
  turn_reflectance(0, -100);
  line_follow_fast(300);
  turn_reflectance(0, -100);
  turn_reflectance(1, 100);
  line_follow_fast(1050);
  set_servo_position(ARM, ARM_DUMP);
  drive(-200, 550);
  while(!over_black (REFLECTANCE))
    create_drive_direct(-250, -250);
  drive(-200, 600);
  set_servo_position(ARM, ARM_UP);
  turn(200, 750);
  set_servo_position(CLAW, CLAW_OPEN);
  msleep(300);
  
  turn_reflectance(1, 200);
  back_until_bump();  
  set_servo_position(ARM, ARM_DRIVE);
  msleep(400);
  line_follow_fast(520);
  
  //grab and move cube 1
  set_servo_position(ARM, ARM_DOWN);
  set_servo_position(CLAW, CLAW_OPEN);
  msleep(300);
  drive(-200, 70);
  set_servo_position(CLAW, CLAW_CUBE);
  msleep(500);
  set_servo_position(ARM, ARM_UP);
  msleep(1000);
  set_servo_position(CLAW, CLAW_OPEN);
  msleep(200);
  set_servo_position(ARM, ARM_DUMP);
  
  //botguy
  line_follow_fast(950);
  set_servo_position(ARM, ARM_BOTGUY);
  line_follow_fast(960);
  set_servo_position_stepping(CLAW, CLAW_BOTGUY, 80);
  
  //move cube 2
  line_follow_fast(1450);
  create_drive_direct(-50, -250);
  msleep(1500);
  create_drive_direct(0,0);
  set_servo_position(CLAW, CLAW_OPEN);
  set_servo_position(ARM, ARM_UP);
  msleep(700);
  create_drive_direct(50, 250);
  msleep(1300);
  create_drive_direct(0,0);
  set_servo_position(ARM, ARM_BOTGUY);
  line_follow_fast(900);
  set_servo_position(CLAW, CLAW_CLOSE);
  msleep(510);
  set_servo_position(ARM, ARM_DRIVE);
  line_follow_fast(1250);
  msleep(200);
  turn(200, 900);
  set_servo_position(CLAW, CLAW_OPEN);
  msleep(600);
  set_servo_position(ARM, ARM_UP);
  msleep(300);
  set_servo_position(ARM, ARM_DRIVE);

  //To the other side 
  turn(200, 900);
  back_until_bump();
  
  //dump first cube
  set_servo_position(ARM, ARM_DUMP);
  msleep(200);
  set_servo_position(BIN, BIN_DUMP);
  msleep(400);
  set_servo_position(BIN, BIN_COLLECT);
  msleep(100);
  set_servo_position(BIN, BIN_DUMP);
  msleep(200);
  set_servo_position(BIN, BIN_COLLECT);
  set_servo_position(ARM, ARM_UP);
  turn(200,150);
  
  //keep going
  turn_reflectance(1, 250);
  line_follow_millis(700);
  back_until_bump();
  drive(-300, 200);
  turn(200, 840);
  drive(-200, 920);
  turn_reflectance(1, 200);
  line_follow_millis(300);
  turn_reflectance(0, -100);
  turn_reflectance(1, 50);
  line_follow_millis(400);
  drive(200, 880);
  turn(200, 210);
  set_servo_position(ARM, ARM_DOWN);
  drive(-200, 460);
  set_servo_position_stepping(CLAW, CLAW_CLOSE,60);
  msleep(250);
  drive(200, 450);
  grab_poms_sequence();
  
  //next grp 1
  turn(-200, 200);
  line_follow_millis(2420);
  turn(-200, 590);
  set_servo_position(ARM, ARM_DOWN);
  drive(-200, 920);
  grab_poms_sequence();
  msleep(2000);
  drive(-200, 90);
  
  //next group 2
  turn(200, 90);
  set_servo_position(ARM, ARM_DOWN);
  drive(-200, 1000);
  drive(-200, 590);
  create_drive_direct(-150,-42);
  msleep(1000);
  create_drive_direct(0,0);
  grab_poms_sequence();
  create_drive_direct(-150,-20);
  msleep(500);
  create_drive_direct(0,0);
  line_follow_millis(1200);
  
  //next group 3
  drive(200, 700);
  turn(-200, 380);
  set_servo_position(ARM, ARM_DOWN);
  set_servo_position(CLAW, CLAW_OPEN);
  msleep(300);
  drive(-200, 700);
  grab_poms_sequence();
  drive(200, 720);
  
  //that one red pom
  turn_reflectance(1, 250);
  line_follow_fast(1000);
  set_servo_position(ARM, ARM_DOWN);
  set_servo_position(CLAW, CLAW_OPEN);
  line_follow_fast(800);
  msleep(300);
  grab_poms_sequence();
  
  //to deposit
  turn(-200, 1700);
  turn_reflectance(1, -250);
  turn_reflectance(0, -250);
  set_servo_position(ARM, ARM_DUMP);
  line_follow_fast(2420);
  turn(200, 145); 
  drive(-350, 1500);
  deposit();
  
  stop_create();
  return; //ADDED
}


int main(){
  create_connect();
  create_full();
  setup();

  wait_for_light(START_LIGHT);
  shut_down_in(119);
  msleep(6400); // wait for cubebot
  enable_servo(ARM);
  seeding();

  for(;;);
  return 0;
}