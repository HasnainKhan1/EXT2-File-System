#include "type.h"

int mount_root(char *argv, int argc)
{
    //Initalize a minode pointer.
    MINODE *mip;

    //If the user enters in a file system then the program will run using that file system.
    if(argc > 1)
    {
        mydisk = argv[1];
    }
    //If no file system is entered in the command line then the file system in the project folder is used.
    else
    {
        mydisk = "vdisk";
    }

    //open the file system for read.
    int fd = open(mydisk, O_RDWR);

    dev = fd;

    //read the super block using the get_block function.
    get_block(fd, 1, buf);
    sp = (SUPER *)buf;


    //check if the file system is a EXT2 file system.
    if (sp->s_magic != 0xEF53)
    {
        printf("NOT an EXT2 FS\n");
    }

    nblocks = sp->s_blocks_count;//get the data block counts
    ninodes = sp->s_inodes_count; //get the number of inodes from the superblock
    mtable[0].nblock = nblocks; //gets the number of blocks from SB
    mtable[0].ninodes = ninodes ; //gets the number of inodes from SB

    //get the group descriptor into the buffer.
    get_block(fd, 2, buf);
    gp = (GD *)buf;

    mtable[0].dev = fd;

    bmap = gp->bg_block_bitmap;
    mtable[0].bmap = bmap; //gets block bitmap from GD

    imap = gp->bg_inode_bitmap;
    mtable[0].imap = imap; //gets inode bitmap from GD

    iblock = gp->bg_inode_table;
    mtable[0].iblock = iblock; //get inode start block

    mtable[0].mountDirPtr = root; //let the mount point DIR pointer = root;

    strcpy(mtable[0].deviceName, "vdisk"); //my devices name.
    strcpy(mtable[0].mountedDirName, "/");

    proc[0].cwd = iget(dev, 2);
    proc[1].cwd = iget(dev, 2);

    mtable[0].dev = dev;

    /*this gets the root INODE*/
    root = iget(fd, 2);

    running = &proc[0]; //set the current running process equal to the parent process.
}
