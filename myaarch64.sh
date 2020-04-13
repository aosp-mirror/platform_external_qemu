#./android/rebuild.sh --target=linux_cross_aarch64   --minbuild --cmake_option WEBRTC=FALSE --noqtwebengine --notests --dist aarch64dist
./android/rebuild.sh --target=linux_cross_aarch64   --minbuild --cmake_option WEBRTC=FALSE --cmake_option QEMU_UPSTREAM=FALSE --noqtwebengine --notests --dist aarch64dist --config debug
