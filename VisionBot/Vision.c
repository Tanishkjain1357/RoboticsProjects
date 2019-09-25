#include <kipr/botball.h>

#define ARM				1
#define CLAW            2
#define LEFT_WHEEL		1 //black down (toward claw) //plastic motor
#define RIGHT_WHEEL		0 //red down (toward claw)

#define CLAW_OPEN       350
#define CLAW_B_CLOSE    1200
#define CLAW_C_CLOSE    1000 //OG 830

#define TOPHAT_FRONT 0 //Change to large top hat
#define TOPHAT_BACK 1

#define YELLOW_ZONE		3
#define RED_ZONE		1
#define GREEN_ZONE		2

#define ARM_BASE		310
#define ARM_UP			550
#define ARM_STARTING	1250

#define DEFAULT_INTERVAL 10

#define TRUE 1
#define FALSE 0

#define LIGHT 5


//right box 17.5 in
//left box 10 in

int zones[] = {0, 0, 0};
int state = 1;

int threshold = 2600; // Greater means on black
void checkCamera();
void checkStatus();
void warmupCamera();
int updateCamera();
void line_follow_reverse_left();
void line_follow_right();
void line_follow_left();
void line_follow_reverse_right();
void line_follow_left_adjustable(int nms, int speed, float turn_ratio);

void setup(){
    enable_servos();
    set_servo_position(CLAW, CLAW_C_CLOSE);
    set_servo_position(ARM, ARM_STARTING);
    checkStatus(camera_open_black(), "Failure to Open Camera -> ");
    checkCamera();
    msleep(200);
    warmupCamera(1500,100);
}

void stop(){
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
}

void drive(int left, int right, int time){

    if(right < 0){ //going backwards
        mav(LEFT_WHEEL, left); //80
        mav(RIGHT_WHEEL, right *.8);
    }
    else{
        mav(LEFT_WHEEL, left); //75
        mav(RIGHT_WHEEL, right*.81);
    }
    msleep(time);
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
    msleep(10);
}

void drive_test(){
    drive(1500, 1500, 4000);
    msleep(2000);
    drive(-1500, -1500, 4000);
}

void drive_until_black(int port, int speed){
    printf("driving until black\n");
    while(analog(port) < threshold){
        mav(LEFT_WHEEL, speed);
        mav(RIGHT_WHEEL, speed);
        //printf("%d\n", analog(port));
    }
    stop();
}

void drive_until_white(int port, int speed){
    printf("driving until white\n");
    while(analog(port) > threshold){
        mav(LEFT_WHEEL, speed);
        mav(RIGHT_WHEEL, speed*.98);
    }
    stop();
}

void drive_until_color(int channel, int obj_size, int speed){ 
    //printf("driving_until_color");
    checkStatus(updateCamera(), "Failure to Update Camera\n");
    printf("%d\n", get_object_area(channel,0));
    while(get_object_area(channel,0) < obj_size){
        printf("Size:%d\n", get_object_area(channel,0));
        if(speed > 0){
            //drive(1500,1500,30);
            line_follow_left_adjustable(10,1500,0.5);
        }
        else{
            //drive(-1500,-1500,30);
            line_follow_reverse_right(10);
        }
        //drive_until_color(drive(speed,speed,10);
        checkStatus(updateCamera(), "Failure to Update Camera\n");
    }
}

void drive_until_color_centered(int channel, int speed){ //camera resolution is 160x120
    printf("driving until centered");
    checkStatus(updateCamera(), "Failure to Update Camera\n");
    int x_val = 0;
    while(get_object_area(channel,0) < 500 || (x_val > 50 && x_val < 40)){
        if(speed > 0){
            drive(1500,1500,30);
            //line_follow_left(1500, 50);
        }
        else{
            line_follow_reverse_right(10);
        }
        checkStatus(updateCamera(), "Failure to Update Camera\n");
        x_val = get_object_center(channel,0).x;
        printf("Center:%d\n", x_val);
    }
}
void drive_until_color_centered_all(int speed){ //camera resolution is 160x120
    checkStatus(updateCamera(), "Failure to Update Camera\n");
    while((get_object_area(0,0) < 200 && get_object_center(0,0).x > 50 && get_object_center(0,0).x < 40) || (get_object_area(1,0) < 200 && get_object_center(1,0).x > 50 && get_object_center(1,0).x < 40) || (get_object_area(2,0) < 200 && get_object_center(2,0).x > 50 && get_object_center(2,0).x < 40)){
        if(speed > 0){
            line_follow_right(50);
        }
        else{
            line_follow_reverse_left(50);
        }
        checkStatus(updateCamera(), "Failure to Update Camera\n");
    }
}

