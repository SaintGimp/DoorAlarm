#include <Debounce.h>

int armingStatusPin = D0;
int speakerPin = D1;
int doorStatusPin = D2;
int armingPin = D4;

const int alarmInterval = 500;
const int alarmDuration = 5000;
const int heartbeatTimeoutInterval = 5 * 60 * 1000;
const int armingStatusBrightness = 2;

bool alarmEnabled = false;
bool soundOn = false;
int previousToneChangeMillis = 0;
int alarmStartMillis = 0;
retained bool alarmArmed;
int previousClosedHeartbeatMillis = 0;

Debounce armingDebouncer = Debounce(); 
LEDStatus status(0x00000000);

// This will be saved to EEPROM and will continue to use the
// external antenna even if this line is removed. To switch back
// to the internal antenna, flash and run with ANT_INTERNAL.
STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup()
{
    pinMode(armingStatusPin, OUTPUT);
    pinMode(doorStatusPin, OUTPUT);
    // The piezo speaker makes a slight amount of noise whenever the
    // pin is in output mode, so we don't do it here but rather
    // when we want to start sounding the alarm.
    
    armingDebouncer.attach(armingPin, INPUT_PULLDOWN);
    armingDebouncer.interval(50); // interval in ms
    
    Particle.subscribe("doorsensor-door-opened", doorOpenedHandler, MY_DEVICES);
    Particle.subscribe("doorsensor-door-closed", doorClosedHandler, MY_DEVICES);
    Particle.subscribe("doorsensor-heartbeat", heartbeatHandler, MY_DEVICES);
    
    digitalWrite(doorStatusPin, HIGH);

    status.setActive(LED_PRIORITY_NORMAL);
}

void loop()
{
    processArming();
    processAlarm();
    
    if (millis() - previousClosedHeartbeatMillis >= heartbeatTimeoutInterval)
    {
        digitalWrite(doorStatusPin, HIGH);
    }
}

void processArming()
{
    armingDebouncer.update();

    if (armingDebouncer.rose())
    {
        alarmArmed = !alarmArmed;

        // TODO: use a scheduler
        analogWrite(armingStatusPin, 255);
        delay(alarmArmed ? 1000 : 100);
    }
    
    if (alarmArmed)
    {
        analogWrite(armingStatusPin, armingStatusBrightness);
    }
    else
    {
        analogWrite(armingStatusPin, 0);
    }
    
    // Disarm at 5:30 AM PST
    if (Time.hour() == 13 && Time.minute() >= 30)
    {
        alarmArmed = false;
    }
}

void processAlarm()
{
    if (alarmEnabled && millis() - previousToneChangeMillis >= alarmInterval)
    {
        previousToneChangeMillis = millis();
        
        if (soundOn)
        {
            analogWrite(speakerPin, 0);
            soundOn = false;
        }
        else
        {
            analogWrite(speakerPin, 128);
            soundOn = true;
        }
    }
    
    if (millis() - alarmStartMillis >= alarmDuration)
    {
        stopAlarm();
    }
}

void startAlarm()
{
    alarmEnabled = true;
    soundOn = true;
    pinMode(speakerPin, OUTPUT);
    analogWrite(speakerPin, 128);
    alarmStartMillis = previousToneChangeMillis = millis();
}

void stopAlarm()
{
    analogWrite(speakerPin, 0);
    pinMode(speakerPin, INPUT);
    soundOn = false;
    alarmEnabled = false;
}

void doorOpenedHandler(const char* event, const char* data)
{
    digitalWrite(doorStatusPin, HIGH);
    
    if (alarmArmed)
    {
        startAlarm();
    }
}

void doorClosedHandler(const char* event, const char* data)
{
    digitalWrite(doorStatusPin, LOW);
}

void heartbeatHandler(const char* event, const char* data)
{
    if (data[0] == 'c')
    {
        digitalWrite(doorStatusPin, LOW);
        previousClosedHeartbeatMillis = millis();
    }
}
