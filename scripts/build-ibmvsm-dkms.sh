#!/usr/bin/env bash
# Copyright 2015, IBM
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

## Shell Opts ----------------------------------------------------------------
set -e -u -o pipefail

## Vars ----------------------------------------------------------------------
install_deps=0
debian_build_deps="dkms debhelper linux-headers-$(uname -r)"
basedir="$(git rev-parse --show-toplevel)"
DKMS_SCRIPT=dkms.mkconf

## Exports ----------------------------------------------------------------------
export IBMVSM_MODULE_NAME=${IBMVSM_MODULE_NAME:-"ibmvsm"}
# Note - Do not use a hyphen ('-') as part of the version. See: https://bugs.launchpad.net/dkms/+bug/599983
export IBMVSM_VERSION=${IBMVSM_VERSION:-"$(git describe --tags --long | sed '1s/^.//' | sed 's/-/./g')"}
export IBMVSM_SOURCE_LOCATION=${IBMVSM_SOURCE_LOCATION:-$basedir}
export IBMVSM_SOURCE_DESTINATION=${IBMVSM_SOURCE_DESTINATION:-"/usr/src/${IBMVSM_MODULE_NAME}-${IBMVSM_VERSION}"}
export DKMS_CONF_LOCATION=${DKMS_CONF_LOCATION:-"$IBMVSM_SOURCE_DESTINATION/dkms.conf"}
export IBMVSM_STRIP_DEBUG=${IBMVSM_STRIP_DEBUG:-"yes"}

## Functions ----------------------------------------------------------------------

function usage {
  echo "Usage: $0 [OPTION]..."
  echo ""
  echo "  -i, --install-deps   Install build dependencies (Ubuntu only)"
  echo "  -h, --help           Print this message"
  echo ""
  exit
}

function process_option {
  case "$1" in
    -h|--help) usage;;
    -i|--install_deps) install_deps=1;;
    *) usage;;
  esac
}

## Main ----------------------------------------------------------------------

# Process arguments
for arg in "$@"; do
    process_option $arg
done

echo "Building ibmvsm-dkms package"

# Try to install the dependency packages if specified
if [ $install_deps -eq 1 ]; then
    os_type=$(awk -F= '/^NAME/{print $2}' /etc/os-release | tr -d '"')
    if [ $os_type == "Ubuntu" ]; then
        apt-get update
        apt-get -y install $debian_build_deps
    else
        echo "Installing dependencies on a non-Ubuntu environment not currently supported" && exit 1
    fi
fi

# If the source directory exists remove it
if [ -d "${IBMVSM_SOURCE_DESTINATION}" ];then
    rm -rf "${IBMVSM_SOURCE_DESTINATION}"
fi

# Clean up any old existing DKMS installs from the same version
if [ -d "/var/lib/dkms/${IBMVSM_MODULE_NAME}/${IBMVSM_VERSION}" ]; then
    rm -rf "/var/lib/dkms/${IBMVSM_MODULE_NAME}/${IBMVSM_VERSION}"
fi

# Place the ibmvsm-dkms source
cp -ra "${IBMVSM_SOURCE_LOCATION}" "${IBMVSM_SOURCE_DESTINATION}"

# Build and place the DKMS configuration file with the source
if [ -f "${basedir}/scripts/${DKMS_SCRIPT}" ]; then
    "${basedir}/scripts/${DKMS_SCRIPT}" "-n ${IBMVSM_MODULE_NAME}-dkms" "-v ${IBMVSM_VERSION}" "-f ${DKMS_CONF_LOCATION}" "-d ${IBMVSM_STRIP_DEBUG}"
else
    echo "Missing DKMS build script ${DKMS_SCRIPT}" && exit 1
fi

# Add the ibmvsm module to dkms
dkms add -m "${IBMVSM_MODULE_NAME}" -v "${IBMVSM_VERSION}"

# Build the ibmvsm module
dkms build -m "${IBMVSM_MODULE_NAME}" -v "${IBMVSM_VERSION}"

# Build the ibmvsm-dkms source package
dkms mkdsc -m "${IBMVSM_MODULE_NAME}" -v "${IBMVSM_VERSION}" --source-only

# Build the ibmvsm-dkms debian package
IBMVSM_PACKAGE_LOCATION=$(dkms mkdeb -m "${IBMVSM_MODULE_NAME}" -v "${IBMVSM_VERSION}" --source-only | awk '/built/')

echo "${IBMVSM_PACKAGE_LOCATION}"
echo "The ibmvsm-dkms package was built successfully"
