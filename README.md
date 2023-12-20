# Read-only File System
On top of a hard disk simulation file ***SOFTDISK****, *ROFS* provides the following syscalls:
> int open_t(char* pathname)

    Given an absolute path of a file, open_t() returns the corresponding inode number or -1

> int read_t(int inode_number, int offset, void* buffer, int count)

    read_t() attempts to read up to **count** bytes from the file with the inode number **inode_number**,
    starting at **offset**, into **buffer**

    On success, the number of bytes read is returned (0 indicates EOF)
    Otherwise, -1 is returned
*: *A robot friend of mine borrowed me his binary copy of Plato's The Republic, I guess you have a robot friend too*
