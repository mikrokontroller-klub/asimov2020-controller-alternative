#include <Arduino.h>

#define SWITCH1_PIN 2
#define SWITCH2_PIN 3
#define SWITCH3_PIN 4
#define SWITCH4_PIN 5
#define POTMETER_PIN A1
#define LEFT_BUTTON_PIN 6
#define RIGHT_BUTTON_PIN 7

#define POTMETER_MIN 0
#define POTMETER_MAX 930

struct StepperControlState
{
  byte direction;
  byte speedLevel;
};
struct ServoControlState
{
  byte angle;
  byte speed;
};
struct ControlState
{
  StepperControlState leftStepper;
  StepperControlState rightStepper;
  ServoControlState servo1;
  ServoControlState servo2;
  ServoControlState servo3;
  ServoControlState servo4;
};

struct SwitchState
{
  bool isEngaged;
};
struct PotmeterState
{
  byte value;
};
struct State
{
  SwitchState switch1;
  SwitchState switch2;
  SwitchState switch3;
  SwitchState switch4;
  SwitchState leftButton;
  SwitchState rightButton;
  PotmeterState potmeter;
};

class Switch
{
private:
  byte pin;
  byte state;
  byte lastReading;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;

public:
  Switch(byte pin)
  {
    this->pin = pin;
    lastReading = LOW;
    init();
  }
  void init()
  {
    pinMode(pin, INPUT);
    update();
  }
  void update()
  {
    byte newReading = digitalRead(pin);

    if (newReading != lastReading)
    {
      lastDebounceTime = millis();
    }
    if (millis() - lastDebounceTime > debounceDelay)
    {
      state = newReading;
    }
    lastReading = newReading;
  }
  byte getState()
  {
    update();
    return state;
  }
  bool isEngaged()
  {
    return (getState() == HIGH);
  }
};
class Potmeter
{
private:
  byte pin;
  byte value;
  byte lastReading;
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 50;

public:
  Potmeter(byte pin)
  {
    this->pin = pin;
    init();
  }
  void init()
  {
    pinMode(pin, INPUT);
    lastReading = LOW;
    update();
  }
  void update()
  {
    byte newReading = analogRead(pin);

    if (newReading != lastReading)
    {
      lastDebounceTime = millis();
    }
    if (millis() - lastDebounceTime > debounceDelay)
    {
      value = newReading;
    }
    lastReading = newReading;
  }
  byte getValue()
  {
    update();
    return value;
  }
};

Switch switch1(SWITCH1_PIN);
Switch switch2(SWITCH2_PIN);
Switch switch3(SWITCH3_PIN);
Switch switch4(SWITCH4_PIN);
Switch leftButton(LEFT_BUTTON_PIN);
Switch rightButton(RIGHT_BUTTON_PIN);
Potmeter potmeter(POTMETER_PIN);

struct State state;
boolean newData = false;

String serialise(struct ControlState ControlState)
{
  char openingMarker = '#';
  char closingMarker = '*';
  char splitDelimiter = ',';

  String payload;

  payload += openingMarker;
  payload += ControlState.leftStepper.direction;
  payload += splitDelimiter;
  payload += ControlState.leftStepper.speedLevel;
  payload += splitDelimiter;
  payload += ControlState.rightStepper.direction;
  payload += splitDelimiter;
  payload += ControlState.rightStepper.speedLevel;
  payload += splitDelimiter;
  payload += ControlState.servo1.angle;
  payload += splitDelimiter;
  payload += ControlState.servo2.angle;
  payload += splitDelimiter;
  payload += ControlState.servo3.angle;
  payload += splitDelimiter;
  payload += ControlState.servo4.angle;
  payload += closingMarker;

  return payload;
}

byte getDirection()
{
  if (state.switch1.isEngaged && !state.switch2.isEngaged)
  {
    return 1;
  }
  if (!state.switch1.isEngaged && state.switch2.isEngaged)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

byte calcSpeedLevel()
{
  byte speedLevel = map(state.potmeter.value, POTMETER_MIN, POTMETER_MAX, 0, 5);
  return speedLevel;
}

struct ControlState prepareControlState()
{
  struct ControlState controlState;

  if (state.leftButton.isEngaged)
  {
    controlState.leftStepper.direction = getDirection();
    controlState.leftStepper.speedLevel = calcSpeedLevel();
  }
  if (state.rightButton.isEngaged)
  {
    controlState.rightStepper.direction = getDirection();
    controlState.leftStepper.speedLevel = calcSpeedLevel();
  }

  return controlState;
}

bool hasStateChanged(struct State &newState)
{
  if (
      state.switch1.isEngaged == newState.switch1.isEngaged &&
      state.switch2.isEngaged == newState.switch2.isEngaged &&
      state.switch3.isEngaged == newState.switch3.isEngaged &&
      state.switch4.isEngaged == newState.switch4.isEngaged &&
      state.leftButton.isEngaged == newState.leftButton.isEngaged &&
      state.rightButton.isEngaged == newState.rightButton.isEngaged &&
      state.potmeter.value == newState.potmeter.value)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void listen()
{
  struct State newState;
  while (newData == false)
  {
    newState.switch1.isEngaged = switch1.isEngaged();
    newState.switch2.isEngaged = switch2.isEngaged();
    newState.switch3.isEngaged = switch3.isEngaged();
    newState.switch4.isEngaged = switch4.isEngaged();
    newState.leftButton.isEngaged = leftButton.isEngaged();
    newState.rightButton.isEngaged = rightButton.isEngaged();
    newState.potmeter.value = potmeter.getValue();
    if (hasStateChanged(newState))
    {
      state = newState;
      newData = true;
    }
    else
    {
      return;
    }
  }
}

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  listen();
  if (newData)
  {
    struct ControlState controlState = prepareControlState();
    String payload = serialise(controlState);
    Serial.println(payload);
    Serial.println("Dispatched");
    newData = false;
  }
}