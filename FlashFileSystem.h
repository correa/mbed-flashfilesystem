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
/* Specifies the classes used to implement the FlashFileSystem which is a
   read-only file system that exists in the internal FLASH of the mbed
   device.
*/
#ifndef _FLASHFILESYSTEM_H_
#define _FLASHFILESYSTEM_H_

#include "FileSystemLike.h"


// Forward declare file system entry structure used internally in 
// FlashFileSystem.
struct _SFileSystemEntry;



// Represents an opened file object in the FlashFileSystem.
class FlashFileSystemFileHandle : public FileHandle 
{
public:
    FlashFileSystemFileHandle();
    FlashFileSystemFileHandle(const char* pFileStart, const char* pFileEnd);
    
    // FileHandle interface methods.
    virtual ssize_t write(const void* buffer, size_t length);
    virtual int close();
    virtual ssize_t read(void* buffer, size_t length);
    virtual int isatty();
    virtual off_t lseek(off_t offset, int whence);
    virtual int fsync();
    virtual off_t flen();

    // Used by FlashFileSystem to maintain entries in its handle table.
    void SetEntry(const char* pFileStart, const char* pFileEnd)
    {
        m_pFileStart = pFileStart;
        m_pFileEnd = pFileEnd;
        m_pCurr = pFileStart;
    }
    int IsClosed()
    {
        return (NULL == m_pFileStart);
    }
    
protected:
    // Beginning offset of file in FLASH memory.
    const char*         m_pFileStart;
    // Ending offset of file in FLASH memory.
    const char*         m_pFileEnd;
    // Current position in file to be updated by read and seek operations.
    const char*         m_pCurr;
};


// Represents an open directory in the FlashFileSystem.
class FlashFileSystemDirHandle : public DirHandle
{
 public:
    // Constructors
    FlashFileSystemDirHandle();
    FlashFileSystemDirHandle(const char*              pFLASHBase,
                             const _SFileSystemEntry* pFirstFileEntry,
                             unsigned int             FileEntriesLeft,
                             unsigned int             DirectoryNameLength);
                             
    // Used by FlashFileSystem to maintain DirHandle entries in its cache.
    void SetEntry(const char*              pFLASHBase,
                  const _SFileSystemEntry* pFirstFileEntry,
                  unsigned int             FileEntriesLeft,
                  unsigned int             DirectoryNameLength)
    {
        m_pFLASHBase = pFLASHBase;
        m_pFirstFileEntry = pFirstFileEntry;
        m_pCurrentFileEntry = pFirstFileEntry;
        m_FileEntriesLeft = FileEntriesLeft;
        m_DirectoryNameLength = DirectoryNameLength;
    }
    int IsClosed()
    {
        return (NULL == m_pFirstFileEntry);
    }
    
    // Methods defined by DirHandle interface.
    virtual int    closedir();
    virtual struct dirent *readdir();
    virtual void   rewinddir();
    virtual off_t  telldir();
    virtual void   seekdir(off_t location);

protected:
    // The first file entry for this directory.  rewinddir() takes the
    // iterator back to here.
    const _SFileSystemEntry*    m_pFirstFileEntry;
    // The next file entry to be returned for this directory enumeration.
    const _SFileSystemEntry*    m_pCurrentFileEntry;
    // Pointer to where the file system image is located in the device's FLASH.
    const char*                 m_pFLASHBase;
    // Contents of previously return directory entry structure.
    struct dirent               m_DirectoryEntry;
    // This is the length of the directory name which was opened.  When the
    // first m_DirectoryNameLength characters change then we have iterated
    // through to a different directory.
    unsigned int                m_DirectoryNameLength;
    // The number of entries left in the file system file entries array.
    unsigned int                m_FileEntriesLeft;
};



