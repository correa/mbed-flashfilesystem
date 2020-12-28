# FLASH File System for mbed 6

A file system which can mount, read, and enumerate a file system image which has been appended to the compiled .bin code file before being uploaded to the mbed device.

This file system can mount, read, and enumerate a file system image which has been appended to the compiled .bin code file before being uploaded to the mbed device. I wrote a utility called fsbld to create images that can be used with this file system. [This GitHub repository](https://github.com/adamgreen/fsbld) contains the sources for that utility.

To get the file system image onto the mbed device, you need to concatenate your binary from the compiler with the file system image binary.

On *nix this can be done with the cat command. For example:

```cat Test_LPC1768.bin FileImage.bin >/Volumes/MBED/test.bin```

On Windows this can be done with the copy command. For example:

```copy Test_LPC1768.bin + FileImage.bin e:\test.bin```

# Original code.

This repository is a port to mbed 6 from original author Adam Green for mbed 5 on [os.mbed.com](https://os.mbed.com/users/AdamGreen/code/FlashFileSystem/)

