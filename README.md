# scot-controller

Antenna Motion Controller for the SCOT.

The board is an adaptor board that fits on the back of an [STM32F746NG Discovery Development Board](https://www.st.com/en/evaluation-tools/32f746gdiscovery.html).

* STM32F746NG Microcontroller
* 4.3” RGB 480×272 color LCD-TFT with capacitive touch screen
* 100Mb/s Ethernet Interface

The firmware for the board uses ChibiOS and uGFX.

## Adaptor Board Features

* 9-36V DC Input
* MCP2562 CAN 2.0A/B Communication Interface

[**Circuit Board Schematic**](https://github.com/philcrump/scot-controller/raw/master/scot-controller-schematic.pdf)

### Adaptor PCB (CAD Render)

![PCB CAD Render](https://raw.githubusercontent.com/philcrump/scot-controller/master/scot-controller-cad.png)

#### Errata - April 2020

* Can Transceiver <-> Arduino footprint, RX / TX mapping is swapped.
