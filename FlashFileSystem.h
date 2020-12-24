/* Copyright 2011 Adam Green (http://mbed.org/users/AdamGreen/)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
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
class FlashFileSystemFileHandle : public mbed::FileHandle 
{
public:
    FlashFileSystemFileHandle();
    FlashFileSystemFileHandle(const char* pFileStart, const char* pFileEnd);
    
    // FileHandle interface methods.
    virtual ssize_t write(const void* buffer, size_t length) override;
    virtual int close() override;
    virtual ssize_t read(void* buffer, size_t length) override;
    virtual off_t seek(off_t offset, int whence) override;
    virtual off_t size() override;

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

    /** Check for poll event flags
     * You can use or ignore the input parameter. You can return all events
     * or check just the events listed in events.
     * Call is nonblocking - returns instantaneous state of events.
     * Whenever an event occurs, the derived class should call the sigio() callback).
     *
     * @param events        bitmask of poll events we're interested in - POLLIN/POLLOUT etc.
     *
     * @returns             bitmask of poll events that have occurred.
     */
    virtual short poll(short events) const override
    {
        // Only readable is true
        return POLLIN;
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
    virtual int    close() override;
    virtual ssize_t read(struct dirent *ent) override;
    virtual void   rewind() override;
    virtual off_t  tell() override;
    virtual void   seek(off_t location) override;

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



/** A filesystem for accessing a read-only file system placed in the internal\n
 *  FLASH memory of the mbed board.
 *\n
 *  The file system to be mounted by this file system should be created through\n
 *  the use of the fsbld utility on the PC.\n
 *\n
 *  As fsbld creates two output files (a binary and a header file), there are two\n
 *  ways to add the resulting file system image:\n
 *  -# Concatenate the binary file system to the end of the .bin file created\n
 *     by the mbed online compiler before uploading to the mbed device.\n
 *  -# Import the header file into your project, include this file in your main\n
 *     file and add 'roFlashDrive' to the FlashfileSystem constructor call.\n
 *     eg : static FlashFileSystem flash("flash", roFlashDrive);\n
 *
 *  A third (optional) parameter in the FlashfileSystem constructor call allows\n
 *  you to specify the size of the FLASH (KB) on the device (default = 512).\n
 *  eg (for a KL25Z device) : static FlashFileSystem flash("flash", NULL, 128);\n
 *  Note that in this example, the pointer to the header file has been omitted,\n
 *  so we need to append the binary file system ourselves (see above).\n
 *  When you use the binary file system header in your main file, you can\n
 *  use the roFlashDrive pointer.\n
 *  eg (for a KL25Z device) : static FlashFileSystem flash("flash", roFlashDrive, 128);\n
 *\n
 *  NOTE: This file system is case-sensitive.  Calling fopen("/flash/INDEX.html")\n
 *        won't successfully open a file named index.html in the root directory\n
 *        of the flash file system.\n
 *
 * Example:
 * @code
#include <mbed.h>
#include "FlashFileSystem.h"
// Uncomment the line below when you imported the header file built with fsbld
// and replace <Flashdrive> with its correct filename
//#include "<FlashDrive>.h"

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
    // NOTE : When you include the the header file built with fsbld,
    //        disable the first static FlashFileSystem... line
    //        and enable the second static FlashFileSystem... line.
    static FlashFileSystem flash("flash");
//    static FlashFileSystem flash("flash", roFlashDrive);
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
class FlashFileSystem : public mbed::FileSystemLike 
{
public:
    FlashFileSystem(const char* pName, const uint8_t *pFlashDrive = NULL, const uint32_t FlashSize = 512);
    
    virtual int open(FileHandle** file, const char* pFilename, int Flags) override;
    virtual int  open(DirHandle** dir, const char *pDirectoryName) override;

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
