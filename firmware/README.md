# Setup environment

## Native

```sh
$ . ~/workspace/tools/esp-idf/export.sh
```

## Docker

```sh
$ docker run -it --rm -v $(pwd):/tmp/tst --device=/dev/ttyUSB0 --device=/dev/ttyUSB1 espressif/idf:v5.5.3
root@3d819e2767ac:/# cd /tmp/tst
```

# Configure app

```sh
$ idf.py set-target esp32s3
$ idf.py menuconfig
```

# Generate new SSL certificates

```sh
$ openssl req -x509 -nodes -newkey rsa:2048 -keyout webserver/certs/prvtkey.pem -out webserver/certs/servercert.pem -config webserver/certs/req-ssl.cnf
```

# Build app

```sh
$ cd frontend
$ npm run build
$ cd ../firmware
$ python www_bin/generate/generate.py ../frontend/dist/
$ idf.py build
```

# Start gdb server

```sh
$ echo $OPENOCD_SCRIPTS
$ openocd -f board/esp32s3-ftdi.cfg
```

# Program chip via JTAG

```sh
$ openocd -f board/esp32s3-ftdi.cfg -c "program_esp mcm-lin.bin 0x10000 verify exit"
```

# Run gdb

```sh
$ xtensa-esp32s3-elf-gdb -x gdbinit build/mcm-lin.elf
```

# Example gdbinit

```
target remote :3333
set remote hardware-watchpoint-limit 2
mon reset halt
maintenance flush register-cache
thb app_main
c
```

# Program chip via bootloader

```sh
$ idf.py -p /dev/ttyUSB1 flash
$ idf.py -p /dev/ttyUSB1 flash monitor
```

# Uncrustify code

Check:

```sh
$ uncrustify -c ../verification/uncrustify/uncrustify.cfg --check -l C device_info/*.c device_info/include/*.h device_status/*.c device_status/include/*.h main/*.c power_ctrl/*.c power_ctrl/include/*.h usb_device/*.c usb_device/include/*.h webserver/*.c webserver/include/*.h networking/*.c networking/include/*.h
```

Fix:

```sh
$ uncrustify -c ../verification/uncrustify/uncrustify.cfg --replace --no-backup -l C device_info/*.c device_info/include/*.h device_status/*.c device_status/include/*.h main/*.c power_ctrl/*.c power_ctrl/include/*.h usb_device/*.c usb_device/include/*.h webserver/*.c webserver/include/*.h networking/*.c networking/include/*.h
```

# Use Docker

```sh
$ docker run -it --rm --device=/dev/ttyUSB0 --device=/dev/ttyUSB1 -v $(pwd):/tmp/tst espressif/idf:v5.5.1
$ cd /tmp/tst/firmware
$ idf.py set-target esp32s3
$ idf.py build
$ idf.py -p /dev/ttyUSB1 flash
$ idf.py -p /dev/ttyUSB1 flash monitor
```
