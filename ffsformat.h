/*
Copyright (c) 2011 Adam Green http://mbed.org/users/AdamGreen/
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* Specifies constants and structures used within the FLASH File System.  The
   header is used by both the runtime and the tool which builds the image on
   the PC.
*/
#ifndef _FFSFORMAT_H_
#define _FFSFORMAT_H_


/* The signature to be placed in SFileSystemHeader::FileSystemSignature.
   Only the first 8 bytes are used and the NULL terminator discarded. */
#define FILE_SYSTEM_SIGNATURE "FFileSys"

/* The size of the FLASH on the device to search through for the file
   system signature. */
#define FILE_SYSTEM_FLASH_SIZE  (512 * 1024)


/* Header stored at the beginning of the file system image. */
typedef struct _SFileSystemHeader
{
    /* Signature should be set to FILE_SYSTEM_SIGNATURE. */
    char            FileSystemSignature[8];
    /* Number of entries in this file system image. */
    unsigned int    FileCount;
    /* The SFileSystemEntry[SFileSystemHeader::FileCount] array will start here. 
       These entries are to be sorted so that a binary search can be performed
       at file open time. */
} SFileSystemHeader;

/* Information about each file added to the file system image. */
typedef struct _SFileSystemEntry
{
    /* The 2 following offsets are relative to the beginning of the file 
       image. */
    unsigned int    FilenameOffset;
    unsigned int    FileBinaryOffset;
    unsigned int    FileBinarySize;
} SFileSystemEntry;


#endif /* _FFSFORMAT_H_ */
