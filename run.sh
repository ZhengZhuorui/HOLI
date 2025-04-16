#!/bin/bash

RED_FONT="\e[0;31m"
WHITE_FONT="\e[0m"
GREEN_FONT="\e[0;32m"

function clean() {
    DIR = $1
    if [ -d $DIR ]; then
        echo "rm -r $1"
        rm -r $1
    fi
}

color_echo(){
    local font=$1
    shift
    echo -e "$font $@ $WHITE_FONT"
}

mode=$1
if [ $mode == "build_debug" ]; then
    clean build_debug
    mkdir -p build_debug
    cd build_debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j 5
fi

if [ $mode == "clean_debug" ]; then
    clean build_debug
fi

if [ $mode == "build_release" ]; then
    clean build
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j 5
fi

if [ $mode == "clean" ]; then
    clean build
fi

if [ $mode == "run_debug" ]; then
    DIR=build_debug
    if [ ! -d $DIR ]; then
        echo "build_debug directory does not exist"
        exit
    fi
    cd build_debug
    make -j 5
    if [[ $? -ne 0 ]]; then
        color_echo $RED_FONT "compile error"
        exit
    fi
    shift
    arg=$@
    echo $1
    if [[ -x "$1" ]]; then
        echo "run: $arg"
        $arg
        if [ $? -eq 0 ]; then
            #echo "$GREEN_FONT success $WHITE_FONT"
            color_echo $GREEN_FONT "run success"
        else
            color_echo $RED_FONT "run error"
        fi
      else
        echo "bineary no exists: $arg"
    fi
fi

if [ $mode == "run" ]; then
    DIR=build
    if [ ! -d $DIR ]; then
        echo "build directory does not exist"
        exit
    fi
    cd build
    make -j 5
    if [[ $? -ne 0 ]]; then
        color_echo $RED_FONT "compile error"
        exit
    fi
    shift
    arg=$@
    echo $1
    if [[ -x "$1" ]]; then
        echo "run: $arg"
        $arg
        if [ $? -eq 0 ]; then
            #echo "$GREEN_FONT success $WHITE_FONT"
            color_echo $GREEN_FONT "run success"
        else
            color_echo $RED_FONT "run error"
        fi
      else
        echo "bineary no exists: $arg"
    fi
fi
