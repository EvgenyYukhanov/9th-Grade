/*
 * Project: One Button to Save the World.
 *
 * Description: Bluetooth keyboard with a single contact key,
 * buzzer and LED (ideal for Dalek).
 *
 * Requirements:
 * - Learn Morse Code
 * - Learn vim for better experience
 * 
 * Virtual CodeDay 2020
**/

#include <LCDWIKI_GUI.h>
#include <SSD1283A.h>
#include <BleKeyboard.h>

// Colors for the screen
// (seems to be RGB with 9 bits per color)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// How often to check if the keyboard is connected
const int kKeyboardConnectionRetryMs = 500;

// bound rate for console
const int kBoudRate = 115200;

// PIN connected to the button with pull-down resistor
const int kButtonPin = 27;

// PIN connected to the LED and/or active buzzer
const int kLedPin = 12;

// Max length of the long press. Everything above is
// Shift or Space
const int kLongPressMs = 800;

// Time after which we consider a pause to be the letter
// completion
const int kButtonCompletionMs = 1000;

// Max length of the short press.
const int kShortPressMs = 200;

// The key may produce wrong value righ after been pressed/released.
// Do not react on events shorted than this value.
const int kNoiseReductionMs = 30;

// Morse code -> key mapping
const std::map<std::string, char> kMapping = {
  {".-", 'a'},
  {"-...", 'b'},
  {"-.-.", 'c'},
  {"-..", 'd'},
  {".", 'e'},
  {"..-.", 'f'},
  {"--.", 'g'},
  {"....", 'h'},
  {"..", 'i'},
  {".---", 'j'},
  {"-.-", 'k'},
  {".-..", 'l'},
  {"--", 'm'},
  {"-.", 'n'},
  {"---", 'o'},
  {".--.", 'p'},
  {"--.-", 'q'},
  {".-.", 'r'},
  {"...", 's'},
  {"-", 't'},
  {"..-", 'u'},
  {"...-", 'v'},
  {".--", 'w'},
  {"-..-", 'x'},
  {"-.--", 'y'},
  {"--..", 'z'},
  {".----", '1'},
  {"..---", '2'},
  {"...--", '3'},
  {"....-", '4'},
  {".....", '5'},
  {"-....", '6'},
  {"--...", '7'},
  {"---..", '8'},
  {"----.", '9'},
  {"-----", '0'},
  {"--..--", ','},
  {".-.-.-", '.'},
  {"..--..", '?'},
  {"...-..", '!'},
  {".----.", ':'},
  {"-..-.", '/'},
  {"-....-", '-'},
  {"-.--.", '('},
  {"-.--.-", ')'},
  {"----", KEY_ESC},
  {"..-..", KEY_RETURN},
};


// Name millisends as ms
using ms = unsigned long;

// The BLE keyboard
BleKeyboard bleKeyboard("Dalek Keyboard", "Evgeny");

// The display
SSD1283A_GUI lcd(5, 2, 4, -1); //hardware spi,cs,cd,reset,led

// Indicates if the BLE keyboard was connected during the
// previous check.
// Hack: let's put the initial state to <code>true</code> to
// make the loop to write 'WAIT' message?
bool connected = true;

// The previous state of the button. Actually, the reading
// on the PIN. LOW - button released, HIGH - button is pressed.
int previousButtonState = LOW;

// Time (in ms since the device startup) when the button
// state changed the last time.
ms buttonStateChange = 0;

// Current sequence of '-' and '.'.
std::string currentSeq;

// Reads the button state, updates currentSeq and returns the
// key ready to be send to the computer. If no key is ready,
// returns 0.
char get_key();

// Prints or hides BLE keyboard connection wait message
void print_wait_message(bool print);

// Prints or hides the current sequence of '-' and '.'.
void print_sequence(bool print);

// Translates the current morse sequence into a key or 0 if
// the sequence is invalid.
char translate();

// Initialize the controller (called once)
void setup() {
  // setup the button and the led
  pinMode(kLedPin, OUTPUT);
  pinMode(kButtonPin, INPUT);

  // setup console output
  Serial.begin(kBoudRate);
  Serial.println("Starting morse keyboard!");

  // start keyboard
  bleKeyboard.begin();

  // initialize screen and put caption
  lcd.init();
  
  lcd.setRotation(0);
  lcd.Fill_Screen(BLACK);
  lcd.Set_Text_colour(YELLOW);
  lcd.Set_Text_Back_colour(BLACK);
  lcd.Set_Text_Size(1);
  lcd.Print_String("Dalek Keyboard", 0, 0);
}

// Called very often
void loop() {
  // check if the keyboard connection state changed
  if(bleKeyboard.isConnected() != connected) {
    connected = bleKeyboard.isConnected();
    print_wait_message(!connected);
    if (!connected) {
      delay(kKeyboardConnectionRetryMs);
      return;
    }
  }

  // if key is ready, simulate button press/release
  char key = get_key();
  if (key) {
    bleKeyboard.press(key);
    delay(100);
    bleKeyboard.releaseAll();
  }

  delay(1);
}

void print_wait_message(bool print) {
  lcd.Set_Text_colour(print ? RED : BLACK);
  lcd.Set_Text_Back_colour(BLACK);
  lcd.Set_Text_Size(3);
  lcd.Print_String("WAIT", 30 , 55);
}

void print_sequence(bool print) {
  lcd.Set_Text_colour(print ? YELLOW : BLACK);
  lcd.Set_Text_Back_colour(BLACK);
  lcd.Set_Text_Size(3);
  lcd.Print_String(currentSeq.c_str(), 0, 60);
}

char translate() {
  char key = 0;
  const auto it = kMapping.find(currentSeq);
  if(kMapping.end() != it) {  // "not found" in C++
    key = it->second;  // OMG...
  }

  print_sequence(false);
  currentSeq = ""; // start new sequence after this
  return key;
}

char get_key() {
  // Read button state: LOW - released, HIGH (+3v) - pressed
  const int buttonState = digitalRead(kButtonPin);
  
  // blink and make some noise
  digitalWrite(kLedPin, buttonState);

  // do not react if state changes too fast - electrical noise
  if (buttonState != previousButtonState && millis() - buttonStateChange < kNoiseReductionMs) {
    previousButtonState = buttonState;
    buttonStateChange = millis();
    return 0;
  }
  
  if (buttonState == LOW && previousButtonState == LOW && millis() - buttonStateChange > kButtonCompletionMs) {
    // we did nothing for too long - consider the Morse sequence to be completed
    return translate();
  }
   
  if (buttonState == LOW && previousButtonState == HIGH) {
    // the button is released, let's decide if it was short, long or extra long
    ms length = millis() - buttonStateChange;
    previousButtonState = LOW;
    buttonStateChange = millis();
    if (length > kLongPressMs) {
      // extra long press
      char key = translate();
      if(key == 0) {
        // nothing else was pressed - send space
        Serial.println(" ");
        return ' ';
      }
      
      if (key >= 'a' && key <= 'z') {
        // a letter was pressed before the extra long press - convert to upper case
        key = 'A' + (key - 'a');
      }
      
      return key;
    }
    
    if (length > kShortPressMs) {
      // long press
      currentSeq = currentSeq + '-';
      Serial.print("-");
    } else {
      // short press
      Serial.print(".");
      currentSeq = currentSeq + '.';
    }
     
    print_sequence(true);
    return 0;
  }

  if (buttonState != previousButtonState) {
    previousButtonState = buttonState;
    buttonStateChange = millis();
    return 0;
  }
   
  return 0;
}
