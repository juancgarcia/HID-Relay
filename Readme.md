# HID-Relay
Make wired USB devices Bluetooth-enabled.

Arduino project to pipe USB HID reports from a USB Host Shield out through a Bluetooth HID module. 

## Requirements

Arduino Pro Mini (3.3V version)

USB Host Shield mini

Bluetooth HID module with UART: RN-42 or a firmware converted HC-05/HC-06

Conversion Instructions [Parallel Programmer](https://www.youtube.com/watch?v=BBqsVKMYz1I) OR [USB FT232RL Programmer](https://youtu.be/fWXJDNcbZAA) (<-preferred)

## Software Setup

You need the Arduino/Genuino IDE (I used 1.6.8) and the latest release of the [USB Host Shield 2.0](https://github.com/felis/USB_Host_Shield_2.0) library. Be sure to get the library from github as the one pulled via the Arduino IDE's library downloader seemed to be out of date.

## Hardware Setup

### Powering the project

The Arduino, Bluetooth module (check your specs, some HC-05, and HC-06 may come on a breakout board expecting 5V), and USB Host shield all run off of 3.3V, but devices you want to plug into the USB connecter will be expecting 5V. To deal with this you'll want to supply the Arduino 5V via the RAW pin (it's regulator will then supply 3.3V to VCC feeding the Host Shield and Bluetooth module). Then route a wire from this 5V source to the Host Shield's power bus. A Bus power trace must also be cut on the Host Shield to prevent this 5V from damaging the 3.3V components there.

More detailed information here: https://www.circuitsathome.com/usb-host-shield-hardware-manual

### Assembly

The Arduino/Shield can be soldered together with either on top, but I prefer the Arduino up to maintain access to the reset button and pin assignment silkscreen. It's also a bit more compact as the Pro Mini will sit within the height of the Host Shield's usb connector.

I recommend against permanently soldering the bluetooth module into this project, or at least connecting it's power via a jumper or wire disconnect of some kind. Your programmer will be flashing the arduino on the same Rx/Tx pins your bluetooth module is listening on. Uploading a sketch is bound to fail if you can't cut the bluetooth power.

My solution here was to mount the ProMini/HostShield sandwich onto some perf board with female headers on either side. The bluetooth module got glued to a perfboard "shield" with male headers which was easy to plug in and out of the base.

## N-Key Rollover?

This project so far only seems to allow up to 6 simultaneous key-presses (minus modifier keys like ctrl,alt, etc, which are handled seperately). I'm not sure yet if this is a limitation of my particular keyboards reporting, or the host shield library possibly truncating reports. More research is needed.

## Additional Considerations

Currently I have tested just a few HID keyboards and mice with this project. All keyboards so far have all worked whether plain wired keyboards or with proprietary wireless dongles (Logitech unified recievers, and others), but mice have been inconsistent. One of my older logitech keyboard/mouse combo seemed to work perfectly with it's unifying reciever, while most other mice I've tried either failed to work altogether or present very drawn out or delayed cursor movements. This might be a speed limitation of the bluetooth module not being able to keep up with some of the faster mice's throughput. More research is needed.