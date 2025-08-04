# Building linux kernel (Ubuntu)
<small>For building kernel need ~30G free space on the drive. Can be used any MicroSD card up to 256G (A2 U3 V30 C10) formatted into ext3/ext4 filesystem (for support symlinks).<br>
>__Step 1. Downloading sources and installing additional packages__<br>
>```bash
>sudo apt-get install linux-source-6.1 git bison libncurses5-dev libssl-dev pahole flex dpkg-dev libelf-dev debhelper-compat libdw-dev
>```
>__Step 2. Preparing external storage for build__<br>
>2.1 Do enter into mounted folder of your external storage (where *** - label of your external storage)
>```bash
>cd /media/$(whoami)/***/
>```
>2.2 Do clone this repository
>```bash
>git clone https://github.com/spark1991z/device_irbis_tw48
>```
>2.3 Do create folder for binaries files
>```bash
>mkdir -p build
>```
>2.4 Do copy downloaded archive with sources
>```bash
>cp /usr/src/linux-source-6.1.tar.xz .
>```
>2.5 Do extract archive with sources
>```bash
>tar xf linux-sources-6.1.tar.xz
>```
>2.6 Do copy current config file from boot partition into folder of kernel sources into folder with configs for x86 arch.
>```bash
>cp /boot/config-$(uname -r) linux-sources-6.1/arch/x86/configs/tw48_defconfig
>```
>2.7 Do patch __touchscreen_dmi.c__ file for your DMI Bios version and replace his in folder with kernel sources by same path
>```bash
>bios_ver=$(sudo dmidecode | head -n10 | grep Version | cut -d ' ' -f 2) && echo $bios_ver
>```
>```bash
>cat device_irbis_tw48/linux-kernel/drivers/platform/x86/touchscreen_dmi.c | \
>sed -e "s/_VERSION_/"${bios_ver}"/" > linux-source-6.1/drivers/platform/x86/touchscreen_dmi.c
>```
>__Step 3. Building kernel__<br>
>3.1 Do enter into folder with kernel sources 
>```bash
>cd linux-source-6.1
>```
>3.2 Do create build configuration from copied file
>```bash
>make -j$(nproc) tw48_defconfig O=../build
>```
>3.3 Do launch build new kernel and installation files (*.deb)<br>
>__Attention:__ On this tablet building will very long (~7-12h), better use 2 cores and turned on charging for building time.
>```bash
>make -j2 bindeb-pkg O=../build
>```
>__Step 4. Installing new kernel__<br>
>4.1 Do leave from folder with kernel sources
>```bash
>cd ..
>```
>4.2 Do install generated *.deb packages
>```bash
>sudo dpkg -i *.deb
>```
>4.3 Do reboot tablet
>```bash
>sudo reboot
>```
</small>
