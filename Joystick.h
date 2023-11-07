#include "Arduino.h"

class Joystick {
  public:
    MPU6050 accelgyro;
    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;

    int tilt;              // Stores the angle of the joystick
    int wobble; 

    RunningMedian MPUAngleSamples = RunningMedian(5);
    RunningMedian MPUWobbleSamples = RunningMedian(5);

    void getInput();

    Joystick(uint8_t address) {
      accelgyro = MPU6050(address);
    }
};

void Joystick::getInput() {
    // This is responsible for the player movement speed and attacking. 
    // You can replace it with anything you want that passes a -90>+90 value to joystickTilt11
    // and any value to joystickWobble1 that is greater than ATTACK_THRESHOLD (defined at start)
    // For example you could use 3 momentery buttons:
    // if(digitalRead(leftButtonPinNumber) == HIGH) joystickTilt11 = -90;
    // if(digitalRead(rightButtonPinNumber) == HIGH) joystickTilt11 = 90;
    // if(digitalRead(attackButtonPinNumber) == HIGH) joystickWobble1 = ATTACK_THRESHOLD;
    
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    int a = (JOYSTICK_ORIENTATION == 0?ax:(JOYSTICK_ORIENTATION == 1?ay:az))/166;
    int g = (JOYSTICK_ORIENTATION == 0?gx:(JOYSTICK_ORIENTATION == 1?gy:gz));
    if(abs(a) < JOYSTICK_DEADZONE) a = 0;
    if(a > 0) a -= JOYSTICK_DEADZONE;
    if(a < 0) a += JOYSTICK_DEADZONE;
    MPUAngleSamples.add(a);
    MPUWobbleSamples.add(g);
    
    tilt = MPUAngleSamples.getMedian();

    wobble = abs(MPUWobbleSamples.getHighest());
}
