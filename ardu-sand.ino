/*
provided 'AS IS', use at your own risk
 * mirel.t.lazar@gmail.com
 * 
 * 
ESP8266 support
Go to File > Preferences
Click the icon to the right of Additional Board Manager URLs
Paste this URL on a separate line (sequence does not matter).
http://arduino.esp8266.com/stable/package_esp8266com_index.json
Click Ok to get out of Preferences
Navigate to: Tools > Board xyz > Board Manager...
Search for 8266
Install esp8266 by ESP8266 Community.
*/
#define DEBUG 0 //2% memory use
#define debug_println(...) \
            do { if (DEBUG) Serial.println(__VA_ARGS__); } while (0)
#define debug_print(...) \
            do { if (DEBUG) Serial.print(__VA_ARGS__); } while (0)
#define debug_printf(...) \
            do { if (DEBUG) Serial.printf(__VA_ARGS__); } while (0)

//#define PxMATRIX_double_buffer true
#include <PxMatrix.h>
//
#include <Adafruit_PixelDust.h> // For simulation

#include <Wire.h>
#include <LSM6.h>
LSM6 imu;
char have_imu = 0;
#define SDA_PIN 1
#define SCL_PIN 3

#ifdef ESP8266
#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2
#endif
// Pins for LED MATRIX
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);

#define USE_SERIAL Serial

//#define USE_FIREWORKS
#define FIREWORKS_DISPLAY 10//sec
#define FIREWORKS_LOOP    50//ms

#ifdef ESP8266
// ISR for display refresh
void display_updater ()
{
  //display.displayTestPattern(70);
  display.display (60);
}
#endif

#define N_GRAINS     50 // Number of grains of sand
#define WIDTH        64 // Display width in pixels
#define HEIGHT       32 // Display height in pixels
#define MAX_FPS      45 // Maximum redraw rate, frames/second

// Sand object, last 2 args are accelerometer scaling and grain elasticity
Adafruit_PixelDust sand (WIDTH, HEIGHT, N_GRAINS, 1, 128);

// Since we're not using the GFX library, it's necessary to buffer the
// display contents ourselves (8 bits/pixel with the Charlieplex drivers).
int grcol[N_GRAINS];

int cc_blk = 0;
int cc_snd = 0;
//
void setup ()
{
  if (DEBUG)
  {
    Serial.begin (115200);
    while (!Serial)
      delay (500); //delay for Leonardo
  }
  debug_println ("");
  debug_println ("debug ready");
  //display setup
  display.begin (16);
  display.setFastUpdate(true);
#ifdef ESP8266
  display_ticker.attach (0.002, display_updater);
  debug_println ("ticker attached");
#endif

  if(!sand.begin())
    debug_println ("Failed to initialize sand");

  memset(grcol, 0, sizeof(int)*N_GRAINS);   // Clear pixel buffer

  char red   = (rand() % 255);/// (float)RAND_MAX);
  char green = (rand() % 255); /// (float)RAND_MAX);
  char blue  = (rand() % 255); /// (float)RAND_MAX);
  cc_snd = display.color565 (red, green, blue);
  //prep screen for clock display
  //display.fillScreen (cc_snd);
#if 0
  // Draw an obstacle for sand to move around
  for(uint8_t y=0; y<3; y++) {
    for(uint8_t x=0; x<3; x++) {
      sand.setPixel(6+x, 2+y);             // Set pixel in the simulation
      display.drawPixel (6+x, 2+y, cc_snd);
      debug_print ("sand (");
      debug_print ((2+y)*WIDTH+6+x);
      debug_print ("): ");
      debug_println (cc_snd);
    }
  }
#endif
  sand.randomize(); // Initialize random sand positions
  //init colors
  dimension_t x, y;
  for(uint8_t i=0; i<N_GRAINS; i++) {
    red   = (rand() % 255);/// (float)RAND_MAX);
    green = (rand() % 255); /// (float)RAND_MAX);
    blue  = (rand() % 255); /// (float)RAND_MAX);
    cc_snd = display.color565 (red, green, blue);
    grcol[i] = cc_snd;
    debug_print ("sand (");
    debug_print (i);
    debug_print ("): ");
    debug_println (cc_snd);
  }
  debug_println ("sand ready");
  //imu
  Wire.begin (SDA_PIN, SCL_PIN);
  if (imu.init())
  {
    imu.enableDefault();
    have_imu = 1;
  }
  //done setup
}

