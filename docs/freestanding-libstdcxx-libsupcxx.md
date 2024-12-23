# Freestanding libstdc++ and libsupc++:

It's simple really. But still no freestanding crt*.o (technically mightn't be possible). Working on that now.

## Command summary:

$ ../gcc-14.2.0/configure --target=i586-elf --disable-nls --enable-languages=c,c++ --prefix=$(HOME)/bin/i586-elf --without-headers --with-static-standard-libraries --disable-hosted-libstdcxx --enable-libstdcxx-static-eh-pool --with-libstdcxx-eh-pool-obj-count=64
$ make -j8 all-gcc all-target-libgcc all-target-libstdc++-v3

## Flag explanations:

### Definitely useful/important:

* --with-static-standard-libraries: Tells GCC's configure to build libgcc and libstdc++ as static libs (*.a, *.la).
* --disable-hosted-libstdcxx: This is the magic that tells GCC's build system to build for a freestanding target.
* --enable-libstdcxx-static-eh-pool: This is useful because it makes libsupc++ manage a static pool of memory which it uses for holding exceptions if malloc() fails. I.e: it *should* enable your kernel to use exceptions early on in the boot process, before the memory manager has been initialized.
* --with-libstdcxx-eh-pool-obj-count=64: This tells libsupc++ how much static memory to allocate for early exception handling.

### Other interesting and potentially useful flags:

--disable-tls
--disable-linux-futex
--disable-libstdcxx-threads
--disable-libstdcxx-filesystem-ts
--disable-libstdcxx-backtrace
--enable-libstdcxx-static-eh-pool
--enable-version-specific-runtime-libs
