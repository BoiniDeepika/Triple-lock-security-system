#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>


SoftwareSerial mySerial(2, 3); 
Adafruit_Fingerprint finger(&mySerial);
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(10, 9); // RFID (SDA, RST)
Servo sg90;


constexpr uint8_t servoPin = 8;
constexpr uint8_t buzzerPin = 7;

// Default credentials
String defaultUID = "43 9A 71 30";
char defaultPassword[5] = "2517";
int defaultFingerprintID = 1;

// Updated credentials
String updatedUID = "";            
char updatedPassword[5] = "";      
int updatedFingerprintID = -1;     


char enteredPassword[5];           
char key;                          
int passwordIndex = 0;


bool RFIDMode = true, KeypadMode = false, FingerMode = false, UpdateMode = false;


const byte rows = 4, cols = 4;
char hexaKeys[rows][cols] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[rows] = {4, 5, 6, 7};
byte colPins[cols] = {A0, A1, A2, A3};
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, rows, cols);

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  mySerial.begin(57600);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.print("    Welcome    ");
  delay(3000);
  lcd.clear();

  sg90.attach(servoPin);
  sg90.write(0); 
  pinMode(buzzerPin, OUTPUT);

  finger.begin(57600);

  Serial.println("Setup completed, waiting for input...");
}

void soundBuzzer() {
  tone(buzzerPin, 5000, 2000);
  delay(500);
  noTone(buzzerPin);
}

void updateMode() {
  lcd.clear();
  lcd.print("Update Mode?");
  lcd.setCursor(0, 1);
  lcd.print("A:Yes B:No");

  while (true) {
    key = keypad.getKey();
    if (key == 'A') {
      lcd.clear();
      lcd.print("Update Options");
      lcd.setCursor(0, 1);
      lcd.print("1:UI 2:Pass 3:FP");
      while (true) {
        key = keypad.getKey();
        if (key == '1') { 
          lcd.clear();
          lcd.print("Scan new tag");
          while (!mfrc522.PICC_IsNewCardPresent());
          if (!mfrc522.PICC_ReadCardSerial()) continue;
          String newUID = "";
          for (byte i = 0; i < mfrc522.uid.size; i++) {
            newUID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            newUID += String(mfrc522.uid.uidByte[i], HEX);
          }
          newUID.toUpperCase();
          updatedUID = newUID; 
          lcd.clear();
          lcd.print("UID Updated");
          delay(2000);
          return;
        } else if (key == '2') { 
          lcd.clear();
          lcd.print("New Password:");
          for (int i = 0; i < 4; i++) {
            key = keypad.waitForKey();
            updatedPassword[i] = key;
            lcd.print('*');
          }
          updatedPassword[5] = '\0'; 
          lcd.clear();
          lcd.print("Password Set");
          delay(2000);
          return;
        } else if (key == '3') { 
          lcd.clear();
          lcd.print("Enroll Finger");
          delay(1000);
          while (!finger.getImage());
          finger.image2Tz();
          finger.createModel();
          finger.storeModel(defaultFingerprintID + 1); 
          updatedFingerprintID = defaultFingerprintID + 1;
          lcd.clear();
          lcd.print("FP Updated");
          delay(2000);
          return;
        }
      }
    } else if (key == 'B') {
      lcd.clear();
      lcd.print("No changes");
      delay(2000);
      return;
    }
  }
}

void loop() {
  if (RFIDMode) {
    lcd.setCursor(0, 0);
    lcd.print("  Lock System  ");
    lcd.setCursor(0, 1);
    lcd.print(" Scan Your Tag ");
    while (!mfrc522.PICC_IsNewCardPresent());
    if (!mfrc522.PICC_ReadCardSerial()) return;

    String tag = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      tag += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      tag += String(mfrc522.uid.uidByte[i], HEX);
    }
    tag.toUpperCase();

    if (tag.substring(1) == defaultUID || tag.substring(1) == updatedUID) { 
      lcd.clear();
      lcd.print("Tag Matched");
      lcd.setCursor(0, 1);
      lcd.print("Access Granted");
      delay(3000);
      lcd.clear();
      lcd.print("Enter Password:");
      RFIDMode = false;
      KeypadMode = true;
      passwordIndex = 0;
    } else {
      lcd.clear();
      lcd.print("Wrong Tag");
      lcd.setCursor(0, 1);
      lcd.print("Access Denied!");
      soundBuzzer();
      delay(3000);
      lcd.clear();
    }
  }

  if (KeypadMode) {
    key = keypad.getKey();
    if (key) {
      enteredPassword[passwordIndex++] = key;
      lcd.setCursor(passwordIndex - 1, 1);
      lcd.print('*');
      if (passwordIndex == 4) {
        enteredPassword[4] = '\0';
        if (strcmp(enteredPassword, defaultPassword) == 0 || strcmp(enteredPassword, updatedPassword) == 0) { 
          lcd.clear();
          lcd.print("Pass Matched");
          lcd.setCursor(0, 1);
          lcd.print("Access Granted");
          delay(3000);
          lcd.clear();
          lcd.print("Scan Finger...");
          KeypadMode = false;
          FingerMode = true;
        } else {
          lcd.clear();
          lcd.print("Wrong Password");
          lcd.setCursor(0, 1);
          lcd.print("Access Denied");
          soundBuzzer();
          delay(3000);
          passwordIndex = 0;
          lcd.clear();
          RFIDMode = true;
          KeypadMode = false;
        }
      }
    }
  }

  if (FingerMode) {
    lcd.setCursor(0, 0);
    lcd.print("Scan Finger...");
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      p = finger.image2Tz();
      if (p == FINGERPRINT_OK) {
        p = finger.fingerFastSearch();
        if (p == FINGERPRINT_OK && (finger.fingerID == defaultFingerprintID || finger.fingerID == updatedFingerprintID)) { // Check both fingerprints
          lcd.clear();
          lcd.print("Finger Matched");
          lcd.setCursor(0, 1);
          lcd.print("Access Granted");
          sg90.write(90); 
          delay(3000);
          lcd.clear();
          updateMode();
          sg90.write(0); 
          delay(3000);
          RFIDMode = true;
          FingerMode = false;
        } else {
          lcd.clear();
          lcd.print("Wrong Finger");
          lcd.setCursor(0, 1);
          lcd.print("Access Denied");
          soundBuzzer();
          delay(3000);
          RFIDMode = true;
          FingerMode = false;
        }
      }
    }
  }
}