void line_up_above_line(int r_dir, int speed){ //input the direction of the right wheel
    printf("started lineup\n");
    int r_drive = r_dir * speed;
    int l_drive = speed * r_dir * -1;
    while(analog(TOPHAT_FRONT) < threshold || analog(TOPHAT_BACK) < threshold){
        //printf("%d, %d, %d, %d\n", analog(TOPHAT_FRONT), analog(TOPHAT_BACK), r_drive, l_drive);
        r_drive = (analog(TOPHAT_FRONT) > threshold) ? 0 : r_dir * speed;
        l_drive = (analog(TOPHAT_BACK) > threshold) ? 0 : speed * r_dir * -1;
        mav(RIGHT_WHEEL, r_drive);
        mav(LEFT_WHEEL, l_drive);
        msleep(5);
    }
}

void interpolate(int port, int pos, int time){
    //printf("interpolating to %d\n", pos);
    double currentTime = 0;
    int starting_pos = get_servo_position(port);
    while(currentTime < time){
        set_servo_position(port, starting_pos + (pos - starting_pos) * currentTime/time);
        currentTime += DEFAULT_INTERVAL;
        msleep(DEFAULT_INTERVAL);
    }    
}

void turn_left(int speed, int time){
    mav(LEFT_WHEEL, -speed);
    mav(RIGHT_WHEEL, speed);
    msleep(time);
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
    msleep(10);
}

void turn_right(int speed, int time){
    mav(LEFT_WHEEL, speed);
    mav(RIGHT_WHEEL, -speed);
    msleep(time);
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
    msleep(10);
}

//turn_ratio is how slow you want the slow wheel to turn, aka the smaller the turn_ratio, the sharper the turn
void line_follow_left_adjustable(int nms, int speed, float turn_ratio){
    int t = 0, interval = 5;
    while(t * interval < nms) {
        if(analog(TOPHAT_FRONT) > threshold) {
            mav(LEFT_WHEEL, speed * turn_ratio * 0.7);
            mav(RIGHT_WHEEL, speed );
            t++;
        }
        else {
            mav(RIGHT_WHEEL, speed * turn_ratio);
            mav(LEFT_WHEEL, speed);
            t++;
        }
        msleep(interval);
    }
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
}

void line_follow_left(int nms) {
    line_follow_left_adjustable(nms, 1200, 0.67);
}


void line_follow_right(int speed, int nms)
{
    int t = 0, interval = 10;
    while(t * interval < nms) {
        if(analog(TOPHAT_FRONT) > threshold) {
            mav(LEFT_WHEEL, speed);
            mav(RIGHT_WHEEL, speed * 0.5 * 0.7); //CHANGE TURN RATIO
            t++;
        }
        else {
            mav(RIGHT_WHEEL, speed * 0.85);
            mav(LEFT_WHEEL, speed * 0.85 * 0.5);
            t++;
        }
        msleep(interval);
    }
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
}

void line_follow_reverse_right(int nms)
{
    int t = 0, interval = 5;
    while(t * interval < nms) {
        if(analog(TOPHAT_BACK) > threshold) {
            mav(LEFT_WHEEL, -900 * 0.7);
            mav(RIGHT_WHEEL, -1500);
            t++;
        }
        else {
            mav(RIGHT_WHEEL, -900*.9);
            mav(LEFT_WHEEL, -1500);
            t++;
        }
        msleep(interval);
    }
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
}

void line_follow_reverse_left(int nms)
{
    int t = 0, interval = 10;
    while(t * interval < nms) {
        if(analog(TOPHAT_BACK) > threshold) {
            mav(LEFT_WHEEL, -1500);
            mav(RIGHT_WHEEL, -900);
            t++;
        }
        else {
            mav(RIGHT_WHEEL, -1500);
            mav(LEFT_WHEEL, -900);
            t++;
        }
        msleep(interval);
    }
    mav(LEFT_WHEEL, 0);
    mav(RIGHT_WHEEL, 0);
}

void starting_box(){

    int left;
    int right;
    while((left = left_button()) == FALSE && (right = right_button()) == FALSE){
        msleep(50);
    }
    state = left == TRUE ? 1 : 0;
    printf("State: %d\n", state);
}

