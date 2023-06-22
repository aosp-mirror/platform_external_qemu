/// AOSP remove_dir_all crate has mismatching version.
/// Since tempdir only requires remove_dir_all function from remove_dir_all
/// crate, a remove_dir_all in std library is used to cover dependency.
pub use std::fs::remove_dir_all;