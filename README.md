# AB5396_minimal
Minimal SDK for Bluetrum AB5396

## how
This repo will be a minimal SDK for the Bluetrum BLE MCU AB5396 found in cheap USB BLE dongles[^1][^2].\
There are currently two ways to run code on the chip, in the `inram/` folder the bootloader is sent
a stub to be ran from `0x12000`, and for the second method the XIP cache is reverse engineered and
described in [XIP.md](docs/XIP.md). For this method `boot.c` implements the stage1 bootloader that is
packed with `mkheader.py` from [^4] which sets up the XIP cache system and jumps to plain
unobfuscated code in flash, which can be put there using the `download.py` script from that same project.
Subsequently `main.c` contains the code that is run from XIP, at the address specified in `XIP_FLASH_OFFSET`.

A programmer for these chips is simply an UART with RX and TX connected by a 200 - 470 Ohm resistor, and
the RX side connected to the USB+ pin (PB3).

The Makefile uses `minichlink`[^5] to enter the bootloader by switching on 5v when the UART is waiting for
the sync byte, if you don't have a linkE programmer you can easily remove that and switch 5v to the chip manually.

Additional info can be found in [^6].

## compile and flash
**WARNING: flashing a custom stage1 bootloader can brick the chip! If the stage1 loader code is not returned
from the bootloader will not respond to `SYNC_TOKEN` anymore!**

Easy and safe quick tests can be done with the inram stub in `inram/main.c`, and can simply be done from
that `inram/` folder:
```bash
make clean all upload
```
Take into account this `Makefile` uses `minichlink` from [^5] and a WCH linkE programmer to toggle power.

To compile the custom stage1 bootloader and main program that will live in flash, just use `make` or `make clean all`
in the root folder of the project. Currently the firmware uses microshell [^7], so clone that repo too and point
the variable in the `Makefile` to this.

After running `make` the resulting stage1 loader header `LUCK.bin` can be written to flash with `bluetrum-tools/download.py`:
```bash
python ../bluetrum-tools/download.py --port /dev/ttyACM0 --baud 115200 write 0x0 LUCK.bin
```

and the main firmware `main.bin` with the same tool should be flashed to the `MAIN_FLASH_OFFSET` address (for example 0x1000):
```bash
python ../bluetrum-tools/download.py --port /dev/ttyACM0 --baud 115200 write 0x1000 main.bin
```

## progress
- [x] Run from RAM using bootrom callback
- [x] Reverse encryption scheme for firmware in flash
- [x] Run from XIP flash
- [x] microshell (for REPL)
- [x] USB stack
- [ ] RF stack

### resources
[^1]: https://www.aliexpress.com/item/1005009109220784.html
[^2]: https://www.aliexpress.com/item/1005009109412274.html
[^3]: https://github.com/atc1441/Bluetrum_AB5682_Hacking
[^4]: https://github.com/kagaimiq/bluetrum-tools
[^5]: https://github.com/cnlohr/ch32fun
[^6]: https://github.com/ZhiqingLi/Sdk_Refresh
[^7]: https://github.com/marcinbor85/microshell
