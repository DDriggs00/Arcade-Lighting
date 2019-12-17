// Import required libraries
#include <WiFi.h>
#include <ctype.h>
#include <aREST.h>
#include <FreeRTOS.h>
#include <freertos/semphr.h>
#include <Adafruit_NeoPixel.h>  // Light control

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
// #define ssid        "AirVandalRTOS"
// #define password    "EmbeddedSystems!19"
#define ssid        "ddriggs-pixel"
#define password    "passworD1"

// Lighting string info
#define LED_PIN     13
#define LED_COUNT   30

// Create an instance of the server
WiFiServer server(80);

// Thread references
TaskHandle_t taskLighting;
TaskHandle_t taskNetwork;

// Threads
void network(void* pvParameter);
void lighting(void* pvParameter);

// Declare functions to be exposed to the API
int setLedState(String command);
int getLedState(String command);

// Color change functions
void doAnimation(int frameTime);
void colorWipe(uint32_t color, int wait);
void colorWipeAlt(uint32_t color1, uint32_t color2, uint8_t groupSize, int wait);
void colorSet(uint32_t color);
void colorSetAlt(uint32_t color1, uint32_t color2, uint8_t groupSize, uint8_t offset);
void theaterChase(uint32_t color, int wait);
void rainbow(int wait);
void theaterChaseRainbow(int wait);

// Global state variable
uint8_t ledState = 0;

// Semaphore for ^
SemaphoreHandle_t sem = xSemaphoreCreateMutex();

// LED Object
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

typedef enum games {
    pacman      = 20,
    digdug      = 21,
    mario       = 22,
    dk          = 23,
    dkjr        = 24,
    bubblebobble= 25,
    snowbros    = 26,
    frogger     = 27,
    mspacman    = 28
} games;

void setup()
{
    // Start Serial
    Serial.begin(9600);
    Serial.println("\nStarted initialization process");

    // Function to be exposed
    rest.function("getLedState",getLedState);
    rest.function("setLedState",setLedState);

    // Give name & ID to the device (ID should be 6 characters long)
    rest.set_id("1");
    rest.set_name("arcade-lighting");

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server.begin();

    // initialize lighting
    strip.begin();
    portDISABLE_INTERRUPTS(); 
    strip.show();
    portENABLE_INTERRUPTS();
    strip.setBrightness(50);   // Max Brightness == 255

    Serial.println("LED Strip initialized");

    // Print the IP address
    Serial.print("Server started at ");
    Serial.println(WiFi.localIP());
    
    // Create tasks
    xTaskCreatePinnedToCore(
        lighting,       // Function to implement the task
        "lighting",     // Name of the task
        10000,          // Stack size in words
        NULL,           // Task input parameter
        2,              // Priority of the task
        &taskLighting,  // Task handle.
        1);             // Core where the task should run

    xTaskCreatePinnedToCore(
        network,        // Function to implement the task
        "network",      // Name of the task
        10000,          // Stack size in words
        NULL,           // Task input parameter
        2,              // Priority of the task
        &taskNetwork,   // Task handle.
        0);             // Core where the task should run
    
    Serial.println("Tasks Created");
}

// Not used
void loop() {}

void network(void* pvParameter) {
    Serial.printf("Started networking tasks on core %i\n", xPortGetCoreID());

    while (true) {
        delay(1);
        // Handle REST calls
        WiFiClient client = server.available();
        if (!client) {
            continue;
        }
        while(!client.available()){
            delay(1);
        }
        rest.handle(client);
    }
}

void lighting(void* pvParameter) {
    Serial.printf("Started lighting tasks on core %i\n", xPortGetCoreID());
    int frameTime = 1000;
    uint8_t lastState = -1;
    while (true) {
        doAnimation(frameTime);

        int temp = -1;
        if( xSemaphoreTake( sem, ( TickType_t ) 100 ) == pdTRUE ) {
            temp = ledState;
            xSemaphoreGive(sem);
        }

        if (temp != lastState) {
            lastState = temp;
            switch (temp) {
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
                case 4: //Christmas
                    colorWipeAlt(strip.Color(255, 0, 0), strip.Color(0, 255, 0), 6, 50); // red/green crawl
                    frameTime = 500;
                    break;
                case games::bubblebobble:
                    colorWipeAlt(strip.Color(0, 0, 255), strip.Color(0, 255, 0), LED_COUNT/2, 50); // blue/green
                    break;
                case games::mario:
                    colorWipeAlt(strip.Color(255, 0, 0), strip.Color(0, 255, 0), LED_COUNT/2, 50); // Red/green
                    break;
                case games::digdug:
                    colorWipeAlt(strip.Color(0, 0, 255), strip.Color(255, 165, 0), 6, 50); // Blue/orange crawl
                    break;
                case games::mspacman:
                case games::pacman:
                    colorWipe(strip.Color(255, 255,   0), 50); // Yellow
                    break;
                case games::dkjr: // Donkey kong junior
                    colorWipe(strip.Color(34, 139,  34), 50); // forest green
                    break;
                default:
                    colorWipe(strip.Color(255, 255,   255), 50); // white
                    break;
            }
        }
        delay(1);
    }
}

