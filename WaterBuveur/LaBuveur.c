#include <kipr/botball.h>

//PORTS
#define ARM 		0
#define CLAW 		3
#define ET_SERVO	2

//Analog PORTS
#define REFLECTANCE 5
#define ET 0
#define LIGHT_PORT 	4

//digital ports
#define LEVER 0

//Basic ARM movements
#define ARM_DOWN	2047
#define ARM_UP		1520
#define ARM_START   0
//Basic CLAW movements
#define CLAW_OPEN	690
#define CLAW_CLOSE	1300
#define CLAW_OPEN_PEOPLE 750
#define CLAW_MID	1010

//Thresholds - for create sensor
#define THRESHOLD 2500
#define SWHITE 500
#define SBLACK 2500

//Special
#define GRAY 500

//ET Positions
#define ET_START	299
#define ET_RUN		900
#define ET_SHOW		1800


//////////////////////////////////////////////////Basic Functions//////////////////////////////////////////////////////

void stop() {
    create_drive_direct(0, 0);
}

void drive(int speed, int time) {
    create_drive_direct(-speed *.96, -speed); 
    msleep(time);
    stop();
}

void turn_right(int speed, int time) {
    create_drive_direct(speed, -speed);
    msleep(time);
    stop();
}

void turn_left(int speed, int time) {
    create_drive_direct(-speed, speed);
    msleep(time);
    stop();
}

