#include <WiFi.h>               // Remote control
#include <Adafruit_NeoPixel.h>  // Light control
#include <aREST.h>              // Rest interface
#include <FreeRTOS.h>
#include <freertos/semphr.h>

// Lighting string info
#define LED_PIN     13
#define LED_COUNT   30

// WiFi parameters
#define ssid "GodBlessAmerica"
#define password "Ray3Tasha5Devin5Kierra6"

// Threads
void network();
void lighting();

// API functions
int setLedColorScheme(String command);

// Color change functions
void colorWipe(uint32_t color, int wait);
void theaterChase(uint32_t color, int wait);
void rainbow(int wait);
void theaterChaseRainbow(int wait);
void colorSet(uint32_t color);

// Global state variable
uint8_t ledState = 0;

SemaphoreHandle_t sem = xSemaphoreCreateMutex();

TaskHandle_t taskLighting;
TaskHandle_t taskNetwork;

// LED Object
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    // Initialize Serial Interface
    Serial.begin(115200);
    Serial.println("Initialization start");

    // Create tasks
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
    
    Serial.println("Tasks Created");

}

void network(void* parameter) {

    Serial.print("Network Task running on core ");
    Serial.println(xPortGetCoreID());
    
    // Create aREST instance
    aREST rest = aREST();
    
    // Set up Rest interface
    rest.function("setLEDColorScheme", setLedColorScheme);
    rest.set_id("1");
    rest.set_name("esp32");
    
    // Server Object
    WiFiServer server(80);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("WiFi connected");

    // Start the server
    server.begin();
    Serial.print("Server started at ");
    Serial.println(WiFi.localIP());

    while (true) {

        // Handle REST calls
        WiFiClient client = server.available();
        if (!client) {
            return;
            
        }
        while(!client.available()){
            delay(1);
        }
        rest.handle(client);
    }
}

void lighting(void* parameter) {

    Serial.print("Lighting Task running on core ");
    Serial.println(xPortGetCoreID());

    // initialize lighting
    strip.begin();
    strip.show();
    strip.setBrightness(50);   // Max Brightness == 255
    
    uint8_t lastState = 0;

    while (true) {
        if (ledState != lastState) {
            lastState = ledState;
            switch (ledState) {
                case 0:
                    colorWipe(strip.Color(  0,   0,   0), 50); // black
                    break;
                case 1:
                    colorWipe(strip.Color(255,   0,   0), 50); // Red
                    break;
                case 2:
                    colorWipe(strip.Color(  0, 255,   0), 50); // Green
                    break;
                case 3:
                    colorWipe(strip.Color(  0,   0, 255), 50); // Blue
                    break;
                case 4:
                    colorWipe(strip.Color(255, 255, 255), 50); // white
                    break;
                default:
                    colorWipe(strip.Color(255, 255,   0), 50); // Yellow
                    break;
            }
        }
    }
}

// Not used
void loop() {}

// REST-Accessible functions
int setLedColorScheme(String command) {
    if( xSemaphoreTake( sem, ( TickType_t ) 100 ) == pdTRUE ) {
        ledState = command.toInt();
        xSemaphoreGive(sem);
        return 0;
    }
    return 1;
}

// Other functions
void errorAlert(String message) {
    return; //TODO
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