// Custom function accessible by the API
int setLedState(String gameId) {
    int stateTemp;
    if (gameId.equalsIgnoreCase("pacman")) stateTemp = games::pacman;
    else if (gameId.equalsIgnoreCase("digdug")) stateTemp = games::digdug;
    else if (gameId.equalsIgnoreCase("bubblebobble")) stateTemp = games::bubblebobble;
    else if (gameId.equalsIgnoreCase("mario")) stateTemp = games::mario;
    else if (gameId.equalsIgnoreCase("donkeykong")) stateTemp = games::dk;
    else if (gameId.equalsIgnoreCase("dkjr")) stateTemp = games::dkjr;
    else if (gameId.equalsIgnoreCase("donkeykongjr")) stateTemp = games::dkjr;
    else if (gameId.equalsIgnoreCase("christmas")) stateTemp = 4;
    else stateTemp = gameId.toInt();

    // Set the global variable atomically
    if( xSemaphoreTake( sem, ( TickType_t ) 100 ) == pdTRUE ) {
        ledState = stateTemp;
        xSemaphoreGive(sem);
        return 0;
    }
    return -1;
}

// Custom function accessible by the API
int getLedState(String command) {
    int temp = -1;
    if( xSemaphoreTake( sem, ( TickType_t ) 100 ) == pdTRUE ) {
        temp = ledState;
        xSemaphoreGive(sem);
    }
    return temp;
}

void doAnimation(int frametime) {
    static unsigned long lastFrame = 0;
    static uint8_t frame = 0;
    
    if (millis() - lastFrame < frametime) return;
    lastFrame = millis();

    int temp = -1;
    if( xSemaphoreTake( sem, ( TickType_t ) 100 ) == pdTRUE ) {
        temp = ledState;
        xSemaphoreGive(sem);
    }

    switch (temp) {
        case 4:
            if (frame > 11) frame = 0;
            colorSetAlt(strip.Color(255, 0, 0), strip.Color(0, 255, 0), 6, frame);
            frame += 1;
            break;
        default:
            break;
    }
}


// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel.
// Args: Color, delay between pixels
void colorWipe(uint32_t color, int wait) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
        strip.show();
        delay(wait);                           //  Pause for a moment
    }
}

// Same as colorWipe, but alternating between 2 colors
void colorWipeAlt(uint32_t color1, uint32_t color2, uint8_t groupSize, int wait) {
    uint8_t currentGroupSize = 0;
    bool colorSelected = false;
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        if (currentGroupSize >= groupSize) {
            colorSelected = !colorSelected;
            currentGroupSize = 0;
        }
        if (colorSelected) {
            strip.setPixelColor(i, color2);
        }
        else {
            strip.setPixelColor(i, color1);
        }
        currentGroupSize += 1;
        strip.show();
        delay(wait);                           //  Pause for a moment
    }
}

// Seets the entire strand to the given color
// Args: Color
void colorSet(uint32_t color) {
    for(int i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
    }
    portDISABLE_INTERRUPTS(); 
    strip.show();
    portENABLE_INTERRUPTS();
}

// sets strip to anternating between color1 and color2 in groups on groupSize, rotate-shifted to the right by offset
void colorSetAlt(uint32_t color1, uint32_t color2, uint8_t groupSize, uint8_t offset) {
    uint8_t currentGroupSize = (offset % (2*groupSize));
    bool colorSelected = false;
    if (currentGroupSize >= groupSize) {
        colorSelected = !colorSelected;
        currentGroupSize -= groupSize;
    }
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        if (currentGroupSize >= groupSize) {
            colorSelected = !colorSelected;
            currentGroupSize = 0;
        }
        if (colorSelected) {
            strip.setPixelColor(i, color2);
        }
        else {
            strip.setPixelColor(i, color1);
        }
        currentGroupSize += 1;
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
            portDISABLE_INTERRUPTS(); 
            strip.show();
            portENABLE_INTERRUPTS();
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
        portDISABLE_INTERRUPTS(); 
        strip.show();
        portENABLE_INTERRUPTS();
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
            portDISABLE_INTERRUPTS(); 
            strip.show();                // Update strip with new contents
            portENABLE_INTERRUPTS();
            delay(wait);                 // Pause for a moment
            firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
        }
    }
}
