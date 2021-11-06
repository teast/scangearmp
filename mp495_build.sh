#!/bin/sh

abort () {
    echo $1
    exit $2
}

DESTDIR=/opt/scangearmp2

pushd scangearmp || abort "could not change to scangearmp folder" 1

trap popd EXIT
libtoolize -cfi || abort "Error while running libtoolize -cfi" 1
./autogen.sh --prefix=/usr LDFLAGS="-L`pwd`/../com/libs_bin64" || abort "Error from autogen.sh" 1

make || abort "Error from make" 1
make DESTDIR=$DESTDIR install || abort "Error from make DESTDIR=$DESTDIR install" 1

echo "All done!"