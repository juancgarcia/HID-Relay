//-------------------- For the soft Latch  ---------------------//
#include <Adafruit_NeoPixel.h>

#define LEDpin         5       // RGB LED connected to pin 5
#define NumOfLED       1       // Only have one RGB LED in this project
#define button         2       // Push Button connected to pin 2
#define softLatch      3       // NPN transistor's base of the soft latch module connected to pin 3
#define holdTime    1500       // How much time the button should be pressed to shut down
#define powerCheck     4       // Emitter of the power checking transistor

volatile boolean buttonPressed;             // Record the button state
volatile unsigned long pressedTime = 0;     // Time of the button press

unsigned long lastKeystroke = 0;            // Variable to store the time of the last key press
const long idleTime = 600000;               // Max idle time before arduino puts itself to sleep/shutdown(in milliseconds)

Adafruit_NeoPixel RGBLED = Adafruit_NeoPixel(NumOfLED, LEDpin, NEO_GRB + NEO_KHZ800);
//---------------------------------------------------------------//

#include <hidboot.h>
#include <usbhid.h>
#include <usbhub.h>

// #define _DEBUG
#define _COMBO

#ifdef _DEBUG
#define DEBUG_PRINT(str) Serial.print (str)
#define DEBUG_PRINTF(str, format) Serial.print (str, format)
#define DEBUG_PRINTLN(str)  Serial.println (str)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTF(x, format)
#define DEBUG_PRINTLN(x)
#endif

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

class HIDRelay : public HIDReportParser
{
  protected:
    virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

void HIDRelay::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  //    DEBUG_PRINT("Relay ");
  //    DEBUG_PRINTLN(len);

  for (uint8_t i = 0; i < len; i++) {
    Serial.write((uint8_t)buf[i]);

    //            DEBUG_PRINTF((uint8_t)buf[i], HEX);
    //            DEBUG_PRINT(",");

    //---- Whenever HIDRelay::Parse() called lastKeystroke record the time ------//
    lastKeystroke = millis();
    //---------------------------------------------------------------------------//
  }
  DEBUG_PRINT("\r\n");
};

class KbdRptParser : public HIDRelay
{
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

void KbdRptParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  //    DEBUG_PRINTLN("Kbd");

  Serial.write((uint8_t)0xFD); // BT-HID: Start Byte
  // Serial.write((uint8_t)0x09); // BT-HID: Data Length
  Serial.write((uint8_t)len + 1); // BT-HID: Data Length
  Serial.write((uint8_t)0x01); // BT-HID: Descriptor (1=Keyboard, 2=Mouse, 3=KybdMouse combo)
  // Keyboard Data consists of a single byte for modifiers (Ctrl, alt, etc)
  // Followed by 0x00
  // Followed by 6 key codes (any or all may be 0x00 for no key pressed)
  HIDRelay::Parse(hid, is_rpt_id, len, buf);
};

#ifdef _COMBO
class MouseRptParser : public HIDRelay
{
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

void MouseRptParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
//    DEBUG_PRINTLN("Mouse");
  //    DEBUG_PRINTLN("Mouse");
  Serial.write((uint8_t)0xFD); // BT-HID: Start Byte
  Serial.write((uint8_t)0x05); // BT-HID: Data Length
  // Serial.write((uint8_t)len+1); // BT-HID: Data Length
  Serial.write((uint8_t)0x02); // BT-HID: Descriptor (1=Keyboard, 2=Mouse, 3=KybdMouse combo)
  // Mouse Data consists of a single byte for pressed buttons
  // Followed by 0x00
  // Followed by 1 byte each for X then Y movement
  // Lastly 1 byte for wheel(optional?)
  HIDRelay::Parse(hid, is_rpt_id, len, buf);
  if (len < 4) {
    //      DEBUG_PRINTLN("ZeroPad");
    for (uint8_t i = 1; len + i < 5; i++)
      Serial.write((uint8_t)0x00);
  }
};
#endif

