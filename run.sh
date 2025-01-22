
#!/bin/bash
mode=$1
if [ $mode == "debug" ]; then
    mkdir -p build_debug
    cd build_debug
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make -j 5
fi

if [ $mode == "clean_debug" ]; then
    echo "clean_debug"
    rm -r build_debug
fi

if [ $mode == "release" ]; then
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make -j 5
fi

if [ $mode == "clean" ]; then
    rm -r build
fi

if [ $mode == "run_debug" ]; then
    DIR=build_debug
    if [ ! -d $DIR ]; then
        echo "build_debug directory does not exist"
        exit
    fi
    cd build_debug
    make -j 5
    shift
    arg=$@
    echo $1
    if [[ -x "$1" ]]; then
        echo "run: $arg"
        $arg
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
    shift
    arg=$@
    echo $1
    if [[ -x "$1" ]]; then
        echo "run: $arg"
        $arg
      else
        echo "bineary no exists: $arg"
    fi
fi
