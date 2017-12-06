#!/bin/bash

# This is to run cts on oc-mr1-emu-dev/release branch only
# and it may not work on other branches.

# create avds: multiple avds are preferred for first run
# as cts is capable of useing --shards <n>; not suitable
# for retry (cts BUG !!!)


# launch n emulators

# launch cts with shards <n>

# for loop: repeatedly launch emulator, run cts
# as long as we are making progress: not very fun,
# it is just how cts works, as of now.
