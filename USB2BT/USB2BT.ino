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
    Serial.write((uint8_t)len+1); // BT-HID: Data Length
    Serial.write((uint8_t)0x01); // BT-HID: Descriptor (1=Keyboard, 2=Mouse, 3=KybdMouse combo)
    // Keyboard Data consists of a single byte for modifiers (Ctrl, alt, etc)
    // Followed by 0x00
    // Followed by 6 key codes (any or all may be 0x00 for no key pressed)
    HIDRelay::Parse(hid,is_rpt_id,len,buf);
};

#ifdef _COMBO
class MouseRptParser : public HIDRelay 
{
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

void MouseRptParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
//    DEBUG_PRINTLN("Mouse");
    Serial.write((uint8_t)0xFD); // BT-HID: Start Byte
    Serial.write((uint8_t)0x05); // BT-HID: Data Length
    // Serial.write((uint8_t)len+1); // BT-HID: Data Length
    Serial.write((uint8_t)0x02); // BT-HID: Descriptor (1=Keyboard, 2=Mouse, 3=KybdMouse combo)
    // Mouse Data consists of a single byte for pressed buttons
    // Followed by 0x00
    // Followed by 1 byte each for X then Y movement
    // Lastly 1 byte for wheel(optional?) 
    HIDRelay::Parse(hid,is_rpt_id,len,buf);
    if(len < 4){
//      DEBUG_PRINTLN("ZeroPad");
      for(uint8_t i=1; len+i<5; i++)
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

void setup()
{
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

}

void loop()
{
    Usb.Task();
}








