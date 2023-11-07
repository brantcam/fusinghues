#include "Arduino.h"

int const MAX_PLAYER_SPEED = 10;   // Max move speed of the player
int lives = 3;

class Player {
  public:
    int pos;        // Stores the player position
    int positionModifier;// +/- adjustment to player position
    long attackMillis;         // Time the attack started
    bool attacking;            // Is the attack in progress
    bool alive;
    long killTime;
    int startingPos;

    // player color
    int r;
    int g;
    int b;

    // default constructor
    Player();
    Player(int red, int green, int blue, int sp){
      r = red;
      g = green;
      b = blue;
      startingPos = sp;
    }
};

Player::Player() {}