void cube(){
    interpolate(CLAW, CLAW_C_CLOSE, 1000);
    msleep(250);
    interpolate(ARM, ARM_UP - 10, 1000);
    if(zones[YELLOW_ZONE] == 1){
        drive(-1500,-1500,1600);
        interpolate(ARM, ARM_UP + 50, 100);
        turn_right(1500, 800);
        drive(-1500, -1500, 1500);
        turn_left(1500, 850);
    }
    else{
    	drive(-1500, -1500, 2400);
        interpolate(ARM, ARM_UP + 50, 100);
    }

    /*
    turn_left(1200, 400);  //drop cubes
    drive(1500, 1500, 1300);
    set_servo_position(ARM, ARM_BASE-20);
    msleep(50);
    set_servo_position(CLAW, CLAW_OPEN-100);
    msleep(500);
    drive(-1500, -1500, 1550);
    turn_right(1500, 300);
    */
}

void cubeDump(){

    drive_until_black(TOPHAT_BACK, -1500);
    if(zones[YELLOW_ZONE] == 0){
        drive(1500,-1500,700);
        line_follow_reverse_right(2300);
        msleep(2000);
        drive_until_color(YELLOW_ZONE, 1000, -1500);
        drive_until_color_centered(YELLOW_ZONE, -1500);
        msleep(100);
        
        drive(-1500,-1500,400);
        interpolate(ARM, ARM_BASE + 150, 500);
        drive(1500,-1500,350);
        interpolate(ARM, ARM_BASE, 100);
        interpolate(CLAW, CLAW_OPEN, 200);
        interpolate(ARM, ARM_UP + 250, 800);

        drive(1200, 1200, 200);
        interpolate(CLAW, CLAW_C_CLOSE, 1000);
        msleep(250);
        drive(-1200, -1200, 200);
        interpolate(ARM, ARM_UP + 400, 500); //destack
        msleep(400);
        drive(900, 900, 700);
        drive(-900, -900, 700);
        drive(-1500, -1500, 900);

        //ensure safety of cubes
        /*
        set_servo_position(ARM, ARM_BASE + 50);
        msleep(400);
        turn_left(1500, 200);
        drive(900,900,600);
        set_servo_position(ARM, ARM_UP);        
        drive(-900,-900,600);
        */
        turn_right(1500, 500);
        drive(-1500,-1500,200);
        set_servo_position(ARM, ARM_BASE);
        msleep(500);
        drive(1500, 1500, 200);
        set_servo_position(CLAW, CLAW_OPEN);    ///DROP CUBE 2
        msleep(200);
        drive(-1500, -1500, 100);
        msleep(100);
        drive(1500, 1500, 200);
        interpolate(ARM, ARM_STARTING, 800);
        msleep(100); //end w/ krakatoa
        
        //NORMAL
        turn_left(1500,920);
        drive(1500, 1500, 4600);
        turn_left(1200, 550);
        drive_until_black(TOPHAT_FRONT, 1500);
        drive(1500, 1500, 500);
        set_servo_position(ARM, ARM_BASE); 
    }
    else if(zones[YELLOW_ZONE] == 2){
        drive(-1500,-1500,400);
        drive(1500,-1500,600);
        line_follow_left(3200);
        
        msleep(500);
        drive_until_color(YELLOW_ZONE, 3000, 1500);
        drive_until_color_centered(YELLOW_ZONE, 1500);
        interpolate(ARM, ARM_BASE + 150, 500);
        //drive(1500,1500,200);
        drive(1500,-1500,400);
        interpolate(ARM, ARM_BASE, 100);
        interpolate(CLAW, CLAW_OPEN, 200);
        interpolate(ARM, ARM_UP + 170, 800);
        interpolate(CLAW, CLAW_C_CLOSE, 1000);
        msleep(250);
        interpolate(ARM, ARM_UP + 400, 500);

        msleep(400);
        drive(-1500, -1500, 900);

        //ensure safety of cubes
        set_servo_position(ARM, ARM_BASE + 100);
        msleep(400);
        turn_left(1500, 200);
        drive(900,900,700);
        set_servo_position(ARM, ARM_UP);
        drive(-1100,-1100,600);
        turn_right(1500, 200);

        drive(1500, -1500, 300);
        drive(-1500,-1500,200);
        interpolate(ARM, ARM_BASE + 150, 500);
        drive(1500, 1500, 300);
        interpolate(ARM, ARM_BASE, 100);
        set_servo_position(CLAW, CLAW_OPEN); 
        drive(1200,1200,200);
        drive(-1500,-1500,600); 
        interpolate(ARM, ARM_STARTING, 200);
        
        drive(1500,-1500,780);
        drive(1500,1500,2000);
        turn_right(1300,800);
        drive_until_black(TOPHAT_FRONT, 1500);
        drive(1500, 1500, 500);
        set_servo_position(ARM, ARM_BASE); 
    }
    else{ //zone 1
        drive(-1500,-1500,400);
        drive(1500,-1500,600);
        line_follow_left(450);
        drive_until_color(YELLOW_ZONE, 3000, 1500);
        drive_until_color_centered(YELLOW_ZONE, 1500);
        drive(1500, 1500, 400);
        //drive(-1500,-1500,300);
        //turn to dump cube
        //drive(1100,-1100,700);
        //drive(1500,1500,300);
        interpolate(ARM, ARM_BASE + 120, 500);
        drive(1500,-1500,400); 
        interpolate(ARM, ARM_BASE,500);
        interpolate(CLAW, CLAW_OPEN,200);
        interpolate(ARM, ARM_UP + 200, 800);
        interpolate(CLAW, CLAW_C_CLOSE, 1000);
        msleep(250);
        interpolate(ARM, ARM_UP + 400, 500); //destack

        msleep(400);
        drive(-1500, -1500, 1000);

        //ensure safety of cubes
        set_servo_position(ARM, ARM_UP + 100);
        msleep(400);
        turn_left(1500, 180);
        drive(1100,1100,700);
        msleep(400);
        drive(-1100,-1100,250);
        interpolate(ARM, ARM_STARTING, 200);
        drive(-1100,-1100,450);
        turn_right(1500, 180);

        drive(1500, -1500, 300);
        drive(-1500,-1500,200);
        interpolate(ARM, ARM_STARTING, 200);
        drive(1500, 1500, 200);
        interpolate(ARM, ARM_BASE, 500);
        interpolate(CLAW, CLAW_OPEN, 500); 
        drive(1200,1200,400);
        drive(-1500,-1500,850); 
        interpolate(ARM, ARM_STARTING, 200);

        //final zone 1 "Go to botguy"
        drive_until_black(TOPHAT_BACK, -1500);
        drive(-1500, -1500, 300);
        turn_left(1500, 700);
        line_follow_right(1500, 1200);
        turn_left(1500, 700);
        
        /*
        drive(-1500,1500,700);
        drive(1500,1500,1200);
        drive(-1500,1500,540);

        drive_until_black(TOPHAT_FRONT, 1500);
        drive(1500, 1500, 500);
        */
        
        set_servo_position(ARM, ARM_BASE); 
    }
}

