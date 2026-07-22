#!/bin/bash
#* ******************************************************************************
#*  If not stated otherwise in this file or this component's LICENSE
#*  file the following copyright and licenses apply:
#*
#*  Copyright 2026 RDK Management
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

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

declare -r TOP="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
declare -r BUILD_DIR="${TOP}/build"
declare -r BUILD_INSTALL_PREFIX="${BUILD_DIR}/usr"
declare -r MOCK_SERVICE_DIR="${TOP}/mockservice"
declare -r RDK_HAL_DIR="${TOP}/rdk-halif-aidl"
declare -r DEFAULT_AIDL_MANIFEST="versions_released.yaml"
declare -r DEF_LINUX_BINDER="${TOP}/linux_binder_idl"

RDK_HALIF_AIDL_VERSION="${RDK_HALIF_AIDL_VERSION:-main}"
AIDL_VERSION_MANIFEST="${AIDL_VERSION_MANIFEST:-${DEFAULT_AIDL_MANIFEST}}"
PREBUILT_AIDL_ROOT="${PREBUILT_AIDL_ROOT:-}"
LINUX_BINDER_DIR="${LINUX_BINDER_DIR:-${DEF_LINUX_BINDER}}"
BINDER_VERSION="${BINDER_VERSION:-}"
TARGET_ARCH="${TARGET_ARCH:-linux}"
HOST_CMAKE="cmake"
LINUX_BINDER_OUT_DIR=""
AIDL_BIN=""

mkdir -p "${BUILD_DIR}"

ensure_rdk_hal_repo()
{
	if [ ! -d "${RDK_HAL_DIR}" ]; then
		git clone https://github.com/rdkcentral/rdk-halif-aidl.git "${RDK_HAL_DIR}"
	fi

	if [ ! -d "${RDK_HAL_DIR}/.git" ]; then
		echo -e "${BOLD}${RED}ERROR: ${RDK_HAL_DIR} exists but is not a git repository${RESET}"
		exit 1
	fi

	pushd "${RDK_HAL_DIR}" >/dev/null

	local target_ref="${RDK_HALIF_AIDL_VERSION}"
	local current_ref
	current_ref="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo detached)"

	if [ "${current_ref}" != "${target_ref}" ]; then
		if [ -n "$(git status --porcelain)" ]; then
			echo -e "${YELLOW}Cleaning uncommitted changes in ${RDK_HAL_DIR}...${RESET}"
			git checkout . || {
				git clean -fd || true
			}
		fi

		git fetch -q --tags origin
		if ! git checkout "${target_ref}"; then
			echo -e "${BOLD}${RED}ERROR: unable to checkout RDK_HALIF_AIDL_VERSION=${target_ref}${RESET}"
			popd >/dev/null
			exit 1
		fi
	fi

	popd >/dev/null
}

resolve_binder_version()
{
	if [ -n "${BINDER_VERSION}" ]; then
		echo "${BINDER_VERSION}"
		return 0
	fi

	local version_file="${RDK_HAL_DIR}/binder_sdk.version"
	if [ -f "${version_file}" ]; then
		local pinned
		pinned="$(grep -vE '^[[:space:]]*(#|$)' "${version_file}" | head -1 | tr -d '[:space:]')"
		if [ -n "${pinned}" ]; then
			echo "${pinned}"
			return 0
		fi
	fi

	echo "2.3.0"
}

ensure_linux_binder_repo()
{
	local target_ref="$1"

	if [ ! -d "${LINUX_BINDER_DIR}" ]; then
		git clone https://github.com/rdkcentral/linux_binder_idl.git "${LINUX_BINDER_DIR}"
	fi

	if [ ! -d "${LINUX_BINDER_DIR}/.git" ]; then
		echo -e "${BOLD}${RED}ERROR: ${LINUX_BINDER_DIR} exists but is not a git repository${RESET}"
		exit 1
	fi

	pushd "${LINUX_BINDER_DIR}" >/dev/null
	if [ -n "$(git status --porcelain)" ]; then
		echo -e "${YELLOW}Cleaning uncommitted changes in ${LINUX_BINDER_DIR}...${RESET}"
		git checkout . || {
			git clean -fd || true
		}
	fi

	git fetch -q --tags origin
	if ! git checkout "${target_ref}"; then
		echo -e "${BOLD}${RED}ERROR: unable to checkout linux_binder_idl ref ${target_ref}${RESET}"
		popd >/dev/null
		exit 1
	fi
	popd >/dev/null
}

