# How to compile & run

## 系统环境

安装必要工具：

```sh
sudo apt update && sudo apt upgrade
sudo apt install -y git gitk cmake cmake-curses-gui libssl-dev libjson-c-dev libxml2-dev
sudo apt install -y ./MVS-2.1.1_aarch64_20211224.deb
```

## 编译 libopencv_world.so

```sh
git clone https://github.com/opencv/opencv
cd opencv
make -p build && cd build
ccmake ..
'c' -> select opencv_world ON
make -j
# make package
make install  # install to `/usr/local/lib`
# append `/usr/local/lib` to `/etc/ld.so.conf` and than `sudo ldconfig`
```



##　编译步骤

```sh
mkdir build && cd build
cmake ..
# change some options. using web or not
ccmake ..
# 'c' -> ('e' ->) 'g'
make -j
make install
# make as a debian package
make package
```

## run

```sh
cd build/src
# show help
./d3cam -h
# run 
sudo ./d3cam # add 'sudo' for use '/dev/ttyTHS0'
# log in /var/log/syslog
# grep d3cam /var/log/syslog
```