void botguy(){
    set_servo_position(CLAW, CLAW_B_CLOSE);
    msleep(200);
    //CHANGE FOR GCER  --> 1100, 1300
    line_follow_right(1500, 1800);
    set_servo_position(CLAW, CLAW_OPEN + 100);
    msleep(200);
    line_follow_right(1500, 700);
    drive(1500, 1500, 600);
    interpolate(CLAW, CLAW_B_CLOSE, 1000);
    msleep(400);
    interpolate(ARM, ARM_UP + 150, 600);
    drive(1500,-1500,150);
    line_follow_reverse_right(2700);
    interpolate(ARM, ARM_STARTING + 100, 800);
    msleep(300);
    printf("Got botguy");
}

void checkStatus(int status, char failureMessage[]){
    if (status == 0) {
        printf(failureMessage);
    }
}

int getRelativePosition(int channel, int threshold){
    int count = get_object_count(channel);
    int totalPosition = 0;
    int totalArea = 0;
    int index = 0;
    while(index < count){
        int area = get_object_area(channel, index);
        if(area > threshold){
            //printf("Object: %d/%d w/ %d + %d -> ", index+1, count, get_object_centroid_x(channel, index), area);
            totalPosition += get_object_centroid_y(channel, index) * area;
            totalArea += area;
        }
        index++;
    }
    //printf("Done\n");
    if(totalArea == 0){
        return 999999;
    }
    return (int)(totalPosition / totalArea);
    if (get_object_count(channel) > 0){
        return get_object_area(channel,0);
    } else {
        return 999999;
    }
}

