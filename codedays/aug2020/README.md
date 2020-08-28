## One Button to Save the World
## Intro
A bluetooth keyboard that uses a Mores code.
## Story
Once apon a time...

The very friendly cute race of Daleks found out that humans have created a keyboard made for 10 fingers. The daleks were overcome with anger and jealousy. Since the daleks did not develope a mental care proffesional assistence system, they resorted to violence. They decided that if humans did not create an all inclusive keyboard that they would destroy the Earth. 

So, I set out on a mission to save it. 
## Hardware
* NodeMcu 32s board based on ESP32 micro-controller;
* SSD1283A LCD module;
* An active buzzer and a led;
* Breadboard.
## Software
* Arduino
* ESP32 BLE keyboard library - https://github.com/T-vK/ESP32-BLE-Keyboard
* Graphics primitives library - https://github.com/lcdwiki/LCDWIKI_gui
* SSD1283A driver - https://github.com/ZinggJM/SSD1283A
### Tricks
* Space Key
Classic Morse code approach doesn't play well with computers. I decided to use a very long press for inserting a space.
* Enter Key
I am using "..-.." for the enter key
* ESC key (for VIM users)
Just use "----" sequence
* Capital letters
Just type a regular Morse code followed by a very long press
* Arrows Keys, Delete, Backspace, ...
Common, just use VIM and be happy without those keys!!! (!!! DALEKS LOVE VIM !!!)
