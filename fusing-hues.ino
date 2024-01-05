// Required libs
#include "FastLED.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "iSin.h"
#include "RunningMedian.h"
#include "Config.h"
#include "Player.h"

// Included libs
#include "Enemy.h"
#include "Particle.h"
#include "Spawner.h"
#include "Lava.h"
#include "Boss.h"
#include "Conveyor.h"
#include "Joystick.h"

CRGB leds[NUM_LEDS];

// initialize pools
Enemy enemyPool[enemyCount];
Particle particlePool[particleCount];
Spawner spawnPool[spawnCount];
Lava lavaPool[lavaCount];
Conveyor conveyorPool[conveyorCount];
Player playerPool[playerCount];
Boss boss = Boss();
Joystick joystickPool[playerCount] = {Joystick(0x68), Joystick(0x69)};

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    // Fast LED
    FastLED.addLeds<LED_TYPE, DATA_PIN, LED_COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(12, 10000);
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.setDither(1);
    
    // Life LEDs
    for (int i = 0; i<3; i++){
        pinMode(lifeLEDs[i], OUTPUT);
        digitalWrite(lifeLEDs[i], HIGH);
    }

    // particle
    for (int i = 0; i < particleCount; i++) {
      particlePool[i] = Particle();
    }

    // enemy
    for (int i = 0; i < enemyCount; i++) {
      enemyPool[i] = Enemy();
    }

    // spawner
    for (int i = 0; i < spawnCount; i++) {
      spawnPool[i] = Spawner();
    }

    // lava
    for (int i = 0; i < lavaCount; i++) {
      lavaPool[i] = Lava();
    }

    // conveyor
    for (int i = 0; i < conveyorCount; i++) {
      conveyorPool[i] = Conveyor();
    }

    playerPool[0] = Player(0, 255, 0, 0);
    playerPool[1] = Player(0, 0, 255, 1000);

    // MPU
    Wire.begin();
    for (int i = 0; i < playerCount; i ++) {
      joystickPool[i].accelgyro.initialize();
    }
    
    loadLevel();
}

