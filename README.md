

### OpenSSL
```bash
./Configure linux-generic32 shared \
--prefix=/usr/arm-linux-gnueabihf --openssldir=/usr/arm-linux-gnueabihf/openssl \
--cross-compile-prefix=arm-linux-gnueabihf-
make depend
make -j4
make install
```

### libmosquitto
```bash
cmake -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
-DCMAKE_INSTALL_PREFIX=/usr/arm-linux-gnueabihf ..

make -j4
make install
```

### libfmt
```bash
cmake -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
-DCMAKE_INSTALL_PREFIX=/usr/arm-linux-gnueabihf -DFMT_TEST=OFF ..

make -j4
make install
```