# FFBArcadePlugin

C++ plugin for adding force feedback support to arcade games.

This code uses GNU GENERAL PUBLIC LICENSE & License is supplied.


# FFBArcadePlugin - JogCon
This project is modified to send FFB commands to an Namco JogCon controller.

Controller must be connected to an Arduino with USB output to PC.
FFBArcadePlugin will send command to the controller by the Arduino's serial com port.

It works and it's fun to play with. But this is just a proof-of-concept project.

The JogCon is not the ideal controller to use as an FFB "racing wheel".
The controller will disable it's motor's if there's no button state change for an amount of time.

Also my Arduino died and I really can't test this sketch or make it better.
Only tested with MAME (x64).

### About the arduino sketch:
Sorgelig's [JogConUSB](https://github.com/MiSTer-devel/Retro-Controllers-USB-MiSTer/tree/master/JogConUSB) was used as a base for "JogCon as a wheel controller".<br/>
Uses a modified version of [Joystick](https://github.com/MHeironimus/ArduinoJoystickLibrary) class by MHeironimus (inlcuded with sketch).<br/>
Uses a modified version of [PsxNewLib](https://github.com/SukkoPera/PsxNewLib) class by SukkoPera (forked [here](https://github.com/sonik-br/PsxNewLib)).<br/>
JogCon commands based on the work by RandomInsano's [pscontroller-rs](https://github.com/RandomInsano/pscontroller-rs).


### How to use:
Follow the orinal FFBArcadePlugin installation instructions but it's not needed to configure the racing wheel on the interface. Instead configure the serial port on the ini file like this:
JogconSerialPort=\\.\COM10
