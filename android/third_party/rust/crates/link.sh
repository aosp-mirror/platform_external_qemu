#!/bin/bash
find ../../../../../rust/chromium/crates/vendor -type d -name "$1*" -exec echo ln -sf {} $\(basename {}\) \;
