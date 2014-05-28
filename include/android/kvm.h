#ifndef KVM_ANDROID_H
#define KVM_ANDROID_H

// Name of environment variable used to control the name of the device
// to use, when it is not /dev/kvm
#define KVM_DEVICE_NAME_ENV  "ANDROID_EMULATOR_KVM_DEVICE"

/* Returns 1 if we can use /dev/kvm on this machine */
extern int kvm_check_allowed(void);

#endif /* KVM_ANDROID_H */
