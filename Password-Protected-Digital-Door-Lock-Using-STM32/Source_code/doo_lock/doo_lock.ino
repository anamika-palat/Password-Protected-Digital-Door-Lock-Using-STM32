#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// KEYPAD
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {PA0, PA1, PA2, PA3};
byte colPins[COLS] = {PA4, PA5, PA6, PA7};

Keypad keypad = Keypad(
  makeKeymap(keys), rowPins, colPins, ROWS, COLS
);

// PINS
const int RELAY_PIN = PA8;
const int BUZZER_PIN = PB0;

// Set true for active-LOW relay modules.
// If relay works in reverse, test false.
const bool RELAY_ACTIVE_LOW = true;

// PASSWORD
String password = "1234";
String entered = "";
String newPassword = "";
String confirmPassword = "";

enum State {
  LOCKED,
  UNLOCKED,
  OLD_PASSWORD,
  NEW_PASSWORD,
  CONFIRM_PASSWORD
};

State state = LOCKED;

unsigned long unlockStarted = 0;
const unsigned long UNLOCK_TIME = 5000;

// RELAY
void relayON() {
  digitalWrite(RELAY_PIN,
    RELAY_ACTIVE_LOW ? LOW : HIGH);
}

void relayOFF() {
  digitalWrite(RELAY_PIN,
    RELAY_ACTIVE_LOW ? HIGH : LOW);
}

// BUZZER: passive buzzer
void beep(int frequency, int duration) {
  unsigned long period = 1000000UL / frequency;
  unsigned long half = period / 2;
  unsigned long cycles =
      ((unsigned long)duration * 1000UL) / period;

  for (unsigned long i = 0; i < cycles; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(half);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(half);
  }
}

void successSound() {
  beep(2000, 150);
  delay(80);
  beep(2500, 150);
}

void wrongSound() {
  for (byte i = 0; i < 3; i++) {
    beep(1000, 300);
    delay(100);
  }
}

void showScreen() {
  entered = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
  lcd.setCursor(0, 1);
  lcd.print("Code: ");
}

void showEntry() {
  lcd.setCursor(0, 1);
  lcd.print("Code: ");
  for (byte i = 0; i < entered.length(); i++) {
    lcd.print('*');
  }
  for (byte i = entered.length(); i < 10; i++) {
    lcd.print(' ');
  }
}

void unlockDoor() {
  relayON();
  state = UNLOCKED;
  unlockStarted = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Granted");
  lcd.setCursor(0, 1);
  lcd.print("Door Unlocked");

  successSound();
  Serial.println("Access Granted");
}

void lockDoor() {
  relayOFF();
  state = LOCKED;
  entered = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Door Locked");
  delay(800);
  showScreen();

  Serial.println("Door Locked");
}

void wrongPassword() {
  relayOFF();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Denied");
  lcd.setCursor(0, 1);
  lcd.print("Wrong Password");

  Serial.println("Wrong Password");
  wrongSound();

  delay(800);
  state = LOCKED;
  showScreen();
}

void beginChange() {
  entered = "";
  newPassword = "";
  state = OLD_PASSWORD;

  lcd.clear();
  lcd.print("Old Password:");
  showEntry();
}

void processSubmit() {
  switch (state) {
    case LOCKED:
      if (entered == password) {
        entered = "";
        unlockDoor();
      } else {
        entered = "";
        wrongPassword();
      }
      break;

    case OLD_PASSWORD:
      if (entered == password) {
        entered = "";
        state = NEW_PASSWORD;
        lcd.clear();
        lcd.print("New Password:");
        showEntry();
      } else {
        entered = "";
        wrongPassword();
      }
      break;

    case NEW_PASSWORD:
      if (entered.length() == 4) {
        newPassword = entered;
        entered = "";
        state = CONFIRM_PASSWORD;

        lcd.clear();
        lcd.print("Confirm Password");
        showEntry();
      } else {
        entered = "";
        lcd.clear();
        lcd.print("Use 4 Digits");
        delay(800);
        lcd.clear();
        lcd.print("New Password:");
        showEntry();
      }
      break;

    case CONFIRM_PASSWORD:
      if (entered == newPassword &&
          entered.length() == 4) {
        password = newPassword;
        lcd.clear();
        lcd.print("Password Changed");
        successSound();
        Serial.println("Password changed");
      } else {
        lcd.clear();
        lcd.print("Password Mismatch");
        wrongSound();
      }

      entered = "";
      newPassword = "";
      state = LOCKED;
      delay(1000);
      showScreen();
      break;

    case UNLOCKED:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  relayOFF();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.setSCL(PB6);
  Wire.setSDA(PB7);
  Wire.begin();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("PASSWORD DOOR");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1200);

  showScreen();
}

void loop() {
  if (state == UNLOCKED &&
      millis() - unlockStarted >= UNLOCK_TIME) {
    lockDoor();
  }

  char key = keypad.getKey();
  if (!key) return;

  if (state == UNLOCKED) {
    if (key == 'B') {
      lockDoor();
    }
    return;
  }

  if (key == '*') {
    entered = "";
    if (state == OLD_PASSWORD ||
        state == NEW_PASSWORD ||
        state == CONFIRM_PASSWORD) {
      state = LOCKED;
    }
    showScreen();
    return;
  }

  if (key == 'A' && state == LOCKED) {
    beginChange();
    return;
  }

  if (key == '#' ) {
    if (entered.length() > 0) {
      processSubmit();
    }
    return;
  }

  if (key >= '0' && key <= '9' &&
      entered.length() < 4) {
    entered += key;
    showEntry();
  }
}