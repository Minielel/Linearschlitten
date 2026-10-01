#include "ButtonDebounced.h"

// ---------- Pins (Trigorilla, X-Treiber) ----------
const uint8_t PIN_X_MOTOR_STEP   = 54;
const uint8_t PIN_X_MOTOR_DIR    = 55;
const uint8_t PIN_X_MOTOR_ENABLE = 38;  // LOW = Treiber aktiv

const uint8_t PIN_REFERENCE_BUTTON = 43;  // Taster gegen GND, interner Pullup
const int INDEX_OF_REFERENCE_BUTTON = 0;
const int NUMBER_OF_BUTTONS = 1;
const int REFERENCE_BUTTON_BOUNCE_TIME_IN_MILLISECONDS = 50;

// ---------- Hier einstellen ----------
const float DISTANCE_TO_TRAVEL_AWAY_FROM_BUTTON_IN_MILLIMETERS = 200.0;

// Schritte pro mm: Motorschritte pro Umdrehung * Mikroschritte / mm pro Umdrehung
// Beispiel: 200 * 16 / 40 mm (GT2-Riemen, 20 Zaehne) = 80 Schritte/mm
const float STEPS_PER_MILLIMETER = 400.0;

const unsigned long STEP_INTERVAL_IN_MICROSECONDS = 300;  // 500 µs = 2000 Schritte/s

// Richtungspegel: bei falscher Fahrtrichtung die beiden Werte tauschen
const uint8_t DIRECTION_LEVEL_TOWARDS_BUTTON   = HIGH;
const uint8_t DIRECTION_LEVEL_AWAY_FROM_BUTTON = LOW;

// ---------- Zustaende ----------
enum MotorState : uint8_t {
  MOTOR_STATE_MOVING_TOWARDS_BUTTON,
  MOTOR_STATE_MOVING_AWAY_FROM_BUTTON
};

MotorState currentMotorState = MOTOR_STATE_MOVING_TOWARDS_BUTTON;
long currentPositionInStepsAwayFromButton = 0;
long targetPositionInStepsAwayFromButton = 0;
unsigned long timestampOfLastStepInMicroseconds = 0;

void startMovingTowardsButton() {
  digitalWrite(PIN_X_MOTOR_DIR, DIRECTION_LEVEL_TOWARDS_BUTTON);
  currentMotorState = MOTOR_STATE_MOVING_TOWARDS_BUTTON;
  Serial.println("Fahre Richtung Taster...");
}

void startMovingAwayFromButton() {
  currentPositionInStepsAwayFromButton = 0;  // Taster erreicht = Position null
  targetPositionInStepsAwayFromButton =
    (long)(DISTANCE_TO_TRAVEL_AWAY_FROM_BUTTON_IN_MILLIMETERS * STEPS_PER_MILLIMETER);

  digitalWrite(PIN_X_MOTOR_DIR, DIRECTION_LEVEL_AWAY_FROM_BUTTON);
  currentMotorState = MOTOR_STATE_MOVING_AWAY_FROM_BUTTON;

  Serial.print("Position 0 gesetzt. Fahre ");
  Serial.print(DISTANCE_TO_TRAVEL_AWAY_FROM_BUTTON_IN_MILLIMETERS);
  Serial.println(" mm vom Taster weg...");
}

void doOneStep() {
  // Zwei digitalWrite hintereinander ergeben einen Puls von ca. 4 µs,
  // das reicht fuer A4988/DRV8825 (mind. ca. 1-2 µs)
  digitalWrite(PIN_X_MOTOR_STEP, HIGH);
  digitalWrite(PIN_X_MOTOR_STEP, LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_X_MOTOR_STEP, OUTPUT);
  pinMode(PIN_X_MOTOR_DIR, OUTPUT);
  pinMode(PIN_X_MOTOR_ENABLE, OUTPUT);
  pinMode(PIN_REFERENCE_BUTTON, INPUT_PULLUP);

  bd_init(NUMBER_OF_BUTTONS);
  bd_setButton(INDEX_OF_REFERENCE_BUTTON, PIN_REFERENCE_BUTTON, REFERENCE_BUTTON_BOUNCE_TIME_IN_MILLISECONDS);

  digitalWrite(PIN_X_MOTOR_ENABLE, LOW);
  startMovingTowardsButton();
}

void loop() {
  const bool referenceButtonIsPressed = (bd_getButton(INDEX_OF_REFERENCE_BUTTON) == LOW);

  // Taster erreicht: Position null setzen und in die andere Richtung fahren
  if (currentMotorState == MOTOR_STATE_MOVING_TOWARDS_BUTTON && referenceButtonIsPressed) {
    startMovingAwayFromButton();
    return;
  }

  // Schritt ausfuehren, sobald das Intervall abgelaufen ist
  const unsigned long currentTimeInMicroseconds = micros();
  if ((currentTimeInMicroseconds - timestampOfLastStepInMicroseconds) >= STEP_INTERVAL_IN_MICROSECONDS) {
    timestampOfLastStepInMicroseconds = currentTimeInMicroseconds;
    doOneStep();

    if (currentMotorState == MOTOR_STATE_MOVING_AWAY_FROM_BUTTON) {
      currentPositionInStepsAwayFromButton++;

      // Strecke geschafft: wieder zum Taster zurueckfahren
      if (currentPositionInStepsAwayFromButton >= targetPositionInStepsAwayFromButton) {
        Serial.println("Ziel erreicht.");
        startMovingTowardsButton();
      }
    }
  }
}