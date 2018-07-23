ibmvsm - IBM Virtual Serial Multiplex kernel module
====================

NOTE
----
This kernel module currently uses DKMS until it can be merged upstream.
Support for DKMS can be used in one of two ways:

- Manually triggering a DKMS build from this repository, as explained below
- Generating and installing the ibmvsm-dkms package using the instructions below and the tooling contained in this repository


Overview
--------
The ibmvsm kernel module provides a device driver with interfaces supporting
interaction with the IBM PowerPC Virtual Serial Multiplex (VSM) device.


License
-------
The license can be found in the LICENSE file. It must be reviewed prior to use.


Manually Building and Installing with DKMS
------------------------------------------
To build and install this code using DKMS manually, do the following
Clone down this repository on the machine you want to build/install on
On the same machine, copy the ibmvsm repo to /usr/src/ibmvsm-<version>

- Install prerequisite packages:
    - dkms
    - debhelper
    - linux-headers-$(uname -r)
- Register the module with dkms using "sudo dkms add -m ibmvsm -v <version>"
- Build the module using dkms by running "sudo dkms build -m ibmvsm -v <version>"
- Install the package using dkms by running "sudo dkms install -m ibmvsm -v <version>"
- Modprobe the ibmvsm module


Building the ibmvsm DKMS Source Package
---------------------------------------
NOTE: The following dependencies are required when building the ibmvsm-dkms package:

- dkms
- debhelper
- linux-headers-$(uname -r)

To build the DKMS package, run, with sudo/root authority:

- scripts/build-ibmvsm-dkms.sh [-i]
  - i.e. sudo IBMVSM_VERSION=1.0 ./scripts/build-ibmvsm-dkms.sh
- Note: Do not use hyphens in the version

Running this script will perform the following steps:

- If not installed, the dependencies listed above can be installed with the -i flag
- If not manually set, the ibmvsm version will be automatically generated using git tags
- The ibmvsm source code will be placed at /usr/src/ibmvsm-<version>
- The ibmvsm module will be added to DKMS
- The ibmvsm module will be built
- A debian source package will be built for ibmvsm-dkms
- A binary debian package containing this source will be built
- The build files will be cleaned up


Installing the DKMS Package
---------------------------
Install the package built in the previous step using:

- dpkg -i ibmvsm-dkms-<version>

As part of the ibmvsm-dkms package installation

- The ibmvsm module will be installed into the kernel module tree through DKMS
- The ibmvsm module will be loaded


Uninstalling the DKMS Package
-----------------------------

To uninstall the package, run:

- dpkg -r ibmvsm-dkms

Doing this will:

- Remove the ibmvsm module from the kernel module tree using DKMS
- Remove the ibmvsm source package from the system
