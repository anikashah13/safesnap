#include <LiquidCrystal.h>

// =============================================================
// CandyCare Smart Pill Bottle — Arduino Uno
// MedTech Hack 2025-2026
//
// PURPOSE:
// Detects when a pill bottle cap is removed using a reed switch
// and magnet. Tracks time between openings to prevent accidental
// double dosing. Alerts user with LEDs, buzzer, and LCD display.
//
// HOW IT WORKS:
// - A neodymium magnet is mounted on the bottle cap
// - A reed switch is mounted on the bottle body at the same height
// - When cap is ON: magnet is near switch, pin reads LOW
// - When cap is OFF: magnet is away from switch, pin reads HIGH
// - A LOW -> HIGH transition = cap removed = dose attempt detected
// - If enough time has passed since last dose: green LED + "Dose OK"
// - If too soon since last dose: red LED + buzzer + "TOO SOON!"
//
// PIN ASSIGNMENTS:
// Pin 2  -> LCD D7
// Pin 3  -> LCD D6
// Pin 4  -> LCD D5
// Pin 5  -> LCD D4
// Pin 6  -> LCD E
// Pin 7  -> LCD RS
// Pin 8  -> Green LED (dose OK indicator)
// Pin 9  -> Red LED (double dose warning)
// Pin 10 -> Piezo Buzzer (audio alert)
// Pin 11 -> Reed Switch (cap open/close sensor)
//
// LCD WIRING:
// LCD Pin 1 (VSS) -> GND
// LCD Pin 2 (VDD) -> 5V
// LCD Pin 3 (V0)  -> Potentiometer wiper (contrast adjustment)
// LCD Pin 15 (A)  -> 5V (backlight power)
// LCD Pin 16 (K)  -> GND (backlight ground)
// =============================================================

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// --- Component Pin Definitions ---
const int reedPin = 11;   // Reed switch input (INPUT_PULLUP)
const int greenLED = 8;   // Green LED output (dose OK)
const int redLED = 9;     // Red LED output (too soon warning)
const int buzzer = 10;    // Piezo buzzer output (audio alert)

// --- Timing Variables ---
// Using 'long' instead of 'unsigned long' so lastOpenTime can be
// initialized to a negative value, guaranteeing first open is always green
long doseInterval = 10000;          // Minimum time between doses in ms (10s for testing, change to 28800000 for 8 hours)
long lastOpenTime = -doseInterval;  // Initialized negative so first open always passes the time check

// --- Debounce Variables ---
// Debouncing prevents false triggers from reed switch vibrations
// or partial magnet movements. The pin must stay in its new state
// for debounceDelay ms continuously before it registers as a real event.
unsigned long lastDebounceTime = 0;       // Timestamp of last pin state change
const unsigned long debounceDelay = 50;   // ms pin must be stable to register (increase if still noisy)
bool stableState = LOW;                   // Last confirmed stable pin state
bool previousState = LOW;                 // Previous confirmed state (used to detect transitions)

void setup() {
  // --- Pin Modes ---
  pinMode(reedPin, INPUT_PULLUP);  // INPUT_PULLUP means pin reads HIGH when switch is open (magnet away)
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // --- Initialize debounce states to actual pin state at boot ---
  // This prevents a phantom trigger if the magnet happens to be
  // away from the switch when the Arduino powers on
  previousState = digitalRead(reedPin);
  stableState = previousState;

  // --- Startup Display ---
  lcd.begin(16, 2);          // Initialize 16x2 LCD
  lcd.print("CandyCare v1"); // Show splash screen
  delay(2000);               // Hold splash for 2 seconds
  lcd.clear();
  lcd.print("Ready");        // Show idle state
}

void loop() {
  bool currentState = digitalRead(reedPin);  // Read current reed switch state

  // --- Debounce Step 1: Reset timer if pin state is changing ---
  // Any time the current reading differs from the last stable state,
  // reset the debounce timer. We won't accept this as a real change
  // until it stays consistent for debounceDelay ms.
  if (currentState != stableState) {
    lastDebounceTime = millis();
  }

  // --- Debounce Step 2: Accept state change only after stable period ---
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // Pin has been stable long enough — check if it actually changed
    if (currentState != previousState) {
      previousState = currentState;  // Update previous state to new confirmed state

      // --- Dose Detection: Cap Removed (LOW -> HIGH transition) ---
      // HIGH means magnet is away from reed switch = cap is off = dose attempt
      if (currentState == HIGH) {

        long currentTime = millis();  // Capture time of this opening event
        lcd.clear();

        if (currentTime - lastOpenTime < doseInterval) {
          // --- TOO SOON: Not enough time since last dose ---
          // Do NOT update lastOpenTime — clock keeps running from last valid dose
          lcd.setCursor(0, 0);
          lcd.print("TOO SOON!");
          lcd.setCursor(0, 1);
          lcd.print("Wait more time");

          digitalWrite(redLED, HIGH);  // Red LED on
          tone(buzzer, 1000);          // 1000Hz buzzer tone
          delay(2000);                 // Hold alert for 2 seconds
          noTone(buzzer);              // Stop buzzer
          digitalWrite(redLED, LOW);   // Red LED off

        } else {
          // --- SAFE DOSE: Enough time has passed since last dose ---
          lastOpenTime = currentTime;  // Record this as the new last dose time

          lcd.setCursor(0, 0);
          lcd.print("Dose OK");
          lcd.setCursor(0, 1);
          lcd.print("Recorded");

          digitalWrite(greenLED, HIGH);  // Green LED on
          delay(2000);                   // Hold confirmation for 2 seconds
          digitalWrite(greenLED, LOW);   // Green LED off
        }

        // --- Return to idle state ---
        lcd.clear();
        lcd.print("Ready");
      }
      // Note: cap being put back on (HIGH -> LOW) is intentionally ignored
    }
  }

  // --- Debounce Step 3: Always update stable state each loop ---
  stableState = currentState;
}