#!/bin/bash
#* ******************************************************************************
#*  If not stated otherwise in this file or this component's LICENSE
#*  file the following copyright and licenses apply:
#*
#*  Copyright 2025 RDK Management
#*
#*  Licensed under the Apache License, Version 2.0 (the License);
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*  http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an AS IS BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#*
#* ******************************************************************************

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# Define the directory where the repository will be cloned
declare -r TOP=`dirname $(realpath $0)`
declare -r UT_CORE_DIR="${TOP}/ut-core"
declare -r BUILD_DIR="${TOP}/build"
declare -r BUILD_INSTALL_PREFIX=${BUILD_DIR}/usr
declare -r MOCK_SERVICE_DIR="${TOP}/mockservice"
declare -r RDK_HAL_DIR=${TOP}/rdk-halif-aidl
declare -r INSTALL_PATH=${BUILD_DIR}/bin
declare -r DEFAULT_AIDL_MANIFEST="versions_released.yaml"

AIDL_VERSION_MANIFEST="${AIDL_VERSION_MANIFEST=${DEFAULT_AIDL_MANIFEST}}"
PREBUILT_AIDL_ROOT="${PREBUILT_AIDL_ROOT=${DEFAULT_PREBUILT_AIDL_ROOT}}"

# build_mock.sh owns mock-only flow and handles binder provisioning.
if [ "${1:-}" == "mock" ]; then
    shift
    exec "${TOP}/build_mock.sh" "$@"
fi

DEF_LINUX_BINDER="`realpath ${TOP}/../`/linux_binder_idl"
# AIDL_BIN: path of aidl compiler
AIDL_BIN=${AIDL_BIN:-${DEF_LINUX_BINDER}/out/aidl}
# LINUX_BINDER_OUT_DIR: output installed after building linux_binder_idl
# contains bin/servicemanager, lib/libbinder etc
LINUX_BINDER_OUT_DIR=${LINUX_BINDER_OUT_DIR:-${DEF_LINUX_BINDER}/local}

# Used in build audiodecoder_hal

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
RDK_HALIF_AIDL_VERSION="${RDK_HALIF_AIDL_VERSION="main"}"

ensure_rdk_hal_repo()
{
    if [ ! -d "$RDK_HAL_DIR" ]; then
        git clone -b $RDK_HALIF_AIDL_VERSION 'ssh://git@github.com/rdkcentral/rdk-halif-aidl.git' $RDK_HAL_DIR
    fi
}

