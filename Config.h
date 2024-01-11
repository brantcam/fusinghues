// LED setup
int const NUM_LEDS = 300;           // set to 300
int const DATA_PIN = 3;
int const CLOCK_PIN = 4;     
int const BRIGHTNESS = 150;        //Use a lower value for lower current power supplies(<2 amps)
int const MIN_REDRAW_INTERVAL = 16;// Min redraw interval (ms) 33 = 30fps / 16 = 63fps
int const USE_GRAVITY = 0;         // 0/1 use gravity (LED strip going up wall)
int const BEND_POINT = 550;        // 0/1000 point at which the LED strip goes up the wall
#define LED_COLOR_ORDER      GRB   //if colours aren't working, try GRB or GBR or BGR
#define LED_TYPE             WS2813//type of LED strip to use(APA102 - DotStar, WS2811 - NeoPixel) For Neopixels, uncomment line #108 and comment out line #106

// GAME
long previousMillis = 0;           // Time of the last redraw
int levelNumber = 0;
long lastInputTime = 0;
int const TIMEOUT = 30000;
int const LEVEL_COUNT = 5;
iSin isin = iSin();

// JOYSTICK
int const JOYSTICK_ORIENTATION = 1;// 0, 1 or 2 to set the angle of the joystick
int const ATTACK_THRESHOLD = 30000;// The threshold that triggers an attack
int const JOYSTICK_DEADZONE = 5;   // Angle to ignore

// WOBBLE ATTACK
int const ATTACK_WIDTH = 80;     // Width of the wobble attack, world is 1000 wide
int const ATTACK_DURATION = 500; // Duration of a wobble attack (ms)
int const BOSS_WIDTH = 40;

// STAGE
char* stage;                       // what stage the game is at (PLAY/DEAD/WIN/GAMEOVER)
long stageStartTime;               // Stores the time the stage changed for stages that are time based

int const LIFE_TOTAL = 6; // if changed update in Player.h
//int const LIFE_TOTAL = 1;

int const enemyCount = 10;
int const particleCount = 40;
int const spawnCount = 2;
int const lavaCount = 4;
int const conveyorCount = 4;
int const playerCount = 2;
