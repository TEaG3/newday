# SMB status in the OPL-only TEa.G3 build

SMB game launch is currently **disabled** in this TEa.G3 branch.

The launcher no longer embeds or invokes Neutrino. The former Neutrino SMB backend and its modules are therefore not part of the runtime package. The OPL ETH code in the upstream OPL source is not wired to the TEa.G3 launcher in this build, so its presence in upstream source is not evidence of a working TEa.G3 SMB launch route.

Current verified launcher routes:
- USB/BDM ISO and ZSO: routed to OPL's BDM direct-launch path (physical PS2 validation is still pending).
- USBExtreme UL: routed to OPL's UL direct-launch path (physical PS2 validation is still pending).

SMB catalog browsing, SMB ISO/ZSO launch, UDPBD, UDPFS, MMCE and APA/HDL are not enabled as PS2 game launch sources. They must remain unavailable in the UI until an OPL-native adapter is wired through TEa.G3, tested in CI, and validated on real hardware.

Do not copy Neutrino runtime files, add Neutrino launch flags, or describe the old Neutrino SMB configuration as a supported workaround. Existing configuration keys may be retained for compatibility, but they do not enable a launch route.
