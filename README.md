# How to compile

## 系统环境

安装必要工具：

```sh
sudo apt update && sudo apt upgrade
sudo apt install git gitk cmake cmake-curses-gui libssl-dev
sudo apt install ./MVS-2.1.1_aarch64_20211224.deb
```

##　编译步骤

```sh
mkdir build && cd build
cmake ..
# change some options
ccmake ..
# 'c' -> ('e' ->) 'g'
make -j
make install
# make as a debian package
make package
```

