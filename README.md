# AB5396_minimal
Minimal SDK for Bluetrum AB5396

## how
This repo will be a minimal SDK for the Bluetrum BLE MCU AB5396 found in cheap USB BLE dongles[^1][^2].\
First step is the appoach taken by atc1441[^3] to run code in the Boot ROM callback.
When this works the firmware encryption needs to be figured out so code can be flashed to the chip, and
a proper startup has to be developed too.

A programmer for these chips is simply an UART with RX and TX connected by a 200 - 470 Ohm resistor, and
the RX side connected to the USB+ pin.

The Makefile uses `minichlink`[^4] to enter the bootloader by switching on 5v when the UART is waiting for
the sync byte, if you don't have a linkE programmer you can easily remove that and switch 5v to the chip manually.

Additional info can be found in [^5] and [^6].

## progress
- [x] Run from RAM using bootrom callback
- [ ] Reverse encryption scheme for firmware in flash
- [ ] Run from flash
- [ ] USB stack
- [ ] RF stack

### resources
[^1]: https://www.aliexpress.com/item/1005009109220784.html
[^2]: https://www.aliexpress.com/item/1005009109412274.html
[^3]: https://github.com/atc1441/Bluetrum_AB5682_Hacking
[^4]: https://github.com/cnlohr/ch32fun
[^5]: https://github.com/kagaimiq/bluetrum-tools
[^6]: https://github.com/ZhiqingLi/Sdk_Refresh