#ifdef USE_FIREWORKS
//fireworks
// adapted to Arduino pxMatrix
// from https://r3dux.org/2010/10/how-to-create-a-simple-fireworks-effect-in-opengl-and-sdl/
// Define our initial screen width, height, and colour depth
int SCREEN_WIDTH  = 64;
int SCREEN_HEIGHT = 32;

const int FIREWORKS = 6;           // Number of fireworks
const int FIREWORK_PARTICLES = 8;  // Number of particles per firework

class Firework
{
  public:
    float x[FIREWORK_PARTICLES];
    float y[FIREWORK_PARTICLES];
    char lx[FIREWORK_PARTICLES], ly[FIREWORK_PARTICLES];
    float xSpeed[FIREWORK_PARTICLES];
    float ySpeed[FIREWORK_PARTICLES];

    char red;
    char blue;
    char green;
    char alpha;

    int framesUntilLaunch;

    char particleSize;
    boolean hasExploded;

    Firework(); // Constructor declaration
    void initialise();
    void move();
    void explode();
};

const float GRAVITY = 0.05f;
const float baselineSpeed = -1.0f;
const float maxSpeed = -2.0f;

// Constructor implementation
Firework::Firework()
{
  initialise();
  for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
  {
    lx[loop] = 0;
    ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
  }
}

void Firework::initialise()
{
    // Pick an initial x location and  random x/y speeds
    float xLoc = (rand() % SCREEN_WIDTH);
    float xSpeedVal = baselineSpeed + (rand() % (int)maxSpeed);
    float ySpeedVal = baselineSpeed + (rand() % (int)maxSpeed);

    // Set initial x/y location and speeds
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        x[loop] = xLoc;
        y[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
        xSpeed[loop] = xSpeedVal;
        ySpeed[loop] = ySpeedVal;
        //don't reset these otherwise particles won't be removed
        //lx[loop] = 0;
        //ly[loop] = SCREEN_HEIGHT + 1; // Push the particle location down off the bottom of the screen
    }

    // Assign a random colour and full alpha (i.e. particle is completely opaque)
    red   = (rand() % 255);/// (float)RAND_MAX);
    green = (rand() % 255); /// (float)RAND_MAX);
    blue  = (rand() % 255); /// (float)RAND_MAX);
    alpha = 50;//max particle frames

    // Firework will launch after a random amount of frames between 0 and 400
    framesUntilLaunch = ((int)rand() % (SCREEN_HEIGHT/2));

    // Size of the particle (as thrown to glPointSize) - range is 1.0f to 4.0f
    particleSize = 1.0f + ((float)rand() / (float)RAND_MAX) * 3.0f;

    // Flag to keep trackof whether the firework has exploded or not
    hasExploded = false;

    //cout << "Initialised a firework." << endl;
}

void Firework::move()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Once the firework is ready to launch start moving the particles
        if (framesUntilLaunch <= 0)
        {
            //draw black on last known position
            //display.drawPixel (x[loop], y[loop], cc_blk);
            lx[loop] = x[loop];
            ly[loop] = y[loop];
            //
            x[loop] += xSpeed[loop];

            y[loop] += ySpeed[loop];

            ySpeed[loop] += GRAVITY;
        }
    }
    framesUntilLaunch--;

    // Once a fireworks speed turns positive (i.e. at top of arc) - blow it up!
    if (ySpeed[0] > 0.0f)
    {
        for (int loop2 = 0; loop2 < FIREWORK_PARTICLES; loop2++)
        {
            // Set a random x and y speed beteen -4 and + 4
            xSpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
            ySpeed[loop2] = -4 + (rand() / (float)RAND_MAX) * 8;
        }

        //cout << "Boom!" << endl;
        hasExploded = true;
    }
}

