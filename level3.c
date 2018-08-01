#include "type.h"

int my_mount(char *filesys, char *directory)
{
    MTABLE *newMount;
    int i = 0, fd, ino;
    SUPER *sb;
    char buf[BLKSIZE];
    MINODE *mip;
    //part 1
    if(strcmp(filesys, "") == 0)
    {
        puts("No file system entered. Using default file system currently mounted.");
        for(i = 0; i < 4; i++)
        {
            printf("Default file system: %s\n", mtable[i].deviceName);
            printf("Directory mounted: %s\n", mtable[i].mountedDirName);
        }
        return;
    }

    //check whether the filesys is already mounted
    for(i = 0; i < 4; i++)
    {
      if(strcmp(mtable[i].deviceName, filesys) == 0)
      {
        puts("this filesystem is already mounted.");
        return;
      }
    }

    //allocate a free mount table entry
    for(i = 0; i < 4; i++)
    {
      if(mtable[i].dev == 0)
      {
        newMount = &(mtable[i]);
        break;
      }
    }

    //get file descriptor
    fd = open(filesys, O_RDWR);
    if(!fd)
    {
      puts("invalid file descriptor");
      return;
    }
    dev = fd;
    //super block
    get_block(fd, 1, buf);
    sb = (SUPER*)buf;


    //check if its ext2 filesystem
    if (sb->s_magic != 0xEF53)
    {
        puts("Not a EXT2 file system!");
        return;
    }

    ino = getino(directory);
    mip = iget(dev, ino);

    //Check if mip pointer is a DIR
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        puts("NOT A DIR");
        return;
    }

    //Check if busy
    if (running->cwd == mip->ino)
    {
        puts("BUSY");
        return;
    }

    //record dev and filesystem name
		newMount->dev = dev;
		strcpy(newMount->deviceName, filesys);
        strcpy(newMount->mountedDirName, directory);

		newMount->ninodes = sb->s_inodes_count;
		newMount->nblock = sb->s_blocks_count;

}

int my_umount(char *pathname)
{
  MTABLE *newMount;
  int i = 0, counter = 0;
  if(strcmp(pathname, "") == 0)
  {
    puts("enter a filesystem to unmount!!!");
    return;
  }

  for(i=0; i < 4; i++)
  {
    if(strcmp(mtable[i].deviceName, pathname) == 0)
		{
			counter++;
      break;
    }
  }

  if(counter == 0)
  {
    puts("This device is not mounted!");
    return;
  }
}
