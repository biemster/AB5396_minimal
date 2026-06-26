# AB5396_minimal
Minimal SDK for Bluetrum AB5396

## how
This repo will be a minimal SDK for the Bluetrum BLE MCU AB5396 found in cheap USB BLE dongles[^4][^5].
First step is the appoach taken by atc1441[^1] to run code in the Boot ROM callback.
Then the firmware encryption needs to be figured out so code can be flashed to the chip, and
a proper startup has to be developed too.

The Makefile uses `minichlink`[^6] to enter the bootloader by switching on 5v when the UART is waiting for
the sync byte, if you don't have a linkE programmer you can easily remove that and switch 5v to the chip manually.

## progress
- [x] Run from RAM using bootrom callback
- [ ] Reverse encryption scheme for firmware in flash
- [ ] Run from flash
- [ ] USB stack
- [ ] RF stack

### resources
[^1] https://github.com/atc1441/Bluetrum_AB5682_Hacking
[^2] https://github.com/kagaimiq/bluetrum-tools
[^3] https://github.com/ZhiqingLi/Sdk_Refresh
[^4] https://www.aliexpress.com/item/1005009109220784.html
[^5] https://www.aliexpress.com/item/1005009109412274.html
[^6] https://github.com/cnlohr/ch32fun
