#!/bin/bash

# Define the directory where the repository will be cloned
declare -r UT_CORE_VER="4.1.0"
declare -r TOP=`dirname $(realpath $0)`
declare -r UT_DIR="${TOP}/ut"
declare -r BUILD_DIR="${UT_DIR}/build"
declare -r BUILD_INSTALL_PREFIX=${BUILD_DIR}/usr

DEF_LINUX_BINDER="`realpath ${TOP}/../../`/linux_binder_idl"
# AIDL_BIN: path of aidl compiler
AIDL_BIN=${AIDL_BIN:-${DEF_LINUX_BINDER}/out/aidl}
# LINUX_BINDER_OUT_DIR: output installed after building linux_binder_idl
# contains bin/servicemanager, lib/libbinder etc
LINUX_BINDER_OUT_DIR=${LINUX_BINDER_OUT_DIR:-${DEF_LINUX_BINDER}/local}

# Used in build videodecoder_hal
export HAL_DIR=${UT_DIR}

mkdir -p $UT_DIR
mkdir -p $BUILD_DIR

if [ ! -x $AIDL_BIN ]; then
	echo "AIDL_BIN: path does not exists, consdier building the linux_binder_idl in $DEF_LINUX_BINDER"
	exit 1
fi
if [ ! -d $LINUX_BINDER_OUT_DIR ]; then
	echo "LINUX_BINDER_OUT_DIR: path does not exists, consider building the linux_binder_idl in $DEF_LINUX_BINDER"
	exit 1
fi

# Turning them into absolute path because we may calling change current working directory
# whiling running the script
declare -r AIDL_BIN=`realpath ${AIDL_BIN}`
declare -r LINUX_BINDER_OUT_DIR=`realpath ${LINUX_BINDER_OUT_DIR}`

install_linux_binder()
{
	echo "Install linux-binder \`service manager, headers and libraries'"
	install -d ${BUILD_INSTALL_PREFIX}/bin ${BUILD_INSTALL_PREFIX}/lib ${BUILD_INSTALL_PREFIX}/include
	install -t ${BUILD_INSTALL_PREFIX}/bin ${LINUX_BINDER_OUT_DIR}/bin/*
	install -t ${BUILD_INSTALL_PREFIX}/lib ${LINUX_BINDER_OUT_DIR}/lib/*
	cp -apr ${LINUX_BINDER_OUT_DIR}/include/* ${BUILD_INSTALL_PREFIX}/include/
}

generate_videodecoder_hal() {
    declare -r HAL_BUILD_DIR=${BUILD_DIR}/videodecoder_aidl
    declare -r VIDEODECODER_VERSION=current
    declare -r AIDL_GEN_DIR="${BUILD_DIR}/videodecoder_gen/$VIDEODECODER_VERSION"
    declare -r RDK_HAL_DIR=${UT_DIR}/rdk-halif-aidl

    # Clone the repository if not already present
    if [ ! -d "$RDK_HAL_DIR" ]; then
        git clone -b develop 'ssh://git@github.com/rdkcentral/rdk-halif-aidl.git' $RDK_HAL_DIR
    fi

    cmake \
        -DAIDL_BIN=$AIDL_BIN \
        -DAIDL_GEN_DIR=$AIDL_GEN_DIR \
        -DAIDL_SRC_VERSION=$VIDEODECODER_VERSION \
        -DAIDL_TARGET=videodecoder \
        -B ${HAL_BUILD_DIR} \
        $RDK_HAL_DIR

}

build_videodecoder_service()
{
    # Invoke Makefile to build the mock_service target
    make -j`nproc` \
		-C $UT_DIR \
		-f ${UT_DIR}/Makefile.mock_service
}

build_ut_core()
{
	if [ ! -d ${UT_DIR}/ut-core ]; then
		git clone https://github.com/rdkcentral/ut-core.git ${UT_DIR}/ut-core
		if [ $? -ne 0 ]; then
			echo "Failed to clone the repository. Exiting."
			exit 1
		fi
	fi

	pushd ${UT_DIR}/ut-core

	git fetch -q origin $UT_CORE_VER
	git checkout $UT_CORE_VER

	./build.sh TARGET=linux VARIANT=CPP
	if [ $? -ne 0 ]; then
		echo "Build script failed. Exiting."
		exit 1
	fi

	echo "Build completed successfully."
	popd
}

build_yaml_cpp()
{
	declare -r YAML_CPP_BUILD_DIR=${BUILD_DIR}/yaml-cpp

	if [ ! -d "yaml-cpp" ]; then
		git clone https://github.com/jbeder/yaml-cpp.git
	fi

	cmake \
		-DYAML_BUILD_SHARED_LIBS=ON \
		-DCMAKE_INSTALL_PREFIX=$BUILD_INSTALL_PREFIX \
		-B $YAML_CPP_BUILD_DIR \
		yaml-cpp
	make -j`nproc` -C ${YAML_CPP_BUILD_DIR} install
}

install_test()
{
	declare -r INSTALL_PATH=${BUILD_DIR}/bin

	install -t ${INSTALL_PATH} ${BUILD_INSTALL_PREFIX}/bin/*
	install -t ${INSTALL_PATH} ${BUILD_INSTALL_PREFIX}/lib/*
	install -t ${INSTALL_PATH} run.sh
	cp -r ${TOP}/profiles/ ${INSTALL_PATH}
}

install_linux_binder
generate_videodecoder_hal
build_videodecoder_service
build_ut_core
# Building test module using ut-core framework
make -j`nproc` -C ${UT_DIR}
install_test