void loop() {
    long mm = millis();
    int brightness = 0;
    
    if (mm - previousMillis >= MIN_REDRAW_INTERVAL) {
        for (int i = 0; i < playerCount; i ++) {
          joystickPool[i].getInput();
        }
        long frameTimer = mm;
        previousMillis = mm;

        for (int i = 0; i < playerCount; i ++) {
          if(abs(joystickPool[i].tilt) > JOYSTICK_DEADZONE){
              lastInputTime = mm;
              if(stage == "SCREENSAVER"){
                  levelNumber = -1;
                  stageStartTime = mm;
                  stage = "WIN";
              }
          }else{
              if(lastInputTime+TIMEOUT < mm){
                  stage = "SCREENSAVER";
              }
          }
        }
        if(stage == "SCREENSAVER"){
            screenSaverTick();
        }else if(stage == "PLAY"){
            // PLAYING
            for (int i = 0; i < playerCount; i ++) {
              if(playerPool[i].attacking && playerPool[i].attackMillis+ATTACK_DURATION < mm) playerPool[i].attacking = 0;
            
              // If not attacking, check if they should be
              if(!playerPool[i].attacking && joystickPool[i].wobble > ATTACK_THRESHOLD){
                  playerPool[i].attackMillis = mm;
                  playerPool[i].attacking = 1;
              }
              
              // If still not attacking, move!
              playerPool[i].pos += playerPool[i].positionModifier;
              if(!playerPool[i].attacking){
                  int moveAmount = (joystickPool[i].tilt/6.0);
  
                  moveAmount = constrain(moveAmount, -MAX_PLAYER_SPEED, MAX_PLAYER_SPEED);
                  playerPool[i].pos -= moveAmount;
                  if(playerPool[i].pos < 0) playerPool[i].pos = 0;
                  if (playerPool[i].pos > 1000) playerPool[i].pos = 1000;
                  if(playerPool[0].pos >= playerPool[1].pos && !boss.Alive()) {
                      // Reached exit!
                      levelComplete();
                      return;
                  }
              }
              
              if(inLava(playerPool[i].pos)){
                  die(playerPool[i]);
              }
            }
            FastLED.clear();
            tickConveyors();
            tickSpawners();
            tickBoss();
            tickLava();
            tickEnemies();
            for (int i = 0; i < playerCount; i ++) {
              drawPlayer(playerPool[i]);
              drawAttack(playerPool[i]);
            }
        }else if(stage == "DEAD"){
            // DEAD
            FastLED.clear();
            if(!tickParticles()){
                loadLevel();
            }
        }else if(stage == "WIN"){
            // LEVEL COMPLETE
            FastLED.clear();
            if (stageStartTime+1500 > mm) {
              int n0 = max(map((mm-stageStartTime), 0, 1500, 0, getLED(playerPool[0].pos)), 0); // 0 -> 1500 ms ====> 0 -> player1.pos
              int n1 = max(map((mm-stageStartTime), 0, 1500, 0, NUM_LEDS-getLED(playerPool[0].pos-1)), 0); // 0 -> 1500 ms ====> player1.pos -> NUM_LEDS
              int left = 0;
              int right = 0;
              while (left < n0 || right < n1) {
                brightness = 255;
                leds[left] = CRGB(playerPool[0].r, playerPool[0].g, playerPool[0].b);
                leds[NUM_LEDS-right-1] = CRGB(playerPool[1].r, playerPool[1].g, playerPool[1].b);
                if (left < n0) {
                  left++;
                }
                if (right < n1) {
                  right++;
                }   
              }
            } else if (stageStartTime+4000 > mm) {
              int n0 = max(map((mm-stageStartTime), 1500, 4000, 0, getLED(playerPool[0].pos)), 0);
              int n1 = max(map((mm-stageStartTime), 1500, 4000, 0, NUM_LEDS-getLED(playerPool[0].pos-1)), 0);
              int left = 0;
              int right = 0;
              while (left < n0 || right < n1) {
                brightness = 255;
                leds[getLED(playerPool[0].pos)-left] = CRGB(playerPool[0].r+playerPool[1].r, playerPool[0].g+playerPool[1].g, playerPool[0].b+playerPool[1].b);
                leds[getLED(playerPool[0].pos)+right] = CRGB(playerPool[0].r+playerPool[1].r, playerPool[0].g+playerPool[1].g, playerPool[0].b+playerPool[1].b);
                if (left < n0) {
                  left++;
                }
                if (right < n1) {
                  right++;
                }
              }
            } else {
              nextLevel();
            }
        }else if(stage == "COMPLETE"){
            FastLED.clear();
            if(stageStartTime+500 > mm){
                int n = max(map(((mm-stageStartTime)), 0, 500, NUM_LEDS, 0), 0);
                for(int i = NUM_LEDS; i>= n; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(stageStartTime+5000 > mm){
                for(int i = NUM_LEDS; i>= 0; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(stageStartTime+5500 > mm){
                int n = max(map(((mm-stageStartTime)), 5000, 5500, NUM_LEDS, 0), 0);
                for(int i = 0; i< n; i++){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else{
                nextLevel();
            }
        }else if(stage == "GAMEOVER"){
            // GAME OVER!
            FastLED.clear();
            stageStartTime = 0;
        }
        
        Serial.print(millis()-mm);
        Serial.print(" - ");
        FastLED.show();
        Serial.println(millis()-mm);
    }
}


// ---------------------------------
// ------------ LEVELS -------------
// ---------------------------------
void loadLevel(){
    cleanupLevel();
    for (int i = 0; i < playerCount; i ++) {
      playerPool[i].pos = playerPool[i].startingPos;
      playerPool[i].alive = 1;
    }
    switch(levelNumber){
        case 0:
            break;
//        case 1:
//            // Slow moving Sin enemy
//            spawnEnemy(500, 1, 5, 250);
//            break;
//        case 2:
//            spawnPool[0].Spawn(350, 3000, 2, 0, 0);
//            spawnPool[1].Spawn(550, 3000, 2, 1, 0);
//            break;
//        case 3:
//            // Lava intro
//            spawnLava(400, 490, 2000, 2000, 0, "OFF");
////            spawnPool[0].Spawn(1000, 5500, 3, 0, 0);
//            break;
//        case 4:
//            // Sin enemy
//            spawnEnemy(700, 1, 7, 275);
//            spawnEnemy(500, 1, 5, 250);
//            break;
//        case 5:
//            // Conveyor
//            spawnConveyor(100, 600, -1);
////            spawnEnemy(800, 0, 0, 0);
//            break;
//        case 6:
//            // Conveyor of enemies
//            spawnConveyor(50, 1000, 1);
//            spawnEnemy(300, 0, 0, 0);
//            spawnEnemy(400, 0, 0, 0);
//            spawnEnemy(500, 0, 0, 0);
//            spawnEnemy(600, 0, 0, 0);
//            spawnEnemy(700, 0, 0, 0);
//            spawnEnemy(800, 0, 0, 0);
//            spawnEnemy(900, 0, 0, 0);
//            break;
//        case 7:
//            // Lava run
//            spawnLava(195, 300, 2000, 2000, 0, "OFF");
//            spawnLava(350, 455, 2000, 2000, 0, "OFF");
//            spawnLava(510, 610, 2000, 2000, 0, "OFF");
//            spawnLava(660, 760, 2000, 2000, 0, "OFF");
//            spawnPool[0].Spawn(1000, 3800, 4, 0, 0);
//            break;
//        case 8:
//            // Sin enemy #2
//            spawnEnemy(700, 1, 7, 275);
//            spawnEnemy(500, 1, 5, 250);
//            spawnPool[0].Spawn(1000, 5500, 4, 0, 3000);
//            spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
//            spawnConveyor(100, 900, -1);
//            break;
        case 9:
            // Boss
            spawnBoss();
            break;
    }
    stageStartTime = millis();
    stage = "PLAY";
}

void spawnBoss(){
    boss.Spawn();
    moveBoss();
}

void moveBoss(){
    int spawnSpeed = 2500;
    if(boss._lives == 2) spawnSpeed = 2000;
    if(boss._lives == 1) spawnSpeed = 1500;
    spawnPool[0].Spawn(boss._pos, spawnSpeed, 3, 0, 0);
    spawnPool[1].Spawn(boss._pos, spawnSpeed, 3, 1, 0);
}

void spawnEnemy(int pos, int dir, int sp, int wobble){
    for(int e = 0; e<enemyCount; e++){
        if(!enemyPool[e].Alive()){
            enemyPool[e].Spawn(pos, dir, sp, wobble);
            return;
        }
    }
}

void spawnLava(int left, int right, int ontime, int offtime, int offset, char* state){
    for(int i = 0; i<lavaCount; i++){
        if(!lavaPool[i].Alive()){
            lavaPool[i].Spawn(left, right, ontime, offtime, offset, state);
            return;
        }
    }
}

void spawnConveyor(int startPoint, int endPoint, int dir){
    for(int i = 0; i<conveyorCount; i++){
        if(!conveyorPool[i]._alive){
            conveyorPool[i].Spawn(startPoint, endPoint, dir);
            return;
        }
    }
}

void cleanupLevel(){
    for(int i = 0; i<enemyCount; i++){
        enemyPool[i].Kill();
    }
    for(int i = 0; i<particleCount; i++){
        particlePool[i].Kill();
    }
    for(int i = 0; i<spawnCount; i++){
        spawnPool[i].Kill();
    }
    for(int i = 0; i<lavaCount; i++){
        lavaPool[i].Kill();
    }
    for(int i = 0; i<conveyorCount; i++){
        conveyorPool[i].Kill();
    }
    boss.Kill();
}

void levelComplete(){
    stageStartTime = millis();
    stage = "WIN";
    if(levelNumber == LEVEL_COUNT) stage = "COMPLETE";
    lives = 3;
}

void nextLevel(){
    levelNumber ++;
    if(levelNumber > LEVEL_COUNT) levelNumber = 0;
    loadLevel();
}

void gameOver(){
    levelNumber = 0;
    loadLevel();
}

void die(Player player) {
    player.alive = 0;
    if(levelNumber > 0) lives --;
    if(lives == 0){
        levelNumber = 0;
        lives = 3;
    }
    for(int p = 0; p < particleCount; p++){
        particlePool[p].Spawn(player.pos);
    }
    stageStartTime = millis();
    stage = "DEAD";
    player.killTime = millis();
}

// ----------------------------------
// -------- TICKS & RENDERS ---------
// ----------------------------------
void tickEnemies(){
    for(int i = 0; i<enemyCount; i++){
        if(enemyPool[i].Alive()){
            enemyPool[i].Tick();
            // Hit attack?
            for (int j = 0; j < playerCount; j ++) {
              if(playerPool[j].attacking){
                  if(enemyPool[i]._pos > playerPool[j].pos-(ATTACK_WIDTH/2) && enemyPool[i]._pos < playerPool[j].pos+(ATTACK_WIDTH/2)){
                     enemyPool[i].Kill();
                  }
              }
            }
            if(inLava(enemyPool[i]._pos)){
                enemyPool[i].Kill();
            }
            // Draw (if still alive)
            if(enemyPool[i].Alive()) {
                leds[getLED(enemyPool[i]._pos)] = CRGB(255, 0, 0);
            }
            // Hit player(s)?
            for (int j = 0; j < playerCount; j ++) {
              if ((abs(playerPool[j].startingPos - playerPool[j].pos)) >= abs(playerPool[j].startingPos - enemyPool[i]._pos)) {
                die(playerPool[j]);
                return;
              }
            }
        }
    }
}

void tickBoss(){
    // DRAW
    if(boss.Alive()){
        boss._ticks ++;
        for(int i = getLED(boss._pos-BOSS_WIDTH/2); i<=getLED(boss._pos+BOSS_WIDTH/2); i++){
            leds[i] = CRGB::DarkRed;
            leds[i] %= 100;
        }

        for (int i = 0; i < playerPool; i ++) {
          // CHECK COLLISION
          if(getLED(playerPool[i].pos) > getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPool[i].pos) < getLED(boss._pos + BOSS_WIDTH)){
              die(playerPool[i]);
              return; 
          }
          // CHECK FOR ATTACK
          if(playerPool[i].attacking){
              if(
                (getLED(playerPool[i].pos+(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPool[i].pos+(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2)) ||
                (getLED(playerPool[i].pos-(ATTACK_WIDTH/2)) <= getLED(boss._pos + BOSS_WIDTH/2) && getLED(playerPool[i].pos-(ATTACK_WIDTH/2)) >= getLED(boss._pos - BOSS_WIDTH/2))
              ){
                 boss.Hit();
                 if(boss.Alive()){
                     moveBoss();
                 }else{
                     spawnPool[0].Kill();
                     spawnPool[1].Kill();
                 }
              }
          }
        }
    }
}

void drawPlayer(Player player){
    leds[getLED(player.pos)] = CRGB(player.r, player.g, player.b);
}

void tickSpawners(){
    long mm = millis();
    for(int s = 0; s<spawnCount; s++){
        if (spawnPool[s].Alive()) {
          leds[getLED(spawnPool[s]._pos)] = CRGB(255, 165, 0);
        }
        if(spawnPool[s].Alive() && spawnPool[s]._activate < mm){
            if(spawnPool[s]._lastSpawned + spawnPool[s]._rate < mm || spawnPool[s]._lastSpawned == 0){
                spawnEnemy(spawnPool[s]._pos, spawnPool[s]._dir, spawnPool[s]._sp, 0);
                spawnPool[s]._lastSpawned = mm;
            }
        }

        for (int j = 0; j < playerCount; j ++) {
          // CHECK COLLISION
          if (((abs(playerPool[j].startingPos - playerPool[j].pos)) >= abs(playerPool[j].startingPos - spawnPool[s]._pos)) && spawnPool[s].Alive()){
              die(playerPool[j]);
              return; 
          }
          // CHECK FOR ATTACK
          if(playerPool[j].attacking){
              if(
                (getLED(playerPool[j].pos+(ATTACK_WIDTH/2)) >= getLED(spawnPool[s]._pos) && getLED(playerPool[j].pos+(ATTACK_WIDTH/2)) <= getLED(spawnPool[s]._pos)) ||
                (getLED(playerPool[j].pos-(ATTACK_WIDTH/2)) <= getLED(spawnPool[s]._pos) && getLED(playerPool[j].pos-(ATTACK_WIDTH/2)) >= getLED(spawnPool[s]._pos))
              ){
                 spawnPool[s].Kill();
                 leds[getLED(spawnPool[s]._pos)] = CRGB(0, 0, 0);
              }
          }
        }
    }
}

void tickLava(){
    int A, B, p, i, brightness, flicker;
    long mm = millis();
    Lava LP;
    for(i = 0; i<lavaCount; i++){
        flicker = random8(5);
        LP = lavaPool[i];
        if(LP.Alive()){
            A = getLED(LP._left);
            B = getLED(LP._right);
            if(LP._state == "OFF"){
                if(LP._lastOn + LP._offtime < mm){
                    LP._state = "ON";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(3+flicker, (3+flicker)/1.5, 0);
                }
            }else if(LP._state == "ON"){
                if(LP._lastOn + LP._ontime < mm){
                    LP._state = "OFF";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(150+flicker, 100+flicker, 0);
                }
            }
        }
        lavaPool[i] = LP;
    }
}

bool tickParticles(){
    bool stillActive = false;
    for(int p = 0; p < particleCount; p++){
        if(particlePool[p].Alive()){
            particlePool[p].Tick();
            leds[getLED(particlePool[p]._pos)] += CRGB(particlePool[p]._power, 0, 0);
            stillActive = true;
        }
    }
    return stillActive;
}

void tickConveyors(){
    int b, dir, n, i, ss, ee, led;
    long m = 10000+millis();
    for (int i = 0; i < playerCount; i++) {
      playerPool[i].positionModifier = 0;
    }
    
    for(i = 0; i<conveyorCount; i++){
        if(conveyorPool[i]._alive){
            dir = conveyorPool[i]._dir;
            ss = getLED(conveyorPool[i]._startPoint);
            ee = getLED(conveyorPool[i]._endPoint);
            for(led = ss; led<ee; led++){
                b = 5;
                n = (-led + (m/100)) % 5;
                if(dir == -1) n = (led + (m/100)) % 5;
                b = (5-n)/2.0;
                if(b > 0) leds[led] = CRGB(0, 0, b);
            }

            for (int j = 0; j < playerCount; j ++) {
              if(playerPool[j].pos > conveyorPool[i]._startPoint && playerPool[j].pos < conveyorPool[i]._endPoint){
                  if(dir == -1){
                      playerPool[j].positionModifier = -(MAX_PLAYER_SPEED-4);
                  }else{
                      playerPool[j].positionModifier = (MAX_PLAYER_SPEED-4);
                  }
              }
            }
        }
    }
}

void drawAttack(Player player){
    if(!player.attacking) return;
    int n = map(millis() - player.attackMillis, 0, ATTACK_DURATION, 100, 5);
    for(int i = getLED(player.pos-(ATTACK_WIDTH/2))+1; i<=getLED(player.pos+(ATTACK_WIDTH/2))-1; i++){
        leds[i] = CRGB(0, 0, n);
    }
    if(n > 90) {
        n = 255;
        leds[getLED(player.pos)] = CRGB(255, 255, 255);
    }else{
        n = 0;
        leds[getLED(player.pos)] = CRGB(player.r, player.g, player.b);
    }
    leds[getLED(player.pos-(ATTACK_WIDTH/2))] = CRGB(n, n, 255);
    leds[getLED(player.pos+(ATTACK_WIDTH/2))] = CRGB(n, n, 255);
}

int getLED(int pos){
    // The world is 1000 pixels wide, this converts world units into an LED number
    return constrain((int)map(pos, 0, 1000, 0, NUM_LEDS-1), 0, NUM_LEDS-1);
}

bool inLava(int pos){
    // Returns if the player is in active lava
    int i;
    Lava LP;
    for(i = 0; i<lavaCount; i++){
        LP = lavaPool[i];
        if(LP.Alive() && LP._state == "ON"){
            if(LP._left < pos && LP._right > pos) return true;
        }
    }
    return false;
}

// ---------------------------------
// --------- SCREENSAVER -----------
// ---------------------------------
void screenSaverTick(){
    int n, b, c, i;
    long mm = millis();
    int mode = (mm/20000)%2;
    
    for(i = 0; i<NUM_LEDS; i++){
        leds[i].nscale8(250);
    }
    if(mode == 0){
        // Marching green <> orange
        n = (mm/250)%10;
        b = 10+((sin(mm/500.00)+1)*20.00);
        c = 20+((sin(mm/5000.00)+1)*33);
        for(i = 0; i<NUM_LEDS; i++){
            if(i%10 == n){
                leds[i] = CHSV( c, 255, 150);
            }
        }
    }else if(mode == 1){
        // Random flashes
        randomSeed(mm);
        for(i = 0; i<NUM_LEDS; i++){
            if(random8(200) == 0){
                leds[i] = CHSV( 25, 255, 100);
            }
        }
    }
}
