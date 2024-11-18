# How to build

1) Install blurring following these instructions. Photon requires version 2.3
   as a strict requirement

```shell
wget https://github.com/axboe/liburing/archive/refs/tags/liburing-2.3.tar.gz -O - | tar -xvz && \
    cd liburing-liburing-2.3 && export CFLAGS="-fPIC -O3 -Wall -Wextra -fno-stack-protector" \ 
    && ./configure --prefix=/usr/local && make -C src -j $(nprocs) && sudo make install \
    && sudo rm -rf liburing-liburing-2.3
```