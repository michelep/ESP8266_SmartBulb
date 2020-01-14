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
#define NUM_LEDS 16

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
  FastLED.setBrightness(env["brightness"]);
}

// Callback LedGame changing
void lgCallback() {
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void ledLoop() {
  switch(int(env["mode"])) {
    case LED_MODE_GAME: gPatterns[gCurrentPatternNumber]();
      EVERY_N_MILLISECONDS( 20 ) { gHue++; } 
      break; 
    case LED_MODE_MOOD: ledMood();
      break;
    case LED_MODE_MANUAL: ledManual();
      break;
    default: ledClear();
      break;
  }
    
  FastLED.setBrightness(env["brightness"]);
  FastLED.show();  
  FastLED.delay(1000/env["speed"]); 
}

void ledMood() {
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

void ledManual() {
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
