A 6TisCH node that demonstrates initial synchronization of 6TiSCH networks using adaptive scan period and compares the connection time with fixed optimal scan period. Core implementation can be found at `os/net/mac/tsch/tsch-adaptive-scan.h`. The overview of os  modifications and the sample implementation can be seen [here](https://github.com/tejasvi/6tisch-adaptive-scan/compare/develop...project#diff-1668b253a38f79989d24a0d1d8962b9b91fa326c5c1afbe2ba2b83ea4cf5bf4c).

Modes of operation
------------------

Checkout `examples/6tisch/adaptive-scan/project-conf.h` for available options. Primarily:
* `ADAPTIVE_SCAN = 1` enables adaptive scan period else it fallsback to optimum scan period as specified in Optimal Initial Synchronization Time in Minimal 6TiSCH Configuration by A. Karalis et al.
* `BROADCAST_BIAS = N` biases the broadcast channel selection by the advertisor to the integeral `N` extent.
* `TSCH_CONF_DEFAULT_HOPPING_SEQUENCE` specifies the broadcast channel sequence used by the advertisors.
* `TSCH_CONF_EB_PERIOD` specifies the multiple of slotframe duration used as the delay between two consecutive EBs.

By default, the node assign themselves the role of reciever and advertisor in the ratio of `3:1`.

Cooja setup
---------------------

To install Cooja:

```
sudo apt update
sudo apt install gcc-msp430 git make ant gcc-arm-none-eabi -y
git clone --recursive --depth=1 <this repository url> adaptive-scan-repo
cd adaptive-scan-repo/tools/cooja && ant run
```

Open the simulation `examples/6tisch/adaptive-scan/cooja-sim.csc` (or compile `node.c`) in Cooja and then start the simulation. To view the result status, filter the logs with `Result status` and for final results `Results`. To save the log data, select the logs (<kbd>Ctrl+A</kbd>) and then choose `Copy selected` in the `Edit` menu.
For detailed logs, enable `ADAPTIVE_SCAN_DEBUG` and enter `DATA` in the log filter.