void interpolate(int srv, int pos, int step_size) {
    int cur_pos = get_servo_position(srv);
    int delta_remain = abs(pos - cur_pos);  
    int direction = pos > cur_pos ? 1 : -1;

    while(delta_remain >= 5) {
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

void back_until_bump(){
    while(!get_create_lbump()&&!get_create_rbump()){
        create_drive_direct(200,200);
    }
    stop();
}

int getSign(int n, int tolerance){
    if(n > tolerance){
        return 1;
    }
    else if(n < -tolerance){
        return -1;
    }
    return 0;
}

void driveToLine(int power, int adjustPower){
    int leftSign = getSign(THRESHOLD - get_create_lcliff_amt(),0);
    int rightSign = getSign(THRESHOLD - get_create_rcliff_amt(),0);
    int rpower = rightSign * power;
    int lpower = leftSign * power;
    while(!(leftSign == 1 && rightSign == 1)){
        create_drive_direct(rpower, lpower);
        msleep(1);
        leftSign = getSign(THRESHOLD - get_create_lcliff_amt(),0);
        rightSign = getSign(THRESHOLD - get_create_rcliff_amt(),0);
        if(leftSign == 1){
            rpower = rightSign * adjustPower;
        }
        if(rightSign == 1){
            lpower = leftSign * adjustPower;
        }
    }
    stop();
}

inline int over_black()
{
    //printf("REFLECTANCE: %d %d\n", analog(REFLECTANCE), analog(REFLECTANCE) > THRESHOLD);
    return analog(REFLECTANCE) > 2900;
}

//for the black on the left and the white/gray on the right
void line_follow(int time) {
    int t = 0, interval = 10;
    while(t * interval < time){
        if(!over_black()) {
            create_drive_direct(-200, -150);
        }
        else {
            create_drive_direct(-150, -200);
        }
        msleep(interval);
        t++;
    }
    stop();
}

//for the black on the right and the white/Gray on the left
void line_follow_left(int time) {
    int t = 0, interval = 10;
    while(t * interval < time){
        if(over_black()) {
            create_drive_direct(-200, -150);
        }
        else {
            create_drive_direct(-150, -200);
        }
        msleep(interval);
        t++;
    }
    stop();
}


void lfet(int d){
    //int t = 0;
    //while(t <= 3000);
    //int x = analog(ET);
    //int y = analog(ET);
    //new
    //while(y - x <= 200){
    //printf("OLD = %d \nNEW = %d\n\n", x, y);
    //x = y;
   // set_servo_position(ET_SERVO, ET_SHOW);
    //msleep(1000);
    if(d == 1){
        while((analog(ET) > 1540 && analog(ET) < 2800)){
            if(!over_black()) {
                create_drive_direct(-80, -40);
            }
            else {
                create_drive_direct(-40, -80);
            }
            //msleep(10);
                //y = analog(ET);
                //msleep(interval);
                //t++;
        	//printf("FRONT %d\n", analog(ET));
        }
        stop();
        printf("FINAL: %d\n", analog(ET));
    }
    if(d == -1){
        while((analog(ET) > 1540 && analog(ET) < 2800)){
            if(get_create_lfcliff_amt() > SBLACK) {
            create_drive_direct(140, 100);
        }
        else {
            create_drive_direct(100, 140);
        }
            printf("BACK %d\n", analog(ET));
        }
        stop();
    	printf("FINAL: %d\n", analog(ET));
    }
    //set_servo_position(ET_SERVO, ET_RUN);
    msleep(200);
}

float average(int values[], int size) {
    int i = 0;
    float sum = 0;
    //printf("[");
    for (i = 0; i < size; i++) {
        //printf("%f, ", (float) values[i]);
        sum += (float) values[i];
    }
    //printf("]");
    //printf("[sum=%f, size=%d]", sum, size);
    return sum / ((float) size);
}

float stdSquared(int values[], int size) {
    float mean = average(values, size);
    int i = 0;
    float sum = 0;
    for (i = 0; i < size; i++) {
        float diff = ((float) values[i]) - mean;
        //printf("%f %f %d %f - ", diff, sum, values[i], mean);
        sum += diff * diff;
    }
    printf("%f-> ", sum / ((float) (size - 1)));
    return sum / ((float) (size - 1));
}

void driveTillET(int left, int right, int threshold, int sign) {
    int arraySize = 50;
    int etValues[arraySize];
    int currentIndex = -1;
    int tophat = analog(REFLECTANCE);
    double percent = ((double) (tophat - SWHITE)) / ((double) (SBLACK - SWHITE));
    int counter = 0;
    printf("Start: %d, ", analog(ET));
    stop();
    while (counter <= (arraySize+500) || (sign * (stdSquared(etValues, sizeof(etValues)/sizeof(int)) - threshold)) < 0) {
        create_drive_direct((percent < 0.5 ? 1 : (1.0 - percent) / 0.5) * left, (percent > 0.5 ? 1 : (percent / 0.5)) * right);
        //msleep(1);
        tophat = analog(REFLECTANCE);
        percent = ((double) (tophat - SWHITE)) / ((double) (SBLACK - SWHITE));
        currentIndex = (currentIndex + 1) % arraySize;
        etValues[currentIndex] = analog(ET);
        //printf("%d, ", analog(ET));
        counter++;
    }
    printf("Done\n");
}

#define IRWHITE 2500
#define IRBLACK 500

void lineFollowIr(int left, int right, int time) {
    long endTime = systime() + time;
    int ir = get_create_lfcliff_amt();
    double percent = ((double) (ir - IRWHITE)) / ((double) (IRBLACK - IRWHITE));
    while (systime() < endTime) {
        create_drive_direct(((percent > 0.5 ? 1 : (percent / 0.5)) * left), ((percent < 0.5 ? 1 : (1.0 - percent) / 0.5) * right));
        msleep(1);
        ir = get_create_lfcliff_amt();
        printf("%f->", percent);
        percent = ((double) (ir - IRWHITE)) / ((double) (IRBLACK - IRWHITE));
    }
}

void back_follow(int time){
    int t = 0, interval = 10;
    while(t * interval < time){
        if(get_create_lfcliff_amt() > SBLACK) {
            create_drive_direct(250, 200);
        }
        else {
            create_drive_direct(200, 250);
        }
        msleep(interval);
        t++;
    }
    stop();
}


void lineFollow(int left, int right, int time) {
    long endTime = systime() + time;
    int tophat = analog(REFLECTANCE);
    double percent = ((double) (tophat - SWHITE)) / ((double) (SBLACK - SWHITE));
    while (systime() < endTime) {
        create_drive_direct((percent < 0.5 ? 1 : (1.0 - percent) / 0.5) * left, (percent > 0.5 ? 1 : (percent / 0.5)) * right);
        msleep(1);
        tophat = analog(REFLECTANCE);
        percent = ((double) (tophat - SWHITE)) / ((double) (SBLACK - SWHITE));
    }
}

void lever(){
    int t = 0;
    int time = 1000;
    while(digital(LEVER) != 1 && t < time){
        create_drive_direct(-100, -100);
        msleep(1);
        t++;
        if(t == time){
            stop();
            msleep(100);
            drive(-200, 400);
            turn_left(120, 30);
        }
    }
    stop();
    if(t == time){
        lever();
    }
}

void interpolateServoByTime(int port, int start, int end, int time){
    unsigned long endTime = systime() + time;
    while(systime() < endTime){
        set_servo_position(port, (end - start) * (1.0 - (((double)(endTime - systime())) / ((double)time))) + start);
        msleep(1);
    }
    set_servo_position(port, end);
}

///////////////////////////////////////////////////////////////////////CODE////////////////////////////////////////////////////////////////////////////////
void setup(){
    create_connect();
    create_full();
    enable_servo(CLAW);
    enable_servo(ARM);
    enable_servo(ET_SERVO);
    set_servo_position(CLAW, CLAW_CLOSE);
    msleep(100);
    set_servo_position(ET_SERVO, ET_RUN); 
    interpolate(ARM, 0, 50);
    set_servo_position(ET_SERVO, ET_START);
    msleep(100);
}

void init(){
    drive(-100, 500);
    turn_right(100, 2000);
    turn_right(150, 1060);
    msleep(1000); 
    //wait_for_light
}

void firstAndSecondSupplies(){
    set_servo_position(ET_SERVO, ET_SHOW);
    msleep(100); 
    interpolate(ARM, ARM_DOWN, 100);
    set_servo_position(CLAW, CLAW_OPEN);
    driveToLine(270, 30);
    drive(-300, 75);
    interpolate(CLAW, CLAW_CLOSE, 70);
    drive(-300, 500);
    turn_right(300, 150);
    drive(300, 1100);

    turn_right(200, 1700);
    drive(300, 300);
    set_servo_position(CLAW, CLAW_OPEN_PEOPLE);
    interpolate(ARM, ARM_UP, 80); 
    turn_left(200, 100);
}

void thirdSupplies(){
    //drive(-200, 650);
    driveToLine(-300, -20);
    drive(-200, 250);
    turn_right(200, 960);
    
    interpolate(ARM, 1875, 80); 
    interpolate(CLAW, CLAW_OPEN, 70);
    line_follow(500);  
    interpolate(ARM, ARM_DOWN, 80); 
    interpolate(CLAW, CLAW_CLOSE, 60);
    interpolate(ARM, ARM_UP, 70); 
    turn_left(150, 1250);
    drive(200, 900);
    interpolate(ARM, 1850, 80); 
    //interpolate(CLAW, CLAW_OPEN, 70);
    set_servo_position(CLAW, CLAW_OPEN);
    msleep(50);
    interpolate(ARM, ARM_UP, 90);  //drops 3rd supplies
}

void fourthSupplies(){    
    driveToLine(-300, 50);
    drive(-200, 200);
    turn_right(200, 960); 
    
    interpolate(ARM, ARM_DOWN, 80); 
    line_follow(2100); 
    //lfet(1);
     
    interpolate(CLAW, CLAW_CLOSE, 75);
    interpolate(ARM, ARM_UP, 70); //grabs 4th supplies
	drive(-200, 200); 
    turn_left(200, 1000);
    drive(200, 1000);
    interpolate(ARM, 1800, 80); 
    //interpolate(CLAW, CLAW_OPEN_PEOPLE+20, 70);
    set_servo_position(CLAW, CLAW_OPEN_PEOPLE + 150);
    msleep(100);
    interpolate(ARM, ARM_UP, 90); 
    driveToLine(-300, 50);
    drive(-150, 350);
    turn_right(200, 950); 
    set_servo_position(ET_SERVO, ET_SHOW); 
    line_follow(300); 
}

void firstPoms(){
    lfet(1);
    turn_right(300, 245);
    msleep(100);
    set_servo_position(ET_SERVO, ET_RUN);
    interpolate(ARM, ARM_DOWN, 120); 
    drive(300, 740);
    interpolate(CLAW, CLAW_CLOSE, 90); 
    drive(-300, 740); 
    turn_left(300, 245);
}

void secondPoms(){ 
    set_servo_position(ET_SERVO, ET_SHOW);
    back_follow(300); 
    lfet(-1);
    back_follow(100);
    interpolate(ET_SERVO, ET_RUN, 90); 
    turn_right(200, 420); 
    set_servo_position(CLAW, CLAW_OPEN + 200); 
    set_servo_position(ET_SERVO, ET_RUN);
    //interpolate(ARM, ARM_DOWN, 90); 
    drive(300, 560);
    interpolate(CLAW, CLAW_CLOSE, 90); 
    drive(-300, 560); 
    turn_left(200, 420);
    interpolate(ARM, ARM_UP, 90);   
}

void pomDump(){
   //interpolate(ET_SERVO, ET_SHOW, 90);  
   back_follow(1300); //1460
   //lfet(-1);  
   back_until_bump();
   line_follow(1150);
   //back_follow(410);
   turn_left(200, 920);
   create_drive_direct(-70, -70);
   interpolateServoByTime(CLAW, CLAW_CLOSE, CLAW_OPEN + 70, 1300); 
   stop(); 
   msleep(300);
   interpolate(ARM, ARM_UP - 200, 70);
}

void thirdPoms(){
    driveToLine(-300, -50);
    drive(-200, 250);
    turn_right(150, 1320);
    interpolate(ET_SERVO, ET_SHOW, 80); 
    line_follow(1650); 
    lfet(-1);  
    back_follow(180);
    turn_right(300, 245);
    msleep(100);
    set_servo_position(CLAW, CLAW_OPEN + 100); 
    set_servo_position(ET_SERVO, ET_RUN);
    interpolate(ARM, ARM_DOWN, 120); 
    drive(300, 700);
    interpolate(CLAW, CLAW_CLOSE, 90); 
    drive(-300, 700); 
    turn_left(300, 245);
    interpolate(ARM, ARM_UP, 90); 
}

void fourthPoms() {
    back_follow(200); //500
    back_until_bump(); 
    turn_right(300, 270);
    msleep(100);
    set_servo_position(ET_SERVO, ET_RUN);
    msleep(100); 
    interpolate(ARM, ARM_DOWN, 90); 
    interpolate(CLAW, CLAW_OPEN, 90); 
    drive(300, 710);
    interpolate(CLAW, CLAW_CLOSE, 90); 
    drive(-300, 710); 
    turn_left(300, 270);
    interpolate(ARM, ARM_UP, 90); 
}

void pomDump2(){
   line_follow(1150); 
   turn_left(200, 920);
   drive(100, 100); 
   create_drive_direct(-70, -70);
   interpolateServoByTime(CLAW, CLAW_CLOSE, CLAW_OPEN + 30, 1300); 
   stop(); 
   msleep(100);
   interpolate(ARM, ARM_UP - 200, 70);
   //driveToLine(-250, -30);
   drive(-250, 200);
   msleep(50);
   interpolate(ARM, ARM_DOWN, 70); 
   drive(250, 350);
   interpolate(CLAW, CLAW_CLOSE-200, 70); 
   drive(250, 700);  
   turn_left(200, 350); 
   interpolate(CLAW, CLAW_OPEN, 70);
   interpolate(ARM, ARM_UP, 70); 
   turn_right(200, 350);
   turn_left(100, 100);
   drive(-250, 750); 
}

void otherBin(){
    
    driveToLine(-250, -50);
    drive(-300, 100);
    turn_right(300, 580);
    turn_right(300, 580);
    drive(-300, 200);
    driveToLine(260, 20);
    interpolate(ARM, ARM_DOWN - 210, 40);
    set_servo_position(CLAW, CLAW_OPEN + 80);
	drive(150, 1200);
    interpolate(ARM, ARM_DOWN, 55); 
    interpolate(CLAW, 1300, 30);
    interpolate(ARM, 1500, 40);
    drive(-200, 900);
    interpolate(ARM, ARM_DOWN, 50);
    turn_right(300, 300);
    drive(200, 800);
    set_servo_position(CLAW, CLAW_OPEN); 
    drive(-300, 800);
    set_servo_position(CLAW, CLAW_CLOSE);
    interpolate(ARM, ARM_START, 120); 
    turn_left(300, 300);
    
}

void gasValve(){
    set_servo_position(ET_SERVO, ET_RUN);
    turn_left(300, 600); //////UNCOMMENT
    back_until_bump(); 
    line_follow(800);
    turn_right(250, 700);
    interpolate(ARM, ARM_DOWN - 190, 120);
    set_servo_position(CLAW, CLAW_OPEN);
    //msleep(50);
    //drive(-200, 200);
    driveToLine(250, 30);
    drive(250, 400);
    set_servo_position(CLAW, CLAW_MID + 400);
    drive(-200, 150);
    interpolate(ARM, 890, 90);
    driveToLine(-250, -30);
    turn_right(300, 1050);
    interpolate(ARM, ARM_DOWN - 20, 90);
    drive(300, 850);
    set_servo_position(CLAW, CLAW_OPEN + 200);
    //msleep(50);
}

void powerLines(){
    //drive(-200, 400);
    driveToLine(-250, -30);
    set_servo_position(CLAW, CLAW_CLOSE);
    interpolate(ARM, 0, 270);
    drive(250, 600);
    turn_left(300, 610);
    drive(350, 680);
    drive(-200, 990);
    turn_left(250, 700);
    interpolate(ET_SERVO, ET_SHOW - 20, 60);
    //turn_left(100, 100);
    //turn_right(100, 100);
    /*while(get_create_lcliff_amt() > THRESHOLD){
        printf("%d\n", get_create_lcliff_amt());
        create_drive_direct(100, 100);
    }
    stop();
    */
    /*
    int a = 0;
    while(a < 3){
        interpolate(ET_SERVO, get_servo_position(ET_SERVO) - 80, 10);
        turn_right(95, 290);
        msleep(200);
        drive(80, 800);
        msleep(200);
        turn_left(120, 250);
        msleep(200);
        drive(-80, 1200);
        msleep(400);
        a++;
    }
    */
    drive(-120, 930);
    interpolate(ET_SERVO, ET_RUN + 300, 40);
    turn_right(100, 400);
    //drive(120, 200);
    create_drive_direct(-40, -240);
    msleep(450);
    stop();
    //drive(200, 400);
    interpolate(ET_SERVO, get_servo_position(ET_SERVO) + 300, 20);
    /*
    create_drive_direct(100, 200);
    msleep(1000);
    create_drive_direct(0, -80);
    msleep(600);
    drive(120, 150);
    */
    /*
    interpolate(ARM, 1020, 40);
    drive(-200, 200);
    turn_left(140, 1030);
    drive(-200, 600);
    
    set_servo_position(ET_SERVO, ET_RUN);
    msleep(100);
    set_servo_position(CLAW, CLAW_MID);
    msleep(1000);
    interpolate(ARM, ARM_UP, 30); //1625
    drive(70, 1300);
    msleep(200);
    //drive(-80, 2600);
    interpolate(CLAW, CLAW_CLOSE, 5);
    interpolate(ARM, 1020, 20);
    msleep(1000);
    turn_right(70, 610);
    drive(80, 2450);
    interpolate(CLAW, CLAW_MID, 5);
    msleep(1000);
    interpolate(ARM, ARM_UP, 10);
    drive(-80, 1000);
    */
    turn_left(130, 1300);
    drive(200, 300);
    turn_right(220, 350);
    driveToLine(300, 35);
    
    drive(-200, 400);
    turn_right(150, 420);
    drive(200, 680);
    create_drive_direct(-200, -50);
    msleep(790);
    stop();
    //drive(200, 200);
    interpolate(ET_SERVO, get_servo_position(ET_SERVO) - 200, 50);
}

int main(){
    setup();
    init();
    float x = systime();
    firstAndSecondSupplies();
    printf("FIRST AND SECOND SUPPLIES: %d\n", (int) (systime() - x));
    thirdSupplies();
    fourthSupplies();
    printf("FINISH DROPPPING SUPPLIES: %d\n", (int) (systime() - x));
    /*
    create_connect();
    create_full();
    enable_servo(CLAW);
    enable_servo(ARM);
    enable_servo(ET_SERVO);
    
    set_servo_position(ET_SERVO, ET_RUN);
    interpolate(CLAW, CLAW_CLOSE, 90); 
    interpolate(ARM, ARM_UP, 90);   
    */
    firstPoms(); 
    secondPoms(); 
    pomDump();
    thirdPoms(); 
    fourthPoms(); 
    pomDump2(); 
    printf("DROPPED IN BIN: %d\n", (int) (systime() - x));
    otherBin();
    gasValve();
    powerLines();
    printf("CODE DONE:%d\n", (int) (systime() - x));
    return 0;
}