int workingCamera = 0;

void flagCamera(){
    if(camera_update()==1){
        printf("Camera is working\n");
        workingCamera = 1;
    }
}

void checkCamera(){
    thread t = thread_create(flagCamera);
    thread_start(t);
}

int updateCamera(){
    if(workingCamera){
        return camera_update();
    }else{
        return 0;
    }
}

int getcenter(int channel){
    int area = 0;
    int i = 0;
    int center = 0;
    while(area < 20 || center == 0){ //sometimes camera will detect a large blob at 0,0 randomly
        checkStatus(updateCamera(), "Failure to Update Camera\n");
        area = get_object_area(channel,0);
        center = get_object_centroid(channel,0).x;
        i++;
        if(i > 20){return -1;}
    }
    printf("Channel: %d, Area: %d, Center: %d, Iteration: %d\n", channel, area, center, i);
    return center;

    /**
    int center = 0;
    int i = 0;
    int index = 1;
    while (i < index){
	    center +=get_object_centroid(channel, i).x;
        //printf("%d,%d,%d\n", channel, center,index);
        i ++;
    }
	return (center)/(index);
    **/
}

void getOrder(int array[], int timeout){
    if(!workingCamera){
        array[RED_ZONE] = 0;
        array[GREEN_ZONE] = 1;
        array[YELLOW_ZONE] = 2;
        return;
    }
    int red = 999999;
    int green = 999999;
    int yellow = 999999;
    //long time = systime();
    checkStatus(updateCamera(), "Failure to Update Camera\n");
    red = getcenter(RED_ZONE);
    green = getcenter(GREEN_ZONE); 
    yellow = getcenter(YELLOW_ZONE);
    //if any color is not detected, automatically assume its the farthest
    if(red == -1){
        if(!state){ //if on left side
            red = 9999999;
        }
    }
    else if(green == -1){
        if(!state){ //if on left side
            green = 9999999;
        }
    }
    else if(yellow == -1){
        if(!state){ //if on left side
            yellow = 9999999;
        }
    }
    /*
    while((red == 999999 || green == 999999 || yellow == 999999) && systime() - time < timeout){

        msleep(150);
        if (red == -1){
            red = 999999;
        }
        if (yellow == -1){
            yellow = 999999;
        }
        if (green == -1){
            green = 999999;
        }
    }*/
    printf("Red: %d, Green: %d, Yellow: %d\n", red, green, yellow);
    array[RED_ZONE] = red;
    array[GREEN_ZONE] = green;
    array[YELLOW_ZONE] = yellow;

    if(array[YELLOW_ZONE] > array[RED_ZONE]){
        if(array[RED_ZONE] > array[GREEN_ZONE]){
            array[YELLOW_ZONE] = 0;
            array[RED_ZONE] = 1;
            array[GREEN_ZONE] = 2;
            // 0 < 1 < 2
        }else{
            if(array[YELLOW_ZONE] > array[GREEN_ZONE]){
                array[YELLOW_ZONE] = 0;
                array[RED_ZONE] = 2;
                array[GREEN_ZONE] = 1;
                // 0 < 2 < 1
            }else{
                array[YELLOW_ZONE] = 1;
                array[RED_ZONE] = 2;
                array[GREEN_ZONE] = 0;
                // 2 < 0 < 1
            }
        }
    }else{
        if(array[YELLOW_ZONE] > array[GREEN_ZONE]){
            array[YELLOW_ZONE] = 1;
            array[RED_ZONE] = 0;
            array[GREEN_ZONE] = 2;
            // 1 < 0 < 2
        }else{
            if(array[RED_ZONE] > array[GREEN_ZONE]){
                array[YELLOW_ZONE] = 2;
                array[RED_ZONE] = 0;
                array[GREEN_ZONE] = 1;
                // 1 < 2 < 0
            }else{
                array[YELLOW_ZONE] = 2;
                array[RED_ZONE] = 1;
                array[GREEN_ZONE] = 0;
                //2 < 1 < 0
            }
        }
    }
}



void warmupCamera(int interval, int timeout){
    long time = systime();
    while(systime() - time < timeout){
        checkStatus(updateCamera(), "Failure to Update Camera\n");
        getRelativePosition(RED_ZONE, 10);
        getRelativePosition(GREEN_ZONE, 10);
        getRelativePosition(YELLOW_ZONE, 10);
        msleep(interval);
    }
}

