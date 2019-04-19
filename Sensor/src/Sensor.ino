#include <Debounce.h>

const int ledPin = D7;
const int switchPin = D0;

const int heartbeatInterval = 5000;

int previousHeartbeatMillis = 0;

Debounce debouncer = Debounce(); 

// This will be saved to EEPROM and will continue to use the
// external antenna even if this line is removed. To switch back
// to the internal antenna, flash and run with ANT_INTERNAL.
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup()
{
    pinMode(ledPin, OUTPUT);

    debouncer.attach(switchPin, INPUT_PULLDOWN);
    debouncer.interval(250); // interval in ms

    Particle.publish("doorsensor-startup", PRIVATE);
}

void loop()
{
    debouncer.update();
    
    if (debouncer.fell())
    {
        Particle.publish("doorsensor-door-opened", PRIVATE);
    }
    
    if (debouncer.rose())
    {
        Particle.publish("doorsensor-door-closed", PRIVATE);
    }

    bool switchValue = !debouncer.read();
    digitalWrite(ledPin, switchValue);
    
    if (millis() - previousHeartbeatMillis >= heartbeatInterval)
    {
        previousHeartbeatMillis = millis();

        String data;
        if (switchValue)
        {
            data = "open";
        }
        else
        {
            data = "closed";
        }
        Particle.publish("doorsensor-heartbeat", data, 60, PRIVATE);
    }
}
