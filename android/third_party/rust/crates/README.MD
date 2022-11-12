# Vendorized Crates

This directory contains a set of symlinks to vendorized
crates that we have made available. In order to bring
in a new crate you:

- Must make sure it is available locally
- Must make sure it went through a proper security review

This can be accomplished by either:

- Using a crate from [AOSP](https://android.googlesource.com/platform/external/rust/crates/). These crates are missing `.cargo-checksum.json` files and cannot be used as direct depdendencies. Instead you will have to [patch](https://doc.rust-lang.org/cargo/reference/overriding-dependencies.html#the-patch-section) them in your workspace definition.

Due to b/258854722 you must add the crate you want to import to the symlink file
after which you can add the symlink here directly.

To use bindgen for AOSP you would add the following section to your workspace:

```rust
[patch.crates-io]
bindgen = { path = "../../../../../external/rust/crates/bindgen" }
```

- Using a crate from [chromium](https://chromium.googlesource.com/chromiumos/third_party/rust_crates/). These are a set of proper vendorized crates that have been reviewed. You make them available by creating a symlink in this directory.

## I want to bring in my own crate (**DON'T DO IT**)

If your crate dependency is not available in chrome or AOSP you will have to vendorize it yourself.

This means:

- You will own the crate (bugfixes, security fixes, etc..)
- You will have to get it reviewed, [see](https://g3doc.corp.google.com/company/thirdparty/android.md) for details.

Please try not to bring in any crates, as the maintenance cost can become high!