void Firework::explode()
{
    for (int loop = 0; loop < FIREWORK_PARTICLES; loop++)
    {
        // Dampen the horizontal speed by 1% per frame
        xSpeed[loop] *= 0.99;

        //draw black on last known position
        //display.drawPixel (x[loop], y[loop], cc_blk);
        lx[loop] = x[loop];
        ly[loop] = y[loop];
        //
        // Move the particle
        x[loop] += xSpeed[loop];
        y[loop] += ySpeed[loop];

        // Apply gravity to the particle's speed
        ySpeed[loop] += GRAVITY;
    }

    // Fade out the particles (alpha is stored per firework, not per particle)
    if (alpha > 0)
    {
        alpha -= 1;
    }
    else // Once the alpha hits zero reset the firework
    {
        initialise();
    }
}

// Create our array of fireworks
Firework fw[FIREWORKS];

void fireworks_loop (int frm)
{
  int cc_frw = frm;
  //display.fillScreen (0);
  // Draw fireworks
  //cout << "Firework count is: " << Firework::fireworkCount << endl;
  for (int loop = 0; loop < FIREWORKS; loop++)
  {
      for (int particleLoop = 0; particleLoop < FIREWORK_PARTICLES; particleLoop++)
      {
  
          // Set colour to yellow on way up, then whatever colour firework should be when exploded
          if (fw[loop].hasExploded == false)
          {
              //glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
              cc_frw = display.color565 (255, 255, 0);
          }
          else
          {
              //glColor4f(fw[loop].red, fw[loop].green, fw[loop].blue, fw[loop].alpha);
              //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
              cc_frw = display.color565 (fw[loop].red, fw[loop].green, fw[loop].blue);
          }

          // Draw the point
          //glVertex2f(fw[loop].x[particleLoop], fw[loop].y[particleLoop]);
          display.drawPixel (fw[loop].x[particleLoop], fw[loop].y[particleLoop], cc_frw);
          display.drawPixel (fw[loop].lx[particleLoop], fw[loop].ly[particleLoop], 0);
      }
      // Move the firework appropriately depending on its explosion state
      if (fw[loop].hasExploded == false)
      {
          fw[loop].move();
      }
      else
      {
          fw[loop].explode();
      }
      //
      //delay (10);
  }
}
//-
#endif //define USE_FIREWORKS
uint32_t        prevTime   = 0;      // Used for frames-per-second throttle

void loop ()
{
  static int i = 0;
  //
#ifdef USE_FIREWORKS
  static int last = 0;
  static int cm;
  //animations?
  cm = millis ();
  //fireworks on 1st of Jan 00:00, for 55 seconds
  if ((cm - last) > FIREWORKS_LOOP)
  {
    //debug_println(millis() - last);
    last = cm;
    i++;
    fireworks_loop (i);
    //debug_println ("fireworks updated!");
  }
#endif //define USE_FIREWORKS
  // Limit the animation frame rate to MAX_FPS.  Because the subsequent sand
  // calculations are non-deterministic (don't always take the same amount
  // of time, depending on their current states), this helps ensure that
  // things like gravity appear constant in the simulation.
  uint32_t t;
  while(((t = micros()) - prevTime) < (1000000L / MAX_FPS));
  prevTime = t;

  // Erase old grain positions in pixelBuf[]
  dimension_t x, y;
  for(i=0; i<N_GRAINS; i++) {
    sand.getPosition(i, &x, &y);
    //pixelBuf[y * WIDTH + x] = 0;
    display.drawPixel (x, y, 0);
  }
  
  // Read accelerometer...
  //accel.read();
  // Run one frame of the simulation
  // X & Y axes are flipped around here to match physical mounting
  //sand.iterate(-accel.y, accel.x, accel.z);
  if (have_imu)
  {
    imu.read();
    sand.iterate (imu.a.y, imu.a.x, -imu.a.z);
  }
  else
  {
    //sand.iterate (1000, 1000, 1000);
  }

  // Draw new grain positions in pixelBuf[]
  for(i=0; i<N_GRAINS; i++) {
    sand.getPosition(i, &x, &y);
    //pixelBuf[y * WIDTH + x] = 80;
    display.drawPixel (x, y, grcol[i]);
  }

  // Update pixel data in LED driver
  //display.showBuffer();
  //yield();
}