build_linux_binder_target()
{
	if [ ! -x "${LINUX_BINDER_DIR}/build-linux-binder-aidl.sh" ]; then
		echo -e "${BOLD}${RED}ERROR: missing build script ${LINUX_BINDER_DIR}/build-linux-binder-aidl.sh${RESET}"
		exit 1
	fi

	echo -e "${BOLD}${CYAN}Building linux_binder_idl target artifacts (TARGET_ARCH=${TARGET_ARCH})...${RESET}"
	echo -e "${BOLD}${CYAN}Build directory: ${LINUX_BINDER_DIR}${RESET}"
	pushd "${LINUX_BINDER_DIR}" >/dev/null
	if [ "${TARGET_ARCH}" = "arm" ]; then
		if [ -z "${CC:-}" ] || [ -z "${CXX:-}" ]; then
			echo -e "${BOLD}${RED}ERROR: Target=arm specified but CC/CXX cross-compiler not set in environment${RESET}"
			echo "Please source the ARM toolchain setup script first, e.g.:"
			echo "  source /opt/toolchains/rdk-glibc-x86_64-arm-toolchain/environment-setup-armv7vet2hf-neon-oe-linux-gnueabi"
			popd >/dev/null
			exit 1
		fi
		echo -e "${GREEN}Using ARM cross-compiler: CC=${CC} CXX=${CXX}${RESET}"
	fi
	./build-linux-binder-aidl.sh
	popd >/dev/null

	LINUX_BINDER_OUT_DIR="${LINUX_BINDER_DIR}/out/target"
	if [ ! -d "${LINUX_BINDER_OUT_DIR}/lib" ] || [ ! -d "${LINUX_BINDER_OUT_DIR}/include" ]; then
		echo -e "${BOLD}${RED}ERROR: linux_binder_idl output not found at ${LINUX_BINDER_OUT_DIR}${RESET}"
		exit 1
	fi
	echo -e "${CYAN}linux_binder_idl output directory structure:${RESET}"
	find "${LINUX_BINDER_DIR}/out" -maxdepth 3 -type f -name "*aidl*" 2>/dev/null | head -20 || true
}

resolve_aidl_bin()
{
	local candidate
	for candidate in \
		"${LINUX_BINDER_DIR}/out/host/bin/aidl" \
		"${LINUX_BINDER_DIR}/out/target/bin/aidl" \
		"${LINUX_BINDER_DIR}/out/target/bin/aidl-cpp" \
		"${LINUX_BINDER_DIR}/out/aidl" \
		"${LINUX_BINDER_DIR}/build-target/aidl" \
		"${LINUX_BINDER_DIR}/build-host/aidl" \
		"${RDK_HAL_DIR}/out/target/bin/aidl"; do
		if [ -x "${candidate}" ]; then
			echo -e "${CYAN}Found AIDL compiler at: ${candidate}${RESET}"
			AIDL_BIN="${candidate}"
			return 0
		fi
	done

	echo -e "${YELLOW}AIDL binary search paths:${RESET}"
	for candidate in \
		"${LINUX_BINDER_DIR}/out/host/bin/aidl" \
		"${LINUX_BINDER_DIR}/out/target/bin/aidl" \
		"${LINUX_BINDER_DIR}/out/aidl" \
		"${LINUX_BINDER_DIR}/build-target/aidl" \
		"${LINUX_BINDER_DIR}/build-host/aidl" \
		"${RDK_HAL_DIR}/out/target/bin/aidl"; do
		echo "  - ${candidate}: $([ -x "${candidate}" ] && echo "✓ found" || echo "✗ missing")"
	done

	AIDL_BIN=""
	return 1
}

