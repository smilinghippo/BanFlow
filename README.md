# BanFlow
A Mass Flow Controller Board for old analog MFCs (+-15V supply, 5V control voltages).

More information is on the EasyEDA site:
* [A link to the schematic, and PCB layout](https://easyeda.com/m.bannerman/banflow)

This repository contains:
* [The FreeCAD 3D design files for the case](https://github.com/toastedcrumpets/BanFlow/blob/main/Box.FCStd)
* The STL files for 3D printing
* [The arduino code for the controller](https://github.com/toastedcrumpets/BanFlow/blob/main/Controller.ino)

The arduino code uses the following libraries:
* [Enhanced I2C library for Teensy 3.x & LC devices](https://github.com/nox771/i2c_t3)
* [Teensy ADC implementation library created by Pedro Villanueva](https://github.com/pedvide/ADC)