/** A filesystem for accessing a read-only file system placed in the internal
 *  FLASH memory of the NXP chip on the mbed board.
 *
 *  The file system to be mounted by this file system should be created through
 *  the use of the fsbld utility on the PC and the resulting file system image
 *  concatentated to the end of the .bin file created by the mbed online
 *  compiler before uploading to the mbed device.
 *
 *  NOTE: This file system is case-sensitive.  Calling fopen("/flash/INDEX.html")
 *        won't successfully open a file named index.html in the root directory
 *        of the flash file system.
 *
 * Example:
 * @code
#include <mbed.h>
#include "FlashFileSystem.h"

static void _RecursiveDir(const char* pDirectoryName, DIR* pDirectory = NULL)
{
    DIR* pFreeDirectory = NULL;
    
    size_t DirectoryNameLength = strlen(pDirectoryName);
 
    // Open the specified directory.
    if (!pDirectory)
    {
        pDirectory = opendir(pDirectoryName);
        if (!pDirectory)
        {
            error("Failed to open directory '%s' for enumeration.\r\n", 
                  pDirectoryName);
        }
        
        // Remember to free this directory enumerator.
        pFreeDirectory = pDirectory;
    }
        
    // Determine if we should remove a trailing slash from future *printf()
    // calls.
    if (DirectoryNameLength && '/' == pDirectoryName[DirectoryNameLength-1])
    {
        DirectoryNameLength--;
    }
    
    // Iterate though each item contained within this directory and display
    // it to the console.
    struct dirent* DirEntry;
    while((DirEntry = readdir(pDirectory)) != NULL) 
    {
        char RecurseDirectoryName[256];
        DIR* pSubdirectory;

        // Try opening this file as a directory to see if it succeeds or not.
        snprintf(RecurseDirectoryName, sizeof(RecurseDirectoryName),
                 "%.*s/%s",
                 DirectoryNameLength,
                 pDirectoryName,
                 DirEntry->d_name);
        pSubdirectory = opendir(RecurseDirectoryName);
        
        if (pSubdirectory)
        {
            _RecursiveDir(RecurseDirectoryName, pSubdirectory);
            closedir(pSubdirectory);
        }
        else
        {
            printf("    %.*s/%s\n", 
                   DirectoryNameLength, 
                   pDirectoryName, 
                   DirEntry->d_name);
        }
    }
    
    // Close the directory enumerator if it was opened by this call.
    if (pFreeDirectory)
    {
        closedir(pFreeDirectory);
    }
}

int main() 
{
    static const char* Filename = "/flash/index.html";
    char*              ReadResult = NULL;
    int                SeekResult = -1;
    char               Buffer[128];

    // Create the file system under the name "flash".
    static FlashFileSystem flash("flash");
    if (!flash.IsMounted())
    {
        error("Failed to mount FlashFileSystem.\r\n");
    }

    // Open "index.html" on the file system for reading.
    FILE *fp = fopen(Filename, "r");
    if (NULL == fp)
    {
        error("Failed to open %s\r\n", Filename);
    }
    
    // Use seek to determine the length of the file
    SeekResult = fseek(fp, 0, SEEK_END);
    if (SeekResult)
    {
        error("Failed to seek to end of %s.\r\n", Filename);
    }
    long FileLength = ftell(fp);
    printf("%s is %ld bytes in length.\r\n", Filename, FileLength);
    
    // Reset the file pointer to the beginning of the file
    SeekResult = fseek(fp, 0, SEEK_SET);
    if (SeekResult)
    {
        error("Failed to seek to beginning of %s.\r\n", Filename);
    }
    
    // Read the first line into Buffer and then display to user.
    ReadResult = fgets(Buffer, sizeof(Buffer)/sizeof(Buffer[0]), fp);
    if (NULL == ReadResult)
    {
        error("Failed to read first line of %s.\r\n", Filename);
    }
    printf("%s:1  %s", Filename, Buffer);
    
    // Done with the file so close it.
    fclose(fp);                               

    // Enumerate all content on mounted file systems.
    printf("\r\nList all files in /flash...\r\n");
    _RecursiveDir("/flash");
        
    printf("\r\nFlashFileSystem example has completed.\r\n");
}
 * @endcode
 */
class FlashFileSystem : public FileSystemLike 
{
public:
    FlashFileSystem(const char* pName);
    
    virtual FileHandle* open(const char* pFilename, int Flags);
    virtual DirHandle*  opendir(const char *pDirectoryName);

    virtual int         IsMounted() { return (m_FileCount != 0); }

protected:
    FlashFileSystemFileHandle*  FindFreeFileHandle();
    FlashFileSystemDirHandle*   FindFreeDirHandle();
    
    // File handle table used by this file system so that it doesn't need
    // to dynamically allocate file handles at runtime.
    FlashFileSystemFileHandle   m_FileHandles[16];
    // Directory handle table used by this file system so that it doesn't need
    // to dynamically allocate file handles at runtime.
    FlashFileSystemDirHandle    m_DirHandles[16];
    // Pointer to where the file system image is located in the device's FLASH.
    const char*                 m_pFLASHBase;
    // Pointer to where the file entries are located in the device's FLASH.
    const _SFileSystemEntry*    m_pFileEntries;
    // The number of files in the file system image.
    unsigned int                m_FileCount;
};

#endif // _FLASHFILESYSTEM_H_
