//
//
//
// Fastled
#include <FastLED.h>
FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_PIN D5
#define NUM_LEDS 12

#define CHIPSET     WS2811
#define COLOR_ORDER GRB

#define FRAMES_PER_SECOND  120
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_LEDS];

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void initLeds() {
  // Initialize LED strip
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(config.brightness);
}

// Callback LedGame changing
void lgCallback() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void ledLoop() {
  if((config.timeout == 0)||(inactivityTimer < config.timeout)) {
    switch(int(config.mode)) {
      case LED_MODE_GAME: gPatterns[gCurrentPatternNumber]();
        EVERY_N_MILLISECONDS( 20 ) { gHue++; } 
        break; 
      case LED_MODE_MOOD: ledMoodLoop();
        break;
      case LED_MODE_SOUND: ledSoundLoop();
        break;
      case LED_MODE_MANUAL: ledManualLoop();
        break;
      default: fadeOut();
        break;
    }
  } else {
    fadeOut();
  }
    
  FastLED.setBrightness(config.brightness);
  FastLED.show();  
  FastLED.delay(1000/config.speed); 
}

void fadeOut() {
  // current gradient palette color index
  static uint8_t heatIndex = 0; // start out at 0

  // HeatColors_p is a gradient palette built in to FastLED
  // that fades from black to red, orange, yellow, white
  // feel free to use another palette or define your own custom one
  CRGB color = ColorFromPalette(HeatColors_p, heatIndex);

  // fill the entire strip with the current color
  fill_solid(leds, NUM_LEDS, color);

  // slowly increase the heat
  EVERY_N_SECONDS(1) {
    // stop incrementing at 255, we don't want to overflow back to 0
    if(heatIndex > 0) {
      heatIndex--;
    }
  }
}

//
// ledSound
// Inspired by https://github.com/atuline/FastLED-SoundReactive
uint8_t squelch = 7;                                          // Anything below this is background noise, so we'll make it '0'.
int sample;                                                   // Current sample.
float sampleAvg = 0;                                          // Smoothed Average.
float micLev = 0;                                             // Used to convert returned value to have '0' as minimum.
uint8_t maxVol = 11;                                          // Reasonable value for constant volume for 'peak detector', as it won't always trigger.
bool samplePeak = 0;                                          // Boolean flag for peak. Responding routine must reset this flag.

static int16_t xdist;                                          // A random number for our noise generator.
static int16_t ydist;
uint16_t xscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint16_t yscale = 30;                                         // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 24;                                      // Value for blending between palettes.

CRGBPalette16 currentPalette(OceanColors_p);
CRGBPalette16 targetPalette(LavaColors_p);
TBlendType    currentBlending;                                // NOBLEND or LINEARBLEND 

void ledSoundLoop() {
  EVERY_N_MILLISECONDS(10) {
    uint8_t maxChanges = 24; 
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);   // AWESOME palette blending capability.
    fillnoise8();                                             // Update the LED array with noise based on sound input
    fadeToBlackBy(leds, NUM_LEDS, 32);                         // 8 bit, 1 = slow, 255 = fast
  }
 
  EVERY_N_SECONDS(5) {                                        // Change the target palette to a random one every 5 seconds.
    targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 255, random8(128,255)), CHSV(random8(), 192, random8(128,255)), CHSV(random8(), 255, random8(128,255)));
  }
  
  getSample();                                                // Sample the sound.

  FastLED.show();                                             // Display everything.
}

void getSample() {  
  int16_t micIn;                                              // Current sample starts with negative values and large values, which is why it's 16 bit signed.
  static long peakTime;
  
  micIn = analogRead(MIC_PIN);                                // Poor man's analog Read.
  micLev = ((micLev * 31) + micIn) / 32;                      // Smooth it out over the last 32 samples for automatic centering.
  micIn -= micLev;                                            // Let's center it to 0 now.
  micIn = abs(micIn);                                         // And get the absolute value of each sample.
  sample = (micIn <= squelch) ? 0 : (sample + micIn) / 2;     // Using a ternary operator, the resultant sample is either 0 or it's a bit smoothed out with the last sample.
  sampleAvg = ((sampleAvg * 31) + sample) / 32;               // Smooth it out over the last 32 samples.

  if (sample > (sampleAvg+maxVol) && millis() > (peakTime + 50)) {    // Poor man's beat detection by seeing if sample > Average + some value.
    samplePeak = 1;                                                   // Then we got a peak, else we don't. Display routines need to reset the samplepeak value in case they miss the trigger.
    peakTime=millis();                
  }                                                           
}  // getSample()

void fillnoise8() {                                                       // Add Perlin noise with modifiers from the soundmems routine.

  int maxLen = sampleAvg;
  if (sampleAvg >NUM_LEDS) maxLen = NUM_LEDS;

  for (int i = (NUM_LEDS-maxLen)/2; i <(NUM_LEDS+maxLen+1)/2; i++) {      // The louder the sound, the wider the soundbar.
    uint8_t index = inoise8(i*sampleAvg+xdist, ydist+i*sampleAvg);        // Get a value from the noise function. I'm using both x and y axis.
    leds[i] = ColorFromPalette(currentPalette, index, 255, LINEARBLEND);  // With that value, look up the 8 bit colour palette value and assign it to the current LED.
  }

  xdist=xdist+beatsin8(5,0,10);
  ydist=ydist+beatsin8(4,0,10);                                                                       
} 

//
// ledMood
//
void ledMoodLoop() {
  CRGBPalette16 palette;
  switch(int(env["mood"])) {
    case 0: palette = ForestColors_p;
      break;
    case 1: palette = OceanColors_p;
      break;
    case 2: palette = LavaColors_p;
      break;
    case 3: palette = RainbowStripeColors_p;
      break;
  }
  uint8_t beat = beatsin8( 60, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void ledManualLoop() {
  for(int i = 0; i < NUM_LEDS; i++) { 
    leds[i].setRGB( env["red"], env["green"], env["blue"]);
  }  
}

void ledClear() {
  for(int i = 0; i < NUM_LEDS; i++) { 
    leds[i] = CRGB::Black;
  }
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter(uint8_t chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}


void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
