.. SPDX-License-Identifier: GPL-2.0+
======================================================
IBM Virtual Management Channel Kernel Driver (IBMVMC)
======================================================

:Authors:
	Bryant G. Ly <bryantly@linux.vnet.ibm.com>,

Introduction
============

Note: Knowledge of virtualization technology is required to understand
this document.

Virtual Serial Multiplex (VSM) Input/Output Adapter (IOA) allows a high-
priviledged partition to serve multiple simultaneous partner
partition's client Virtual Terminal IOAs. Management consoles are able
to indirectly interact with a single VSM IOA to access multiple
simultaneous partner partition's client virtual serial sessions up to
a maximum number of simultaneous connections.

The VSM driver uses the mechanisms of the Reliable Command/Response
Transport (single sided) and an extended class of Virtual Terminal
Hypervisor Calls (HCALLS) to manage, service, and send virtual serial
traffic to the hypervisor.

Additional Information
======================

For more information on the documentation for CRQ Messages and signal
messages please refer to the Linux on Power Architecture Platform
Reference. Section F.
