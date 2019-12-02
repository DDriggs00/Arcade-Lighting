#include <WiFi.h>               // Remote control
#include <Adafruit_NeoPixel.h>  // Light control
#include <aREST.h>              // Rest interface
#include <FreeRTOS.h>

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

void colorWipe(uint32_t color, int wait);
void theaterChase(uint32_t color, int wait);
void rainbow(int wait);
void theaterChaseRainbow(int wait);
void colorSet(uint32_t color);

void network();
void lighting();

void setLEDColor();

// Lighting string info
#define LED_PIN     13
#define LED_COUNT   30

uint8_t ledState = 0;

// WiFi parameters
const char* ssid = "GodBlessAmerica";
const char* password = "Ray3Tasha5Devin5Kierra6";

TaskHandle_t taskLighting;
TaskHandle_t taskNetwork;

// LED Object
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    
    xTaskCreatePinnedToCore(
        lighting,       // Function to implement the task
        "lighting",     // Name of the task
        10000,          // Stack size in words
        NULL,           // Task input parameter
        0,              // Priority of the task
        &taskLighting,  // Task handle.
        0);             // Core where the task should run

    xTaskCreatePinnedToCore(
        network,        // Function to implement the task
        "network",      // Name of the task
        10000,          // Stack size in words
        NULL,           // Task input parameter
        0,              // Priority of the task
        &taskNetwork,   // Task handle.
        1);             // Core where the task should run
     
    // initialize lighting
    strip.begin();
    strip.show();
    strip.setBrightness(255);   // Max Brightness == 255
}

void network(void* parameter) {

    Serial.print("Task1 running on core ");
    Serial.println(xPortGetCoreID());
    
    // Create aREST instance
    aREST rest = aREST();

    // Server Object
    WiFiServer server(80);

    
    // Function to be exposed
    rest.function("setLED", setLedColor);

    while (true) {

    }
}

void lighting(void* parameter) {

    while (true) {

    }
    colorWipe(strip.Color(255,   0,   0), 50); // Red
    colorWipe(strip.Color(  0, 255,   0), 50); // Green
    colorWipe(strip.Color(  0,   0, 255), 50); // Blue
}

void loop() {
    // theaterChase(strip.Color(127, 127, 127), 50); // White, half brightness
    // theaterChase(strip.Color(127,   0,   0), 50); // Red, half brightness
    // theaterChase(strip.Color(  0,   0, 127), 50); // Blue, half brightness
}


// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel.
// Args: Color, delay between pixels
void colorWipe(uint32_t color, int wait) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
        strip.show();                          //  Update strip to match
        delay(wait);                           //  Pause for a moment
    }
}

// Seets the entire strand to the given color
// Args: Color
void colorSet(uint32_t color) {
    for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
    for(int a=0; a<10; a++) {  // Repeat 10 times...
        for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
            strip.clear();         //   Set all pixels in RAM to 0 (off)
            // 'c' counts up from 'b' to end of strip in steps of 3...
            for(int c=b; c < strip.numPixels(); c += 3) {
                strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
            }
            strip.show(); // Update strip with new contents
            delay(wait);  // Pause for a moment
        }
    }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
    // Hue of first pixel runs 5 complete loops through the color wheel.
    // Color wheel has a range of 65536 but it's OK if we roll over, so
    // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
    // means we'll make 5*65536/256 = 1280 passes through this outer loop:
    for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
            // Offset pixel hue by an amount to make one full revolution of the
            // color wheel (range of 65536) along the length of the strip
            // (strip.numPixels() steps):
            int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
            // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
            // optionally add saturation and value (brightness) (each 0 to 255).
            // Here we're using just the single-argument hue variant. The result
            // is passed through strip.gamma32() to provide 'truer' colors
            // before assigning to each pixel:
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
    }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
    int firstPixelHue = 0;     // First pixel starts at red (hue 0)
    for(int a=0; a<30; a++) {  // Repeat 30 times...
        for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
            strip.clear();         //   Set all pixels in RAM to 0 (off)
            // 'c' counts up from 'b' to end of strip in increments of 3...
            for(int c=b; c<strip.numPixels(); c += 3) {
                // hue of pixel 'c' is offset by an amount to make one full
                // revolution of the color wheel (range 65536) along the length
                // of the strip (strip.numPixels() steps):
                int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
                uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
                strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
            }
            strip.show();                // Update strip with new contents
            delay(wait);                 // Pause for a moment
            firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
        }
    }
}
