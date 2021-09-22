# qmk_tool
QMK Stuff

QMK related work
* console.py: QMK console client based on hidapi
* orgb_test.py: openrgb client for 1 fps solid color flashing effect
* sonix_swd_flash.py: a sonix SWD flasher based on openocd. !!!DONT TRY THIS UNLESS U KNOW WHAT U R DOING!!!
Usage:
[usage]
sonix_swd_flash.py [start address] [size] [file] --openocd [host]:[ip]

[example]
python sonix_swd_flash.py 0 0x200 e:\work\KEYBOARD\QMK_jumploader-gmmk_ansi_512.bin --openocd 127.0.0.1:4444


Notes
| Feature       | RAM           | Comment |  Idea |
| ------------- | ------------- |---------| ------|
| CONSOLE_ENABLE  | 216  | OUT/IN CAP = 1 | use RAW IN for console |
| NKRO_ENABLE  | 56  ||
