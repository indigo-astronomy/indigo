# Hardware blocker and continuation

K-m discovery works; USBclaim(-99) blocks allphysical capture/control/timing verification, evenUID0. No VMentitlement. SystemMassStorage restored afterdiagnostics; no physical unplug. Details: tethering-research.md.

After thischeckpoint is preserved, close the app holding the USBuserclient and run independently from terminal withcameraawake:

```sh
cd /private/tmp/indigo-complete-pentax
sudo /usr/bin/python3 tools/run_driver_test.py ccd_pentax --hw --type 'PENTAX K-m'
```

Proposed continuation, NOT executed/pass. One groupedshort run, snapshot/restore settings, no SD/carddeletion/firmwareoperations. Closing ChatGPT is not proven sufficient. No automatic appshutdown authorized.