void zone_switch(){
    /* if case is YRG
	Green has the smallest X co-ordinate no matter where you start from (LEFT or RIGHT)
    Therefore Zone_switch works for both
*/
    //if (state == FALSE){// in right box
    if(zones[RED_ZONE] == 2){
        zones[RED_ZONE] = 0;
    }
    else if(zones[RED_ZONE] == 0){
        zones[RED_ZONE] = 2;
    }
    if(zones[YELLOW_ZONE] == 2){
        zones[YELLOW_ZONE] = 0;
    }
    else if(zones[YELLOW_ZONE] == 0){
        zones[YELLOW_ZONE] = 2;
    }
    if(zones[GREEN_ZONE] == 2){
        zones[GREEN_ZONE] = 0;
    }
    else if(zones[GREEN_ZONE] == 0){
        zones[GREEN_ZONE] = 2;
    }
    printf("Zones switched\n");
    //}
}

int main(){ 
    setup();
    set_servo_position(ARM, 780);
    msleep(100);
    
    /*
    while(1){
     	checkStatus(updateCamera(), "Failure to Update Camera\n");
        printf("%d ", get_object_area(YELLOW_ZONE, 0));
        msleep(50);
    }
	*/

    starting_box(); //left button = left box, right button = right box
    //wait_for_light(LIGHT);
    getOrder(zones, 2000);
    //zone_switch();
    printf("Red: %d, Green: %d, Yellow: %d\n", zones[RED_ZONE], zones[GREEN_ZONE], zones[YELLOW_ZONE]);
    msleep(500);
	shut_down_in(119);
    //if in right box
    if (state == FALSE){
        drive(-1500,0,2000);
        drive(-1500,-1500,400);
        drive(0, -1500, 500);
        msleep(50);
        line_follow_reverse_right(4800);
        printf("right\n");
    }
    //if in left box
    else{
        drive(1500,0,1240); //CHANGED OFTEN
        drive(1500, 1500, 1600);
        msleep(50);
        line_follow_left(3500); 
        line_follow_left(1000);
         msleep(100);    ///////////////////WITH KRAKATOA 13000
        //drive_until_color_centered_all(1500); //doesn't wrk very well
        printf("left\n");
    }
    //drive(1500, 1500, 200);
    set_servo_position(ARM, ARM_BASE + 50);

    //towards botguy + cubes
    turn_left(1300, 730);
    drive_until_black(TOPHAT_FRONT, 1500);
    drive(1500, 1500, 300);
    set_servo_position(CLAW, CLAW_OPEN - 100);
    line_follow_right(1500, 1750);

    cube();

    cubeDump();

    //turn_right(1500, 100);
    botguy();

    drive(-1500,-1500,1500);
    printf("%d", zones[RED_ZONE]);
    drive_until_black(TOPHAT_BACK, -1500);
    if(zones[RED_ZONE] == 0){
        //interpolate(ARM, ARM_UP + 500, 500);
        //turn_left(1300, 1450);
        //drive(-1500,-1500,500);
        if(zones[YELLOW_ZONE] == 1){
            drive(1500,1500,300);   
        }
        drive(1500,-1500,600); //OG 800
        drive(-1500, -1500, 200);
        line_follow_reverse_right(2700);
        msleep(1000);
        drive_until_color(RED_ZONE, 1000, -1500);
        drive_until_color_centered(RED_ZONE, -1500);
        drive(-1500,-1500,300);
    }
    else if(zones[RED_ZONE] == 2){
        if(zones[YELLOW_ZONE] == 1){
            drive(1500,1500,300);   
        }
        drive(1500, 1500, 200);
        drive(1500,-1500,600);
        drive(1500, 1500, 300);
        line_follow_left(2500);
        msleep(1000);
        drive_until_color(RED_ZONE, 1000, 1500);
        drive_until_color_centered(RED_ZONE, 1500);
        drive(-1500,-1500,100);
    }
    else{ //zone 1
        drive(-1500, -1500, 400);
        drive(1500, -1500, 660);
        drive(-1500,-1500,900);
    }
    drive(1500,-1500,400);
    set_servo_position(ARM, ARM_BASE);
    msleep(500);
    drive(800,800,500);
    //set_servo_position(CLAW, CLAW_OPEN);
    msleep(200);
    return 0;
}