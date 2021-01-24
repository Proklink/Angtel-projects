# Hello World

```console
# sudo insmod ./hello_world.ko
# dmesg | tail -n 10
[...]
[23290.504017] Hello, World!
# lsmod | grep hello_world
# sudo rmmod hello_world
# dmesg | tail -n 10
[...]
[23412.080424] Goodbye, World!
```