resolve_manifest_file()
{
    if [[ "$AIDL_VERSION_MANIFEST" = /* ]]; then
        echo "$AIDL_VERSION_MANIFEST"
    else
        echo "${RDK_HAL_DIR}/${AIDL_VERSION_MANIFEST}"
    fi
}

manifest_component_version()
{
    local component="$1"
    local manifest_file
    local default_version

    manifest_file="$(resolve_manifest_file)"
    if [ ! -f "$manifest_file" ]; then
        echo "current"
        return 0
    fi

    default_version=$(awk '/^default:/{print $2; exit}' "$manifest_file")
    default_version=${default_version:-current}

    awk -v comp="$component" -v def="$default_version" '
        /^components:/      { inmap=1; next }
        inmap && /^[^[:space:]#]/ { inmap=0 }
        inmap && $1 == comp":" {
            print ($2 == "" ? def : $2)
            found=1
            exit
        }
        END { if (!found) print def }
    ' "$manifest_file"
}

stage_prebuilt_aidl_artifacts()
{
    local prebuilt_root
    local prebuilt_include
    local prebuilt_lib
    local audio_ver
    local avbuffer_ver
    local videodecoder_ver
    local audio_lib
    local avbuffer_lib
    local videodecoder_lib

    prebuilt_root="$(realpath "$PREBUILT_AIDL_ROOT" 2>/dev/null || true)"
    prebuilt_include="${prebuilt_root}/usr/include"
    prebuilt_lib="${prebuilt_root}/usr/lib"

    if [ ! -d "$prebuilt_include/com/rdk/hal/videodecoder" ] || [ ! -d "$prebuilt_lib" ]; then
        return 1
    fi

    ensure_rdk_hal_repo

    audio_ver="$(manifest_component_version audiodecoder)"
    avbuffer_ver="$(manifest_component_version avbuffer)"
    videodecoder_ver="$(manifest_component_version videodecoder)"

    audio_lib="libaudiodecoder-v${audio_ver}-cpp.so"
    avbuffer_lib="libavbuffer-v${avbuffer_ver}-cpp.so"
    videodecoder_lib="libvideodecoder-v${videodecoder_ver}-cpp.so"

    if [ ! -f "${prebuilt_lib}/${audio_lib}" ] || [ ! -f "${prebuilt_lib}/${avbuffer_lib}" ] || [ ! -f "${prebuilt_lib}/${videodecoder_lib}" ]; then
        return 1
    fi

    install -d ${BUILD_INSTALL_PREFIX}/include ${BUILD_INSTALL_PREFIX}/lib
    cp -apr ${prebuilt_include}/com ${BUILD_INSTALL_PREFIX}/include/

    install -d ${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/audiodecoder
    install -d ${BUILD_DIR}/avbuffer_gen/current/h/com/rdk/hal/avbuffer
    install -d ${BUILD_DIR}/videodecoder_gen/current/h/com/rdk/hal/videodecoder
    cp -f ${prebuilt_include}/com/rdk/hal/audiodecoder/*.h ${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/audiodecoder/
    cp -f ${prebuilt_include}/com/rdk/hal/avbuffer/*.h ${BUILD_DIR}/avbuffer_gen/current/h/com/rdk/hal/avbuffer/
    cp -f ${prebuilt_include}/com/rdk/hal/videodecoder/*.h ${BUILD_DIR}/videodecoder_gen/current/h/com/rdk/hal/videodecoder/

    cp -f ${prebuilt_lib}/${audio_lib} ${BUILD_INSTALL_PREFIX}/lib/
    cp -f ${prebuilt_lib}/${avbuffer_lib} ${BUILD_INSTALL_PREFIX}/lib/
    cp -f ${prebuilt_lib}/${videodecoder_lib} ${BUILD_INSTALL_PREFIX}/lib/

    ln -sf ${audio_lib} ${BUILD_INSTALL_PREFIX}/lib/libaudiodecoderaidl.so
    ln -sf ${avbuffer_lib} ${BUILD_INSTALL_PREFIX}/lib/libavbufferaidl.so
    ln -sf ${videodecoder_lib} ${BUILD_INSTALL_PREFIX}/lib/libvideodecoderaidl.so

    echo "Using prebuilt AIDL artifacts from ${prebuilt_root}"
    echo "Resolved versions: audiodecoder=${audio_ver}, avbuffer=${avbuffer_ver}, videodecoder=${videodecoder_ver}"

    return 0
}

prepare_aidl_binder_sdk()
{
    # Ensure Python dependencies for the AIDL host tools are available
    python3 -m pip install --quiet colorama 2>/dev/null || true

    # Stage the Binder SDK inside rdk-halif-aidl/out/target/ so that
    # rdk-halif-aidl/CMakeLists.txt can find it (it checks for .sdk_ready).
    if [ ! -f "${RDK_HAL_DIR}/out/target/.sdk_ready" ]; then
        echo "Preparing Binder SDK for AIDL generation..."
        (
            cd "${RDK_HAL_DIR}"
            ./build_binder.sh
        )
    fi
}

find_host_cmake()
{
    # file(REAL_PATH) in rdk-halif-aidl CMakeLists requires cmake >= 3.19.
    # When an OE/Yocto toolchain is active it may put an older cmake (e.g. 3.16)
    # at the front of PATH. Search known locations for a cmake >= 3.19.
    local candidates=("cmake" "/usr/bin/cmake" "/usr/local/bin/cmake")
    for c in "${candidates[@]}"; do
        if command -v "$c" &>/dev/null || [ -x "$c" ]; then
            local ver
            ver=$("$c" --version 2>/dev/null | awk 'NR==1{print $3}')
            local major minor
            IFS='.' read -r major minor _ <<< "$ver"
            if [ "${major:-0}" -gt 3 ] || { [ "${major:-0}" -eq 3 ] && [ "${minor:-0}" -ge 19 ]; }; then
                HOST_CMAKE="$c"
                echo -e "${GREEN}Using cmake: $HOST_CMAKE (version $ver)${RESET}"
                return 0
            fi
        fi
    done
    echo -e "${YELLOW}WARNING: cmake >= 3.19 not found, AIDL generation may fail${RESET}"
    HOST_CMAKE="cmake"
}

install_linux_binder()
{
    echo "Install linux-binder \`service manager, headers and libraries'"
    install -d ${BUILD_INSTALL_PREFIX}/bin ${BUILD_INSTALL_PREFIX}/lib ${BUILD_INSTALL_PREFIX}/include
    install -t ${BUILD_INSTALL_PREFIX}/bin ${LINUX_BINDER_OUT_DIR}/bin/*
    install -t ${BUILD_INSTALL_PREFIX}/lib ${LINUX_BINDER_OUT_DIR}/lib/*
    cp -apr ${LINUX_BINDER_OUT_DIR}/include/* ${BUILD_INSTALL_PREFIX}/include/
}

generate_audiodecoder_hal()
{
    declare -r HAL_BUILD_DIR=${BUILD_DIR}/audiodecoder_aidl
    declare -r AUDIODECODER_VERSION=current
    declare -r AIDL_GEN_DIR="${BUILD_DIR}/audiodecoder_gen/$AUDIODECODER_VERSION"


    ensure_rdk_hal_repo

    # Prepare cmake build directories and configuration for each module
    ${HOST_CMAKE} \
        -DAIDL_BIN=$AIDL_BIN \
        -DAIDL_GEN_DIR=$AIDL_GEN_DIR \
        -DAIDL_SRC_VERSION=$AUDIODECODER_VERSION \
        -DAIDL_TARGET=audiodecoder \
        -B ${HAL_BUILD_DIR}  \
        $RDK_HAL_DIR
}

generate_avbuffer_hal()
{
    declare -r HAL_BUILD_DIR=${BUILD_DIR}/avbuffer_hal
    declare -r VERSION=current
    declare -r AIDL_GEN_DIR="${BUILD_DIR}/avbuffer_gen/$VERSION"

    ensure_rdk_hal_repo

    for m in avbuffer; do
        ${HOST_CMAKE} \
        -DAIDL_BIN=$AIDL_BIN \
        -DAIDL_GEN_DIR=$AIDL_GEN_DIR \
        -DAIDL_SRC_VERSION=$VERSION \
        -DAIDL_TARGET=$m  \
        -B ${HAL_BUILD_DIR} \
        $RDK_HAL_DIR
    done
}

generate_videodecoder_hal()
{
    declare -r HAL_BUILD_DIR=${BUILD_DIR}/videodecoder_aidl
    declare -r VIDEODECODER_VERSION=current
    declare -r AIDL_GEN_DIR="${BUILD_DIR}/videodecoder_gen/$VIDEODECODER_VERSION"

    ensure_rdk_hal_repo

    ${HOST_CMAKE} \
        -DAIDL_BIN=$AIDL_BIN \
        -DAIDL_GEN_DIR=$AIDL_GEN_DIR \
        -DAIDL_SRC_VERSION=$VIDEODECODER_VERSION \
        -DAIDL_TARGET=videodecoder \
        -B ${HAL_BUILD_DIR} \
        $RDK_HAL_DIR

}

generate_avbufferhelper_skeleton()
{
    declare -r AVBUFFER_HELPER_DIR=${MOCK_SERVICE_DIR}/avbufferhelper
    mkdir -p $AVBUFFER_HELPER_DIR/build
    cd $AVBUFFER_HELPER_DIR/build

    cmake ..
    make

    cp libavbufferhelper.so ${BUILD_DIR}/usr/lib/
    cd -
    rm -rf $AVBUFFER_HELPER_DIR/build
}

build_aidl_libs()
{
    local halif_lib_dir="${RDK_HAL_DIR}/out/target/lib/halif"
    local halif_inc_dir="${RDK_HAL_DIR}/out/target/include/halif"
    local binder_lib_dir="${RDK_HAL_DIR}/out/target/lib"
    local binder_sdk_inc="${RDK_HAL_DIR}/out/build/include/binder_sdk"

    echo "Building AIDL interface libraries via cmake..."
    # audiodecoder cmake was configured to build all modules (INTERFACE_TARGET=all)
    ${HOST_CMAKE} --build "${BUILD_DIR}/audiodecoder_aidl" -j$(nproc)
    echo "AIDL cmake build completed."

    install -d "${BUILD_INSTALL_PREFIX}/lib" "${BUILD_INSTALL_PREFIX}/include"

    # Overwrite binder runtime libs with x86_64 rdkcentral ones (matching the AIDL build)
    install -t "${BUILD_INSTALL_PREFIX}/lib" "${binder_lib_dir}"/lib*.so

    # Overwrite binder headers to match the rdkcentral binder the AIDL libs were built against
    for d in binder utils android android-base cutils log; do
        [ -d "${binder_lib_dir}/../include/${d}" ] && \
            cp -apr "${binder_lib_dir}/../include/${d}" "${BUILD_INSTALL_PREFIX}/include/"
    done
    # binder_sdk build-time headers (android/binder_to_string.h etc.)
    [ -d "${binder_sdk_inc}" ] && cp -apr "${binder_sdk_inc}"/. "${BUILD_INSTALL_PREFIX}/include/"

    # Stage AIDL libs with the symlink names expected by mockservice Makefile
    local audio_lib avbuffer_lib videodecoder_lib
    audio_lib=$(ls "${halif_lib_dir}/libaudiodecoder-v"*"-cpp.so" 2>/dev/null | head -1)
    avbuffer_lib=$(ls "${halif_lib_dir}/libavbuffer-v"*"-cpp.so" 2>/dev/null | head -1)
    videodecoder_lib=$(ls "${halif_lib_dir}/libvideodecoder-v"*"-cpp.so" 2>/dev/null | head -1)

    if [ -z "$audio_lib" ] || [ -z "$avbuffer_lib" ] || [ -z "$videodecoder_lib" ]; then
        echo "ERROR: cmake did not produce expected AIDL libraries in ${halif_lib_dir}"
        ls "${halif_lib_dir}" 2>/dev/null || true
        exit 1
    fi

    cp -f "$audio_lib"        "${BUILD_INSTALL_PREFIX}/lib/$(basename "$audio_lib")"
    cp -f "$avbuffer_lib"     "${BUILD_INSTALL_PREFIX}/lib/$(basename "$avbuffer_lib")"
    cp -f "$videodecoder_lib" "${BUILD_INSTALL_PREFIX}/lib/$(basename "$videodecoder_lib")"

    ln -sf "$(basename "$audio_lib")"        "${BUILD_INSTALL_PREFIX}/lib/libaudiodecoderaidl.so"
    ln -sf "$(basename "$avbuffer_lib")"     "${BUILD_INSTALL_PREFIX}/lib/libavbufferaidl.so"
    ln -sf "$(basename "$videodecoder_lib")" "${BUILD_INSTALL_PREFIX}/lib/libvideodecoderaidl.so"

    # Copy ALL halif libs so transitive dependencies are resolvable
    install -t "${BUILD_INSTALL_PREFIX}/lib" "${halif_lib_dir}"/lib*.so
    # Refresh AIDL generated headers needed by mockservice compile
    for module in audiodecoder avbuffer videodecoder; do
        local gen_dir="${BUILD_DIR}/${module}_gen/current/h"
        if [ -d "${halif_inc_dir}/com/rdk/hal/${module}" ]; then
            install -d "${gen_dir}/com/rdk/hal/${module}"
            cp -f "${halif_inc_dir}/com/rdk/hal/${module}/"*.h "${gen_dir}/com/rdk/hal/${module}/"
        fi
    done
    # Common HAL headers (AVSource.h, State.h, etc.) go in the audiodecoder gen dir
    if [ -d "${halif_inc_dir}/com/rdk/hal" ]; then
        install -d "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal"
        cp -f "${halif_inc_dir}/com/rdk/hal/"*.h \
              "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/" 2>/dev/null || true
    fi

    echo "AIDL Build completed successfully."
}

build_mockservice()
{
    # Invoke Makefile to build the mock_service target
    make -j`nproc` \
        -C ${MOCK_SERVICE_DIR}
}

build_mock_only()
{
    install_linux_binder
    generate_avbufferhelper_skeleton

    if ! stage_prebuilt_aidl_artifacts; then
        echo "Prebuilt AIDL artifacts unavailable; falling back to local AIDL generation."
        prepare_aidl_binder_sdk
        find_host_cmake
        generate_avbuffer_hal
        generate_audiodecoder_hal
        generate_videodecoder_hal
        build_aidl_libs
    fi

    build_mockservice

    local required
    for required in \
        ${BUILD_DIR}/bin/MockVideoDecoderService \
        ${BUILD_DIR}/usr/lib/libvideodecoderaidl.so \
        ${BUILD_DIR}/usr/lib/libavbufferaidl.so \
        ${BUILD_DIR}/usr/lib/libavbufferhelper.so; do
        if [ ! -e "$required" ]; then
            echo -e "${BOLD}${RED}Mock-only build failed: missing artifact ${required}${RESET}"
            exit 1
        fi
    done

    mkdir -p ${BUILD_DIR}/mock_bundle
    cp -f ${BUILD_DIR}/bin/MockVideoDecoderService ${BUILD_DIR}/mock_bundle/
    cp -f ${BUILD_DIR}/usr/lib/libvideodecoderaidl.so ${BUILD_DIR}/mock_bundle/
    cp -f ${BUILD_DIR}/usr/lib/libavbufferaidl.so ${BUILD_DIR}/mock_bundle/
    cp -f ${BUILD_DIR}/usr/lib/libavbufferhelper.so ${BUILD_DIR}/mock_bundle/

    echo -e "${BOLD}${GREEN}Mock-only build completed. Artifacts: ${BUILD_DIR}/mock_bundle${RESET}"
}

build_ut_core()
{
    if [ ! -d ${UT_CORE_DIR} ]; then
        git clone https://github.com/rdkcentral/ut-core.git ${UT_CORE_DIR}
        if [ $? -ne 0 ]; then
            echo "Failed to clone the repository. Exiting."
            exit 1
        fi
    fi

    pushd ${UT_CORE_DIR}

    git fetch -q -t origin
    # get the latest version of ut-core
        if [ -z "$UT_CORE_VERSION" ]; then
            UT_CORE_VERSION=`git tag | sort -Vr | head -n1`
        fi

    git checkout $UT_CORE_VERSION

    ./build.sh TARGET=linux VARIANT=CPP
    if [ $? -ne 0 ]; then
        echo "Build script failed. Exiting."
        exit 1
    fi

    echo "Build completed successfully."
    popd
}

build_hal_log()
{
    declare -r HAL_LOG_DIR="${TOP}/mockservice/hal_log"
    pushd ${HAL_LOG_DIR}
    make

    cp -f ${HAL_LOG_DIR}/libhal_logger.so ${INSTALL_PATH}/
    cp ${HAL_LOG_DIR}/libhal_logger.so ${BUILD_DIR}/usr/lib/
    make clean
    popd
}

install_test()
{
    install -t ${INSTALL_PATH} ${BUILD_INSTALL_PREFIX}/bin/*
    install -t ${INSTALL_PATH} ${BUILD_INSTALL_PREFIX}/lib/*
    install -t ${INSTALL_PATH} run.sh
    cp -rf ${TOP}/profiles ${INSTALL_PATH}
    cp -rf ${TOP}/videodecoder_test_configuration ${INSTALL_PATH}
    #cp -rf ${TOP}/stream_configuration ${INSTALL_PATH}
    #mkdir -p ${INSTALL_PATH}/assets/streams/input
    #mkdir -p ${INSTALL_PATH}/assets/streams/reference
    rm -rf ${INSTALL_PATH}/libavbufferhelper.so
}

dist_clean_test()
{
    rm -rf $BUILD_DIR
    rm -rf src/*.o
    rm -rf ${MOCK_SERVICE_DIR}/src/*.o
    rm -rf $RDK_HAL_DIR
    rm -rf ${UT_CORE_DIR}
}

clean_test()
{
    rm -rf $BUILD_DIR
    rm -rf src/*.o
    rm -rf ${MOCK_SERVICE_DIR}/src/*.o
}

help()
{
    echo -e "${BOLD}${CYAN}Usage:${RESET}"

    echo -e "${BOLD}╔════════════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${BOLD}* ${GREEN}$0 all${RESET}            ${YELLOW}- Builds the test${RESET}                        ${BOLD}*${RESET}"
    echo -e "${BOLD}* ${GREEN}$0 mock${RESET}           ${YELLOW}- Builds mock service only (no UT/tests)${RESET} ${BOLD}*${RESET}"
    echo -e "${BOLD}* ${GREEN}$0 clean${RESET}          ${YELLOW}- Cleans the build folder${RESET}                ${BOLD}*${RESET}"
    echo -e "${BOLD}* ${GREEN}$0 dist_clean${RESET}     ${YELLOW}- Deletes all build and checkout folders${RESET} ${BOLD}*${RESET}"
    echo -e "${BOLD}* ${GREEN}$0 help/-h/--help${RESET} ${YELLOW}- Prints this message${RESET}                    ${BOLD}*${RESET}"
    echo -e "${BOLD}╚════════════════════════════════════════════════════════════════════╝${RESET}"
}

echo -e "${BOLD}${GREEN}VTS Version            : ${RESET}${YELLOW}$(git rev-parse --abbrev-ref HEAD)${RESET}"
echo -e "${BOLD}${GREEN}RDK_HALIF_AIDL_VERSION : ${RESET}${YELLOW}$RDK_HALIF_AIDL_VERSION${RESET}"
echo -e "${BOLD}${GREEN}AIDL_BIN               : ${RESET}${YELLOW}$AIDL_BIN${RESET}"
echo -e "${BOLD}${GREEN}LINUX_BINDER_OUT_DIR   : ${RESET}${YELLOW}$LINUX_BINDER_OUT_DIR${RESET}"

if [ "$1" == "clean" ]; then
    clean_test
elif [ "$1" == "dist_clean" ]; then
    dist_clean_test
elif [[ "$1" == "help" || "$1" == "--help" || "$1" == "-h" ]]; then
    help
elif [ "$1" == "mock" ]; then
    build_mock_only
elif [ "$1" == "all" ]; then
    install_linux_binder
    prepare_aidl_binder_sdk
    find_host_cmake
    generate_avbuffer_hal
    generate_audiodecoder_hal
    generate_videodecoder_hal
    generate_avbufferhelper_skeleton
    build_aidl_libs
    build_mockservice
    build_ut_core
    build_hal_log
    # Building test module using ut-core framework
    make -j`nproc` -C ${TOP}
    install_test
else
    echo -e "${BOLD}${RED}Invalid Arguments${RESET}"
    help
fi