USB     Usb;
#ifdef _COMBO
USBHub     Hub(&Usb);
HIDBoot < USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE > HidComposite(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE>    HidMouse(&Usb);
#endif
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);

uint32_t next_time;

KbdRptParser KbdPrs;
#ifdef _COMBO
MouseRptParser MousePrs;
#endif

void setup() {
  Serial.begin( 115200 );

#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif

  DEBUG_PRINTLN("Start");

  if (Usb.Init() == -1)
    DEBUG_PRINTLN("OSC did not start.");

  delay( 200 );

  next_time = millis() + 5000;

#ifdef _COMBO
  HidComposite.SetReportParser(0, &KbdPrs);
  HidComposite.SetReportParser(1, &MousePrs);
  HidMouse.SetReportParser(0, &MousePrs);
#endif
  HidKeyboard.SetReportParser(0, &KbdPrs);

  //------------------- For the soft Latch part ---------------------//
  pinMode(button, INPUT);                       // Set push button as an input
  pinMode(softLatch, OUTPUT);                   // Set the transistor base pin as output
  pinMode(powerCheck, INPUT);                   // USB power check input
  digitalWrite(softLatch, HIGH);                // Hold the soft latch transistor HIGH to keep the micro controller turned on

  attachInterrupt(0, buttonEvent, FALLING);     // Get interrupts from the pin 2 (0) and call the "buttonEvent" function (ISR)
  buttonPressed = false;                        // Set the button state as UNPRESSED

  RGBLED.begin();                               // Initiate the NeoPixel library

  if (digitalRead(powerCheck) == HIGH) {                // Check whether the USB power connected when start up
    RGBLED.setPixelColor(0, RGBLED.Color(0, 0, 250));   // Blink color blue when a USB cable connected at startup
    RGBLED.show();                                      
    delay(700);
    RGBLED.setPixelColor(0, RGBLED.Color(0, 0, 0));     // No color
    RGBLED.show();
  }
  if (digitalRead(powerCheck) == LOW) {
    RGBLED.setPixelColor(0, RGBLED.Color(0, 250, 0));   // Blink color green to indicate powering from the battery
    RGBLED.show();                                      
    delay(700);
    RGBLED.setPixelColor(0, RGBLED.Color(0, 0, 0));     // No color
    RGBLED.show();
  }
  //-----------------------------------------------------------------//

}

void loop()
{
  //------ If the keyboard is idle for more than {idleTime} and if the keyboard powered from the battery, shut it down --------//
  if ( millis() - lastKeystroke >= idleTime) {
    if (digitalRead(powerCheck) == HIGH) {                // Powering from the USB
      // Sleep the arduino
    }
    if (digitalRead(powerCheck) == LOW) {                 // powering from the battery
      // turn off the Keyboard
      RGBLED.setPixelColor(0, RGBLED.Color(250, 0, 0));   // Color red
      RGBLED.show();                                      // This sends the updated pixel color to the hardware.
      delay(1000);
      digitalWrite(softLatch, LOW);                       // Shutdown by pulling the base of the soft latch transistor to ground
    }
  }
  //---------------------------------------------------------------------------------------------------------------------------//

  //------------------- For the soft Latch --------------------------//
  if (buttonPressed == true && millis() - pressedTime >= holdTime) {
    RGBLED.setPixelColor(0, RGBLED.Color(250, 0, 0));   // Color red
    RGBLED.show();                                      // This sends the updated pixel color to the hardware.
    delay(1000);
    digitalWrite(softLatch, LOW);                       // Shutdown by pulling the base of the transistor to ground

  }
  else {
    // Do somthing if the button released before the threshold time, like displaying battery status from the LED
  }
  //-----------------------------------------------------------------//
  Usb.Task();

}



//------------------- Soft Latch function(ISR) -------------------------//
void buttonEvent() {                            // Whenever a button interrupt occours, the buttonEvent function will call
  if (digitalRead(button) == LOW) {
    buttonPressed = true;
    pressedTime = millis();
  }
  if (digitalRead(button) == HIGH) {
    buttonPressed = false;
  }
}
//-----------------------------------------------------------------//