resolve_manifest_file()
{
	if [[ "${AIDL_VERSION_MANIFEST}" = /* ]]; then
		echo "${AIDL_VERSION_MANIFEST}"
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
	if [ ! -f "${manifest_file}" ]; then
		echo "current"
		return 0
	fi

	default_version="$(awk '/^default:/{print $2; exit}' "${manifest_file}")"
	default_version="${default_version:-current}"

	awk -v comp="${component}" -v def="${default_version}" '
		/^components:/      { inmap=1; next }
		inmap && /^[^[:space:]#]/ { inmap=0 }
		inmap && $1 == comp":" {
			print ($2 == "" ? def : $2)
			found=1
			exit
		}
		END { if (!found) print def }
	' "${manifest_file}"
}

install_linux_binder()
{
	echo -e "${BOLD}${CYAN}Installing linux binder runtime and headers...${RESET}"
	install -d "${BUILD_INSTALL_PREFIX}/bin" "${BUILD_INSTALL_PREFIX}/lib" "${BUILD_INSTALL_PREFIX}/include"
	if [ -d "${LINUX_BINDER_OUT_DIR}/bin" ]; then
		install -t "${BUILD_INSTALL_PREFIX}/bin" "${LINUX_BINDER_OUT_DIR}/bin"/*
	fi
	install -t "${BUILD_INSTALL_PREFIX}/lib" "${LINUX_BINDER_OUT_DIR}/lib"/*
	cp -apr "${LINUX_BINDER_OUT_DIR}/include"/* "${BUILD_INSTALL_PREFIX}/include/"

	# Also stage binder SDK to rdk-halif-aidl/out/target for AIDL generation
	echo -e "${BOLD}${CYAN}Staging Binder SDK to rdk-halif-aidl/out/target...${RESET}"
	local rdk_binder_out="${RDK_HAL_DIR}/out/target"
	local rdk_build_out="${RDK_HAL_DIR}/out/build"
	install -d "${rdk_binder_out}/lib" "${rdk_binder_out}/include" "${rdk_binder_out}/bin"
	install -d "${rdk_build_out}" "${rdk_build_out}/include/binder_sdk"
	cp -apr "${LINUX_BINDER_OUT_DIR}"/lib/* "${rdk_binder_out}/lib/" 2>/dev/null || true
	cp -apr "${LINUX_BINDER_OUT_DIR}"/include/* "${rdk_binder_out}/include/" 2>/dev/null || true
	cp -apr "${LINUX_BINDER_OUT_DIR}"/include/* "${rdk_build_out}/include/binder_sdk/" 2>/dev/null || true
	if [ -d "${LINUX_BINDER_OUT_DIR}/bin" ]; then
		cp -apr "${LINUX_BINDER_OUT_DIR}"/bin/* "${rdk_binder_out}/bin/" 2>/dev/null || true
	fi
	touch "${rdk_binder_out}/.sdk_ready"
}

stage_linux_binder_source_for_rdk_hal()
{
	local expected_root="${RDK_HAL_DIR}/build-tools"
	local expected_src="${expected_root}/linux_binder_idl"

	install -d "${expected_root}"
	ln -sfn "${LINUX_BINDER_DIR}" "${expected_src}"

	echo -e "${CYAN}Binder source for rdk-halif-aidl: ${expected_src} -> ${LINUX_BINDER_DIR}${RESET}"
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

	if [ -z "${PREBUILT_AIDL_ROOT}" ]; then
		return 1
	fi

	prebuilt_root="$(realpath "${PREBUILT_AIDL_ROOT}" 2>/dev/null || true)"
	prebuilt_include="${prebuilt_root}/usr/include"
	prebuilt_lib="${prebuilt_root}/usr/lib"

	if [ ! -d "${prebuilt_include}/com/rdk/hal/videodecoder" ] || [ ! -d "${prebuilt_lib}" ]; then
		return 1
	fi

	audio_ver="$(manifest_component_version audiodecoder)"
	avbuffer_ver="$(manifest_component_version avbuffer)"
	videodecoder_ver="$(manifest_component_version videodecoder)"

	audio_lib="libaudiodecoder-v${audio_ver}-cpp.so"
	avbuffer_lib="libavbuffer-v${avbuffer_ver}-cpp.so"
	videodecoder_lib="libvideodecoder-v${videodecoder_ver}-cpp.so"

	if [ ! -f "${prebuilt_lib}/${avbuffer_lib}" ] || [ ! -f "${prebuilt_lib}/${videodecoder_lib}" ]; then
		return 1
	fi

	install -d "${BUILD_INSTALL_PREFIX}/include" "${BUILD_INSTALL_PREFIX}/lib"
	cp -apr "${prebuilt_include}/com" "${BUILD_INSTALL_PREFIX}/include/"

	install -d "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal"
	install -d "${BUILD_DIR}/avbuffer_gen/current/h/com/rdk/hal/avbuffer"
	install -d "${BUILD_DIR}/videodecoder_gen/current/h/com/rdk/hal/videodecoder"
	# Keep shared HAL headers in a known include root used by the mock build.
	cp -f "${prebuilt_include}/com/rdk/hal/"*.h "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/" 2>/dev/null || true
	# Audiodecoder-specific headers are optional for videodecoder mock builds.
	if [ -d "${prebuilt_include}/com/rdk/hal/audiodecoder" ]; then
		install -d "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/audiodecoder"
		cp -f "${prebuilt_include}/com/rdk/hal/audiodecoder"/*.h "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/audiodecoder/" 2>/dev/null || true
	fi
	cp -f "${prebuilt_include}/com/rdk/hal/avbuffer"/*.h "${BUILD_DIR}/avbuffer_gen/current/h/com/rdk/hal/avbuffer/"
	cp -f "${prebuilt_include}/com/rdk/hal/videodecoder"/*.h "${BUILD_DIR}/videodecoder_gen/current/h/com/rdk/hal/videodecoder/"

	cp -f "${prebuilt_lib}/${avbuffer_lib}" "${BUILD_INSTALL_PREFIX}/lib/"
	cp -f "${prebuilt_lib}/${videodecoder_lib}" "${BUILD_INSTALL_PREFIX}/lib/"

	ln -sf "${avbuffer_lib}" "${BUILD_INSTALL_PREFIX}/lib/libavbufferaidl.so"
	ln -sf "${videodecoder_lib}" "${BUILD_INSTALL_PREFIX}/lib/libvideodecoderaidl.so"

	if [ -f "${prebuilt_lib}/${audio_lib}" ]; then
		cp -f "${prebuilt_lib}/${audio_lib}" "${BUILD_INSTALL_PREFIX}/lib/"
		ln -sf "${audio_lib}" "${BUILD_INSTALL_PREFIX}/lib/libaudiodecoderaidl.so"
	fi

	echo "Using prebuilt AIDL artifacts from ${prebuilt_root}"
	echo "Resolved versions: avbuffer=${avbuffer_ver}, videodecoder=${videodecoder_ver}, audiodecoder(optional)=${audio_ver}"

	return 0
}

prepare_aidl_binder_sdk()
{
	python3 -m pip install --quiet colorama 2>/dev/null || true

	if [ ! -f "${RDK_HAL_DIR}/out/target/.sdk_ready" ]; then
		echo "Preparing Binder SDK for AIDL generation..."
		(
			cd "${RDK_HAL_DIR}"
			BINDER_VERSION="${BINDER_VERSION}" \
			BINDER_SOURCE_DIR="${LINUX_BINDER_DIR}" \
			./build_binder.sh
		)
	fi
}

find_host_cmake()
{
	local candidates=("cmake" "cmake3" "/usr/bin/cmake" "/usr/local/bin/cmake" "${OECORE_NATIVE_SYSROOT:-}/usr/bin/cmake")
	local c
	for c in "${candidates[@]}"; do
		[ -z "${c}" ] && continue
		if command -v "${c}" &>/dev/null || [ -x "${c}" ]; then
			local ver
			ver=$("${c}" --version 2>/dev/null | awk 'NR==1{print $3}')
			local major minor
			IFS='.' read -r major minor _ <<< "${ver}"
			if [ "${major:-0}" -gt 3 ] || { [ "${major:-0}" -eq 3 ] && [ "${minor:-0}" -ge 19 ]; }; then
				HOST_CMAKE="${c}"
				echo -e "${GREEN}Using cmake: ${HOST_CMAKE} (version ${ver})${RESET}"
				return 0
			fi
		fi
	done
	echo -e "${BOLD}${RED}ERROR: cmake >= 3.19 is required (CMake file(REAL_PATH) is used by rdk-halif-aidl)${RESET}"
	echo "Install/use newer cmake (e.g. cmake3) or ensure toolchain cmake is in PATH."
	exit 1
}

fix_aidl_ops_shebang()
{
	local aidl_ops="${LINUX_BINDER_DIR}/host/aidl_ops.py"

	if [ ! -f "${aidl_ops}" ]; then
		return 0
	fi

	# Modify shebang to use system python3 explicitly
	# This avoids colorama import issues when ARM toolchain is sourced
	if head -1 "${aidl_ops}" | grep -q "#!/usr/bin/env python3"; then
		echo "Fixing aidl_ops.py shebang to use /usr/bin/python3 explicitly..."
		sed -i '1s|#!/usr/bin/env python3|#!/usr/bin/python3|' "${aidl_ops}"
	fi
}

prepare_hal_cmake_env()
{
	if [ "${TARGET_ARCH}" != "arm" ]; then
		return 0
	fi

	if [ -z "${CC:-}" ] || [ -z "${CXX:-}" ]; then
		echo -e "${BOLD}${RED}ERROR: TARGET_ARCH=arm requires CC/CXX from sourced toolchain env${RESET}"
		exit 1
	fi

	local cc_full="${CC}"
	local cxx_full="${CXX}"
	local cc_bin cc_extra cxx_bin cxx_extra
	local tune_args="${OECORE_TUNE_CCARGS:-}"

	read -ra _CC_PARTS <<< "${cc_full}"
	read -ra _CXX_PARTS <<< "${cxx_full}"
	cc_bin="${_CC_PARTS[0]}"
	cxx_bin="${_CXX_PARTS[0]}"
	cc_extra="${_CC_PARTS[*]:1}"
	cxx_extra="${_CXX_PARTS[*]:1}"

	export CC="${cc_bin}"
	export CXX="${cxx_bin}"

	# Keep ABI/arch flags consistent for compile and link (avoids VFP ABI mismatch).
	if [ -n "${cc_extra}" ]; then
		export CFLAGS="${cc_extra} ${CFLAGS:-}"
		export LDFLAGS="${cc_extra} ${LDFLAGS:-}"
	fi
	if [ -n "${cxx_extra}" ]; then
		export CXXFLAGS="${cxx_extra} ${CXXFLAGS:-}"
	fi
	if [ -n "${tune_args}" ]; then
		export CFLAGS="${tune_args} ${CFLAGS:-}"
		export CXXFLAGS="${tune_args} ${CXXFLAGS:-}"
		export LDFLAGS="${tune_args} ${LDFLAGS:-}"
	fi

	# Fallback for toolchain environments that do not export hard-float ABI flags.
	# This prevents VFP ABI mismatch errors during CMake compiler checks.
	if [[ "${CFLAGS:-} ${CXXFLAGS:-}" != *"-mfloat-abi="* ]]; then
		local abi_flags="-mfloat-abi=hard -mfpu=neon"
		export CFLAGS="${abi_flags} ${CFLAGS:-}"
		export CXXFLAGS="${abi_flags} ${CXXFLAGS:-}"
		export LDFLAGS="${abi_flags} ${LDFLAGS:-}"
	fi

	echo -e "${CYAN}Normalized ARM toolchain for HAL CMake:${RESET}"
	echo "  CC=${CC}"
	echo "  CXX=${CXX}"
	echo "  CFLAGS=${CFLAGS:-}"
	echo "  CXXFLAGS=${CXXFLAGS:-}"
	echo "  LDFLAGS=${LDFLAGS:-}"
}

pre_generate_aidl_dependencies()
{
	local aidl_ops="${LINUX_BINDER_DIR}/host/aidl_ops.py"
	local deps_file="${RDK_HAL_DIR}/out/aidl/dependencies.txt"
	local interface_roots="${RDK_HAL_DIR}/common;${RDK_HAL_DIR}/audiodecoder;${RDK_HAL_DIR}/avbuffer;${RDK_HAL_DIR}/videodecoder"
	local sys_python="/usr/bin/python3"

	if [ ! -f "${aidl_ops}" ]; then
		echo -e "${YELLOW}WARNING: aidl_ops.py not found at ${aidl_ops}, skipping pre-generation${RESET}"
		return 0
	fi

	mkdir -p "${RDK_HAL_DIR}/out/aidl"

	if [ ! -x "${sys_python}" ]; then
		sys_python="python3"
	fi

	if ! "${sys_python}" -c 'import colorama' >/dev/null 2>&1; then
		echo "Installing colorama for ${sys_python}..."
		"${sys_python}" -m pip install --quiet --user colorama >/dev/null 2>&1 || true
	fi

	if ! "${sys_python}" -c 'import colorama' >/dev/null 2>&1; then
		echo -e "${RED}ERROR: colorama is not available for ${sys_python}${RESET}"
		echo "Install it with: ${sys_python} -m pip install --user colorama"
		exit 1
	fi

	echo "Pre-generating AIDL interface dependencies..."
	"${sys_python}" "${aidl_ops}" -a -r "${interface_roots}" -o "${RDK_HAL_DIR}/out/aidl" || {
		echo -e "${RED}ERROR: aidl_ops.py failed${RESET}"
		exit 1
	}

	if [ ! -f "${deps_file}" ]; then
		echo -e "${RED}ERROR: dependencies.txt not generated at ${deps_file}${RESET}"
		exit 1
	fi

	echo -e "${GREEN}✓ AIDL dependencies pre-generated${RESET}"
}

generate_audiodecoder_hal()
{
	declare -r HAL_BUILD_DIR="${BUILD_DIR}/audiodecoder_aidl"
	declare -r AUDIODECODER_VERSION="$(manifest_component_version audiodecoder)"
	declare -r GEN_DIR="${BUILD_DIR}/audiodecoder_gen/${AUDIODECODER_VERSION}"
	declare -r INTERFACE_ROOTS="${RDK_HAL_DIR}/common;${RDK_HAL_DIR}/audiodecoder;${RDK_HAL_DIR}/avbuffer;${RDK_HAL_DIR}/videodecoder"
	declare -r SDK_LIB_DIR="${RDK_HAL_DIR}/out/target/lib"

	rm -rf "${HAL_BUILD_DIR}"

	"${HOST_CMAKE}" \
		-DAIDL_BIN="${AIDL_BIN}" \
		-DAIDL_GEN_DIR="${GEN_DIR}" \
		-DINTERFACE_VERSION="${AUDIODECODER_VERSION}" \
		-DINTERFACE_TARGET=audiodecoder \
		-DINTERFACES_ROOT_DIRS="${INTERFACE_ROOTS}" \
		-DSDK_LIB_DIR="${SDK_LIB_DIR}" \
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
		-B "${HAL_BUILD_DIR}" \
		"${RDK_HAL_DIR}"
}

generate_avbuffer_hal()
{
	declare -r HAL_BUILD_DIR="${BUILD_DIR}/avbuffer_hal"
	declare -r VERSION="$(manifest_component_version avbuffer)"
	declare -r GEN_DIR="${BUILD_DIR}/avbuffer_gen/${VERSION}"
	declare -r INTERFACE_ROOTS="${RDK_HAL_DIR}/common;${RDK_HAL_DIR}/audiodecoder;${RDK_HAL_DIR}/avbuffer;${RDK_HAL_DIR}/videodecoder"
	declare -r SDK_LIB_DIR="${RDK_HAL_DIR}/out/target/lib"

	rm -rf "${HAL_BUILD_DIR}"

	"${HOST_CMAKE}" \
		-DAIDL_BIN="${AIDL_BIN}" \
		-DAIDL_GEN_DIR="${GEN_DIR}" \
		-DINTERFACE_VERSION="${VERSION}" \
		-DINTERFACE_TARGET=avbuffer \
		-DINTERFACES_ROOT_DIRS="${INTERFACE_ROOTS}" \
		-DSDK_LIB_DIR="${SDK_LIB_DIR}" \
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
		-B "${HAL_BUILD_DIR}" \
		"${RDK_HAL_DIR}"
}

generate_videodecoder_hal()
{
	declare -r HAL_BUILD_DIR="${BUILD_DIR}/videodecoder_aidl"
	declare -r VIDEODECODER_VERSION="$(manifest_component_version videodecoder)"
	declare -r GEN_DIR="${BUILD_DIR}/videodecoder_gen/${VIDEODECODER_VERSION}"
	declare -r INTERFACE_ROOTS="${RDK_HAL_DIR}/common;${RDK_HAL_DIR}/audiodecoder;${RDK_HAL_DIR}/avbuffer;${RDK_HAL_DIR}/videodecoder"
	declare -r SDK_LIB_DIR="${RDK_HAL_DIR}/out/target/lib"

	rm -rf "${HAL_BUILD_DIR}"

	"${HOST_CMAKE}" \
		-DAIDL_BIN="${AIDL_BIN}" \
		-DAIDL_GEN_DIR="${GEN_DIR}" \
		-DINTERFACE_VERSION="${VIDEODECODER_VERSION}" \
		-DINTERFACE_TARGET=videodecoder \
		-DINTERFACES_ROOT_DIRS="${INTERFACE_ROOTS}" \
		-DSDK_LIB_DIR="${SDK_LIB_DIR}" \
		-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
		-B "${HAL_BUILD_DIR}" \
		"${RDK_HAL_DIR}"
}

generate_avbufferhelper_skeleton()
{
	declare -r AVBUFFER_HELPER_DIR="${MOCK_SERVICE_DIR}/avbufferhelper"
	mkdir -p "${AVBUFFER_HELPER_DIR}/build"
	pushd "${AVBUFFER_HELPER_DIR}/build" >/dev/null

	cmake ..
	make -j"$(nproc)"

	cp libavbufferhelper.so "${BUILD_DIR}/usr/lib/"
	popd >/dev/null
	rm -rf "${AVBUFFER_HELPER_DIR}/build"
}

build_aidl_libs()
{
	local halif_lib_dir="${RDK_HAL_DIR}/out/target/lib/halif"
	local halif_inc_dir="${RDK_HAL_DIR}/out/target/include/halif"
	local binder_lib_dir="${RDK_HAL_DIR}/out/target/lib"
	local binder_sdk_inc="${RDK_HAL_DIR}/out/build/include/binder_sdk"
	local audio_ver avbuffer_ver videodecoder_ver common_ver

	echo "Building AIDL interface libraries via cmake..."
	"${HOST_CMAKE}" --build "${BUILD_DIR}/videodecoder_aidl" -j"$(nproc)"
	"${HOST_CMAKE}" --build "${BUILD_DIR}/avbuffer_hal" -j"$(nproc)"
	"${HOST_CMAKE}" --build "${BUILD_DIR}/audiodecoder_aidl" -j"$(nproc)"
	echo "AIDL cmake build completed."

	install -d "${BUILD_INSTALL_PREFIX}/lib" "${BUILD_INSTALL_PREFIX}/include"

	install -t "${BUILD_INSTALL_PREFIX}/lib" "${binder_lib_dir}"/lib*.so

	local d
	for d in binder utils android android-base cutils log; do
		[ -d "${binder_lib_dir}/../include/${d}" ] && cp -apr "${binder_lib_dir}/../include/${d}" "${BUILD_INSTALL_PREFIX}/include/"
	done
	[ -d "${binder_sdk_inc}" ] && cp -apr "${binder_sdk_inc}"/. "${BUILD_INSTALL_PREFIX}/include/"

	audio_ver="$(manifest_component_version audiodecoder)"
	avbuffer_ver="$(manifest_component_version avbuffer)"
	videodecoder_ver="$(manifest_component_version videodecoder)"
	common_ver="$(manifest_component_version common)"

	local audio_lib avbuffer_lib videodecoder_lib common_lib
	audio_lib="${halif_lib_dir}/libaudiodecoder-v${audio_ver}-cpp.so"
	avbuffer_lib="${halif_lib_dir}/libavbuffer-v${avbuffer_ver}-cpp.so"
	videodecoder_lib="${halif_lib_dir}/libvideodecoder-v${videodecoder_ver}-cpp.so"
	common_lib="${BUILD_DIR}/videodecoder_aidl/libcommon-v${common_ver}-cpp.so"

	[ -f "${audio_lib}" ] || audio_lib=$(ls "${halif_lib_dir}/libaudiodecoder-v"*"-cpp.so" 2>/dev/null | head -1)
	[ -f "${avbuffer_lib}" ] || avbuffer_lib=$(ls "${halif_lib_dir}/libavbuffer-v"*"-cpp.so" 2>/dev/null | head -1)
	[ -f "${videodecoder_lib}" ] || videodecoder_lib=$(ls "${halif_lib_dir}/libvideodecoder-v"*"-cpp.so" 2>/dev/null | head -1)
	[ -f "${common_lib}" ] || common_lib=$(ls "${BUILD_DIR}/videodecoder_aidl/libcommon-v"*"-cpp.so" 2>/dev/null | head -1)
	[ -f "${common_lib}" ] || common_lib=$(find "${BUILD_DIR}" -maxdepth 2 -type f -name "libcommon-v*-cpp.so" 2>/dev/null | head -1)

	if [ -z "${avbuffer_lib}" ] || [ -z "${videodecoder_lib}" ]; then
		echo "ERROR: cmake did not produce required AIDL libraries in ${halif_lib_dir}"
		ls "${halif_lib_dir}" 2>/dev/null || true
		exit 1
	fi

	cp -f "${avbuffer_lib}" "${BUILD_INSTALL_PREFIX}/lib/$(basename "${avbuffer_lib}")"
	cp -f "${videodecoder_lib}" "${BUILD_INSTALL_PREFIX}/lib/$(basename "${videodecoder_lib}")"
	if [ "$(basename "${avbuffer_lib}")" != "libavbuffer-vcurrent-cpp.so" ]; then
		ln -sf "$(basename "${avbuffer_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libavbuffer-vcurrent-cpp.so"
	fi
	if [ "$(basename "${videodecoder_lib}")" != "libvideodecoder-vcurrent-cpp.so" ]; then
		ln -sf "$(basename "${videodecoder_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libvideodecoder-vcurrent-cpp.so"
	fi

	ln -sf "$(basename "${avbuffer_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libavbufferaidl.so"
	ln -sf "$(basename "${videodecoder_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libvideodecoderaidl.so"

	if [ -n "${audio_lib}" ]; then
		cp -f "${audio_lib}" "${BUILD_INSTALL_PREFIX}/lib/$(basename "${audio_lib}")"
		ln -sf "$(basename "${audio_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libaudiodecoderaidl.so"
		if [ "$(basename "${audio_lib}")" != "libaudiodecoder-vcurrent-cpp.so" ]; then
			ln -sf "$(basename "${audio_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libaudiodecoder-vcurrent-cpp.so"
		fi
	fi

	if [ -n "${common_lib}" ]; then
		cp -f "${common_lib}" "${BUILD_INSTALL_PREFIX}/lib/$(basename "${common_lib}")"
		if [ "$(basename "${common_lib}")" != "libcommon-vcurrent-cpp.so" ]; then
			ln -sf "$(basename "${common_lib}")" "${BUILD_INSTALL_PREFIX}/lib/libcommon-vcurrent-cpp.so"
		fi
	fi

	install -t "${BUILD_INSTALL_PREFIX}/lib" "${halif_lib_dir}"/lib*.so

	local module
	for module in audiodecoder avbuffer videodecoder; do
		local gen_dir="${BUILD_DIR}/${module}_gen/current/h"
		if [ -d "${halif_inc_dir}/com/rdk/hal/${module}" ]; then
			install -d "${gen_dir}/com/rdk/hal/${module}"
			cp -f "${halif_inc_dir}/com/rdk/hal/${module}/"*.h "${gen_dir}/com/rdk/hal/${module}/"
		fi
	done

	if [ -d "${halif_inc_dir}/com/rdk/hal" ]; then
		install -d "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal"
		cp -f "${halif_inc_dir}/com/rdk/hal/"*.h "${BUILD_DIR}/audiodecoder_gen/current/h/com/rdk/hal/" 2>/dev/null || true
	fi

	echo "AIDL Build completed successfully."
}

build_mockservice()
{
	make -C "${MOCK_SERVICE_DIR}" clean || true
	make -j"$(nproc)" -C "${MOCK_SERVICE_DIR}"
}

validate_and_bundle_artifacts()
{
	local required
	for required in \
		"${BUILD_DIR}/bin/MockVideoDecoderService" \
		"${BUILD_DIR}/usr/lib/libvideodecoderaidl.so" \
		"${BUILD_DIR}/usr/lib/libavbufferaidl.so"; do
		if [ ! -e "${required}" ]; then
			echo -e "${BOLD}${RED}Mock-only build failed: missing artifact ${required}${RESET}"
			exit 1
		fi
	done

	mkdir -p "${BUILD_DIR}/mock_bundle"
	cp -f "${BUILD_DIR}/bin/MockVideoDecoderService" "${BUILD_DIR}/mock_bundle/"
	cp -f "${BUILD_DIR}/usr/lib/libvideodecoderaidl.so" "${BUILD_DIR}/mock_bundle/"
	cp -f "${BUILD_DIR}/usr/lib/libavbufferaidl.so" "${BUILD_DIR}/mock_bundle/"

	echo -e "${BOLD}${GREEN}Mock-only build completed. Artifacts: ${BUILD_DIR}/mock_bundle${RESET}"
}

build_mock_only()
{
	ensure_rdk_hal_repo

	BINDER_VERSION="$(resolve_binder_version)"
	echo -e "${BOLD}${GREEN}RDK_HALIF_AIDL_VERSION : ${RESET}${YELLOW}${RDK_HALIF_AIDL_VERSION}${RESET}"
	echo -e "${BOLD}${GREEN}BINDER_VERSION         : ${RESET}${YELLOW}${BINDER_VERSION}${RESET}"
	echo -e "${BOLD}${GREEN}TARGET_ARCH            : ${RESET}${YELLOW}${TARGET_ARCH}${RESET}"
	echo -e "${BOLD}${GREEN}AIDL_MANIFEST          : ${RESET}${YELLOW}$(resolve_manifest_file)${RESET}"

	ensure_linux_binder_repo "${BINDER_VERSION}"
	build_linux_binder_target
	install_linux_binder
	stage_linux_binder_source_for_rdk_hal

	if ! stage_prebuilt_aidl_artifacts; then
		echo "Prebuilt AIDL artifacts unavailable; using pre-generated code from rdk-halif-aidl."
		if ! resolve_aidl_bin; then
			echo -e "${BOLD}${RED}ERROR: aidl binary not found after linux_binder build${RESET}"
			exit 1
		fi
		find_host_cmake
		fix_aidl_ops_shebang
		prepare_hal_cmake_env
		echo -e "${BOLD}${CYAN}Using pre-generated AIDL code from rdk-halif-aidl (no generation needed for release versions)${RESET}"
		pre_generate_aidl_dependencies
		generate_avbuffer_hal
		generate_videodecoder_hal
		generate_audiodecoder_hal
		build_aidl_libs
	fi

	build_mockservice
	validate_and_bundle_artifacts
}

clean_mock()
{
	make -C "${MOCK_SERVICE_DIR}" clean || true
	rm -rf "${BUILD_DIR}/mock_bundle"
}

help()
{
	echo -e "${BOLD}${CYAN}Usage:${RESET}"
	echo "  $0 [Target=ARCH]    Build mock service and required artifacts"
	echo "  $0 clean            Clean mock service outputs"
	echo "  $0 help|-h|--help   Show this message"
	echo
	echo "Target architecture (optional):"
	echo "  Target=linux        Build for x86_64 Linux (default)"
	echo "  Target=arm          Build for ARM with cross-compiler (requires CC/CXX set)"
	echo
	echo "Environment overrides:"
	echo "  RDK_HALIF_AIDL_VERSION (default: main, e.g. 0.21.0)"
	echo "  AIDL_VERSION_MANIFEST (default: versions_released.yaml)"
	echo "  BINDER_VERSION (default: binder_sdk.version pin)"
	echo "  LINUX_BINDER_DIR (default: ${DEF_LINUX_BINDER})"
	echo "  PREBUILT_AIDL_ROOT (optional prebuilt root with usr/include + usr/lib)"
	echo "  TARGET_ARCH (default: linux, set via Target= parameter)"
	echo
	echo "Cross-compilation example (ARM):"
	echo "  source /opt/toolchains/rdk-glibc-x86_64-arm-toolchain/environment-setup-armv7vet2hf-neon-oe-linux-gnueabi"
	echo "  export RDK_HALIF_AIDL_VERSION=0.21.0"
	echo "  $0 Target=arm"
}

# Parse Target=arch parameter
if [[ "${1:-}" == Target=* ]]; then
	TARGET_ARCH="${1#Target=}"
	shift || true
fi

case "${1:-}" in
	help|-h|--help)
		help
		;;
	clean)
		clean_mock
		;;
	"")
		build_mock_only
		;;
	*)
		echo -e "${BOLD}${RED}Invalid argument: $1${RESET}"
		help
		exit 1
		;;
esac
