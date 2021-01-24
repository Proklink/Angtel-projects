/etc/confdb-features
/.etc/persistent/confdb-features

ietf-if-extension: max-frame-size


# libnl
autoreconf -i -f
./configure --prefix=/home/root/applications/libnl-build
make -j4

# hsr_interface
autoreconf -i -f
cd ./.build-x86
export PKG_CONFIG_PATH="/home/root/applications/libnl-build/lib/pkgconfig"
../configure
make
