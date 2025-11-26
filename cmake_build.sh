#!/bin/bash

if [[ -n $1 ]]; then
    APP=$1
    if [[ -n $2 ]]; then
        USER_CMD=$2
    else
        USER_CMD="-j"
    fi
else
    APP=app
    USER_CMD="-j"
fi

if [[ ! -e $PWD/MSDK/${APP} ]]; then
    echo "build app error: $PWD/MSDK/${APP} is not exsit !!"
    exit 1
fi

if [[ ! -e ./tools/gd32vw55x_toolchain_linux ]]; then
    if [[ -e ./tools/gd32vw55x_toolchain_linux.tar.gz00 ]]; then
        echo "Unzip gd32vw55x toolchain ......."
#        tar xvzf ./tools/gd32vw55x_toolchain_linux.tar.gz -C ./tools
        cat ./tools/gd32vw55x_toolchain_linux.tar.gz* | tar xvz -C ./tools/
    else
        echo "toolchain error: $PWD/tools/gd32vw55x_toolchain_linux.tar.gz00 is not exsit!"
        exit 1
    fi
fi

if [[ ! -e ./tools/xpack-openocd-0.11.0-3_linux ]]; then
    if [[ -e ./tools/xpack-openocd-0.11.0-3_linux.tar.gz ]]; then
        echo "Unzip gd32vw55x opencod ......."
        tar xvzf ./tools/xpack-openocd-0.11.0-3_linux.tar.gz -C ./tools
    else
        echo "openocd error: $PWD/tools/xpack-openocd-0.11.0-3_linux.tar.gz is not exsit!"
        exit 1
    fi
fi

export PATH=$PATH:$PWD/tools/gd32vw55x_toolchain_linux/bin

if type python3 > /dev/null 2>&1; then
    PYTHON=python3
else
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt install -y python3"
    echo "#####################################################"
    exit 1;
fi

if ! type make >/dev/null 2>&1; then
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt install -y build-essential"
    echo "#####################################################"
    exit 1;
fi

if ! type srec_cat >/dev/null 2>&1; then
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt install srecord"
    echo "#####################################################"
    exit 1;
fi

if ! type cmake >/dev/null 2>&1; then
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt install cmake"
    echo "#####################################################"
    exit 1;
fi

if ! ldconfig -p | grep -q libncursesw.so.5; then
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt-get install libncursesw5"
    echo "#####################################################"
    exit 1;
fi

if ! ldconfig -p | grep -q libncurses.so.5; then
    echo "#####################################################"
    echo "Error:"
    echo "Please run the following command to install the dependent libraries:"
    echo "sudo apt-get install libncurses5"
    echo "#####################################################"
    exit 1;
fi

if [[ ! -e cmake_build ]]; then
    mkdir cmake_build
fi

cd cmake_build
if [[ $USER_CMD = "clean" ]];then
    rm ./* -rf
else
    cmake -G "Unix Makefiles" -DAPP=${APP} -DCONFIG_BLE_FEATURE=MAX -DCONFIG_MBEDTLS_VERSION="3.6.2" -DCMAKE_TOOLCHAIN_FILE:PATH=./scripts/cmake/toolchain.cmake  ..

    make ${USER_CMD}
fi
cd ../

