// Import required libraries
#include <WiFi.h>
#include <aREST.h>
#include <FreeRTOS.h>

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
#define ssid        "AirVandalRTOS"
#define password    "EmbeddedSystems!19"

// Create an instance of the server
WiFiServer server(80);

// Declare functions to be exposed to the API
int ledControl(String command);

void setup()
{
    // Start Serial
    Serial.begin(9600);

    // Function to be exposed
    rest.function("led",ledControl);

    // pinmode(A13, OUTPUT);

    // Give name & ID to the device (ID should be 6 characters long)
    rest.set_id("1");
    rest.set_name("esp32");

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

    // Print the IP address
    Serial.print("Server started at ");
    Serial.println(WiFi.localIP());

}

void loop() {
    
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

// Custom function accessible by the API
int ledControl(String command) {

    // Get state from command
    int state = command.toInt();

    // digitalWrite(13,state);
    return state;
}
