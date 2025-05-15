# Linux Kernel Module: snd-soc-es9038q2m

## Description

This is a project to make ESS Technologies es9038q2m work properly under Linux, with a enphasis in supporting as much of the codec as possible instead to be just a skeleton module.

As the test bed, a prototype of MDH314 DAC is begin used with a Raspberry Pi 4, thus all the installation instructions will be geared towards it.

## Requirements

- Linux kernel source code
- GCC compiler
- Knowledge of how to cross compile raspberry pi kernel or use the rasp itself to compile natively
- Be devoid of murderous intent

## Installation

1. Clone the repository
  ```bash
  git clone https://github.com/fbugno/snd-soc-es9038q2m.git
  ```

2. Clone the raspberry pi firmware repository
  ```bash
  git clone https://github.com/raspberrypi/linux
  ```

3. Copy the files to their folders
  ```bash
  cd snd-soc-es9038q2m
  cp bcm2711_defconfig ../linux/arch/arm64/configs/
  cp Kconfig Makefile es9038q2m.c es9038q2m.h ../linux/sound/soc/codecs/
  ```

4. Compile the kernel
  ```bash
  cd ../linux
  make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- bcm2711_defconfig
  make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- -j$(nproc)
  make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu- INSTALL_MOD_PATH=./dump modules_install
  ```
5. Install your new kernel and modules (adapt the commands to your use case, my rasp has root enabled and accessible from ssh, given that its just a development machine that I don't care much... your may not, and will need to move the files to intermediate folder first and from inside the raspberry mode the file to its final place)
  ```bash
  scp arch/arm64/boot/Image root@raspberrypi.local:/boot/kernel8.img
  scp -r arch/arm64/boot/dts/overlays/ root@raspberrypi.local:/boot/firmware/overlays/
  rm dump/lib/modules/6.12.27-v8+/build
  scp -r dump/lib/modules/6.12.27-v8+/ root@raspberrypi.local:/lib/modules/
  scp ../snd-soc-es9038q2m/mahaudio-mhd314.dts
  ```

6. Modify the raspberry pi by editing /boot/firmware/config.txt by
   - Comment out dtparam=audio=on
   - Add dtparam=i2s=on it not already there

7. Reboot to enable the new kernel

8. Install the device tree compiler at the raspberry pi and kill pipewire/pulseaudio forever, reboot after this
   ```bash
   apt install device-tree-compiler
   apt remove pulseaudio
   ```

9. Compile the device tree
  ```bash
  dtc -I dts -O dtb -o mahaudio-mhd314.dtbo mahaudio-mhd314.dts
  ```

From now on, you setup the enviroment to develop the module.

To enable it, do:
```bash
mkdir /sys/kernel/config/device-tree/overlays/mahaudio-mhd314
cat /boot/firmware/overlays/mahaudio-mhd314.dtbo >  /sys/kernel/config/device-tree/overlays/mahaudio-mhd314/dtbo
```

The module should be loaded by the device tree once done that, as:
```bash
root@raspberry:/home/fbugno# lsmod
Module                  Size  Used by
snd_soc_es9038q2m      12288  1
regmap_i2c             12288  1 snd_soc_es9038q2m
snd_soc_simple_card    16384  1
snd_soc_simple_card_utils    32768  1 snd_soc_simple_card
```

And should be available to ALSA to use standart tools like speaker-test:
```bash
speaker-test -Dhw:CARD=mhd314 -c2 -FS32_LE -r48000 -t sine -f 8000
```
To unload the driver, do
```bash
rmdir /sys/kernel/config/device-tree/overlays/mahaudio-mhd314
rmmod snd_soc_es9038q2m
```

## Documentation

The only real document needed is the ES9038Q2M datasheet.

[ESS Tech datasheet for ES9038Q2M](https://www.esstech.com/wp-content/uploads/2022/09/ES9038Q2M-Datasheet-v1.4.pdf)

# License

This project is licensed under the GPLv3.
