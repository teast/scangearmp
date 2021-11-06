# scangearmp for Canon printers 

This printers are supported (accorendly to original documentations, I only got an mp495 to test with)

```
mp250
mp280
mp495
mg5100
mg5200
mg6100
mg8100
```

## requirements

I had to install libusb-compat because scangearmp uses the old libusb and I do not have the expectise (nor time) to migrate it to the new and hot libusb.


*NOTE!*
This is a work in progress and not yet fully implemented. The compilation is working but I have yet to "connect" it to the rest of the system.
xsane do work with my mp495 without any "connection" (could be due to cnijfilter...).

## To compile

run:

```bash
./mp495_build.sh
```

This will configure, build and install the driver.

*NOTE!* Currently I only set DESTDIR on install and therefore the install part will try and setup files in your /usr/* folders. If you run `mp495_build.sh` as a normal user you have nothing to fear for any pollusion in your system folders.
