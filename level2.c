#include "type.h"
#define atoa(x) #x

/**Open function**/
int f_open(char *filename, char *flags)
{
    int ino, index, modeType;
    char oFile[60];
    MINODE *mip;
    OFT *ft;

    strcpy(oFile, filename);///string copy the filename passed in.

    ///STEP 1:
    ino = getino(oFile); //get Files inode number.

    ///check to see if file exits otherwise create the file.
    if(ino == 0)
    {
        create(oFile);
        ino = getino(oFile);
    }

    ///if the file exists then get its inode.
    mip = iget(dev, ino);

    //depending on what flag is passed in we set the according modeType to the right number.
    if(strcmp(flags, "r") == 0)
    {
        puts("Open for read");
        modeType = 1;
    }
    else if (strcmp(flags, "w") == 0)
    {
        puts("Open for write.");
        modeType = 2;
    }
    else if(strcmp(flags, "rw") == 0)
    {
        puts("Open for read/write.");
        modeType = 3;
    }
    else if (strcmp(flags, "append") == 0)
    {
        puts("Open file for append");
        modeType = 4;
    }
    else
    {
        puts("Error: Flag unrecognized.");
        return;
    }

    ///STEP 2
    INODE *ip = &(mip->INODE);

    ///Check to see the files permissions.
    if(!S_ISREG(ip->i_mode))
    {
        printf("Error: Cannot open directory for %s\n", flags);
        return;
    }


    ///STEP 3:
    ///Allocate an openTable entry.
    ft = (OFT *)malloc(sizeof(OFT));

    ///intialize all openTable entries
    ft->mode  = modeType;
    ft->mptr = mip;
    ft->offset = 0; //offset is 0 unless the flag is for append mode.
    ft->refCount = 1;

    ///set the offset to file size if it is in APPEND mode.
    if(modeType == 4)
    {
        ft->offset = ip->i_size;
    }

    ///STEP 4
    //Find a available open file table entry and then let the file descriptor point to that open Table entry.
    for(index = 0; index < NFD; index++)
    {
        if(proc->fd[index] == 0)
        {
            proc->fd[index] = ft; ///let the fd point to the openTable entry.
            break;
        }
    }
    //This displays all current open file descriptors to the users.
    puts("Current open FDs:");
    openFD();

    printf("fd %d is on file %s. File size %d\n", index, oFile, ip->i_size);
    return index; //return the index of the file descriptor.
}

/**Close function**/
int my_close(char *file)
{
    int fd = atoi(file); //turns the passed in file descriptor into an integer.

    //checks to see if the file descriptor is in range.
    if((fd < 0) || (fd > 16))
    {
        puts("Error: fd out of range.");
    }
    //check to see if the refCount is above 0. If it is then do the following.
    if(running->fd[fd]->refCount != 0)
    {
        //decrement the refCount and check to see if its 0. If it is 0 then we will iPut the minode pointer.
        //back into the file system.
        if(--running->fd[fd]->refCount == 0)
        {
            iput(running->fd[fd]->mptr);//if you've written to a file and it needs to get put back you have to use this.
        }
    }
    running->fd[fd] = 0; //clear the file descriptor.
    printf("File descriptor %d is closed\n", fd);

    puts("Current open FDs:");
    openFD();
    return;
}

/**myRead function**/
int myRead(char *file, char *bytes)
{
    //convert the bytes that are passed into the read function.
    int size = atoi(bytes);
    char str[10];
    //This converts the integer size into a string to display to the user.
    snprintf(str, 10, "%d", size);
    //initialize the buf function to be of size bytes passed in plus 1. We add 1 so we don't get an out of bounds
    //error.
    char buf[size + 1];
    //call the auxiliary read file function in which we pass in the file, the buffer, and the bytes we want to read
    read_file(file, buf, str);
}

/**read_file function**/
int read_file(char *file, char *buf, char *bytes)
{
    int fd, nbytes, count = 0, avail, lblk, blk, start, remain;
    char rbuf[BLKSIZE], *cq, buf1[BLKSIZE];

    //fd is set eqaul to the file which is converted into an interger.
    fd = atoi(file);
    printf("fd: %d\n", fd);

    //convert the bytes passed into an integer.
    nbytes = atoi(bytes);
    printf("nbytes: %d\n", nbytes);

    //check to see if the file descriptor is a valid file descriptor.
    if (fd < 0 || fd > 15)
    {
        printf("INVALID FILE DESCRIPTOR\n");
        return -1;
    }
    //check to see if the file is a valid open file.
    if (running->fd[fd] == NULL)
    {
        printf("FILE NOT OPEN\n");
        return -1;
    }
    //step 2
    //Avail holds the amount of bytes that are avaiable in the file to be read.
    //We basically subtract the file size from the offset which is the pointer in the file.
    avail = ((running->fd[fd]->mptr->INODE.i_size) - (running->fd[fd]->offset));

    //we set the buffer equal to a temporary buffer.
    cq = buf;

    //step 3
    //We will keep reading while the number of bytes passed in and while there is stuff available to read.
    while(nbytes && avail)
    {
        //using the mailman algorithm to get the logical block and start is the specific
        //byte in the file we want to begin reading from.
        lblk = running->fd[fd]->offset/BLKSIZE;
        start = running->fd[fd]->offset%BLKSIZE;

        //step 4
        //check for direct and indirect block
        //There are pointers to the first 12 blocks which contain the file's data in the inode.
        //Pointers 1 to 12 point to direct blocks, pointer 13 points to an indirect block.
        //pointer 14 points to a doubly indirect block.

        //Here we will convert logical blocks to physical blocks. We first deal with the direct blocks.
        if(lblk < 12)
        {
            blk = running->fd[fd]->mptr->INODE.i_block[lblk];
        }
        //indirect blocks have 2^8 cells
        else if(lblk >= 12 && lblk < 268)
        {
            //this is accessing the indirect blocks. Stores inot the buffer.
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[lblk], buf1);

            //set a reference that starts at 13
            blk = buf[lblk-12];
        }
        //This is dealing with the double indirect blocks.
        else
        {
            //getting the double indirect block.
            get_block(running->fd[fd]->mptr->dev, running->fd[fd]->mptr->INODE.i_block[lblk], buf1);
            //were getting the physical block of the double indirect block we just got and are storing inside the buffer.
            blk = buf1[(lblk - 12) - (256 * ((lblk - 12) / 256))];
        }

        //step 5
        //read the block into the buffer so we can see what's in the file.
        get_block(running->fd[fd]->mptr->dev, blk, rbuf);

        //sets a pointer to the start of the buffer.
        char *cp = rbuf + start;

        //remain is what is remaining in the buffer.
        remain = BLKSIZE - start;

        //step 6
        //While there is information in the file we continue to parse it.
        //We copy bytes from one buffer to another buffer.
        while(remain)
        {
            //cq is the size of the file plus 1 and we're setting cq equal to cp so everything gets copied over one at a time.
            *cq++ = *cp++;

            //increment the offset with each increment of the file.
            running->fd[fd]->offset++;

            //counts the characters in the file to display the number to the user.
            count++;

            //decrement the amount that is available to read in the file, the bytes we've read, and what remains to be read.
            avail--; nbytes--; remain--;

            //if the amount of bytes and availabe bytes left to read is less than or eqaul to 0 we just break;
            if (nbytes <= 0 || avail <= 0)
            {
                break;
            }
        }
    }
    printf("myRead: read %d char from file descriptor %d\n", count, fd);

    //prints out the information contained inside the buffer.
    puts(buf);
    return count; //count is the actual number of bytes read
}

/**myWrite function**/
int myWrite(char *file)
{
    //take the passed in file and convert its file descriptor to an integer.
    int fd  = atoi(file);

    char str[256];

    //validate the file descriptor to see if it is open and that the refCount is valid.
    if(running->fd[fd] && running->fd[fd]->refCount)
    {
        //ask user for string to write in the file
        fgets(str, 256, stdin);
        //call the write file function by passing in the file descriptor, the string, and the length of the string the
        //user wrote.
        write_file(fd, str, strlen(str));
    }
    //If the file isn't open then just return.
    else
    {
        printf("File is not open!\n");
        return;
    }
}

/**write_file function**/
int write_file(int fd, char *buf, int nbytes)
{
    //part 1
    //set the open file table type equal to the open file descriptor.
    OFT *oftp = running->fd[fd];
    //get the minode pointer of the open file descriptor and stores it in an minode pointer.
    MINODE *mip = oftp->mptr;

    //part 2
    int count = 0, lblk, start, remain, blk;
    char *cq = buf;
    char mybuf[BLKSIZE];

    //part 3
    //while the number bytes the user wants to write or are written are still available.
    while(nbytes)
    {
        //using the mailman algorithm to get the logical block and start is the specific location in the
        //file we want to begin writing to.
        lblk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;

        //converts logical block into a physical block and then allocates a direct block to write to if need be.
        if(mip->INODE.i_block[lblk] == 0)
        {
            mip->INODE.i_block[lblk] = balloc(mip->dev);
        }

        //let the physical block equal the data block that we are writing to.
        blk = running->fd[fd]->mptr->INODE.i_block[lblk];

        //part 5
        get_block(mip->dev, blk, mybuf);
        char *cp = mybuf + start;
        remain = BLKSIZE - start;

        //while we have items remaining to write we will continue to write to the file.
        while(remain)
        {
            //We will increment cp while writing each character to it from cq.
            *cp++ = *cq++;
            //increment the offset and the count of how many things we have written.
            oftp->offset++; count++;
            //decrement the amount that is available to read in the bytes we've read and what remains to be read.
            remain--; nbytes--;

            //If the offset is greater than the size we will increment the size in the minode file.
            if(oftp->offset > mip->INODE.i_size)
            {
                mip->INODE.i_size++;
            }
            //If the number bytes is less than or equal to 0 we will break.
            if(nbytes <= 0)
            {
                break;
            }
        }

        //put whatever is in the buffer back into the file system so that it gets written.
        put_block(mip->dev, blk, mybuf);
    }

    //mark the minode pointer dirty.
    mip->dirty = 1;
    //put the minode pointer back into the file system.
    iput(mip);

    printf("Wrote %d characters into file descriptor %d\n", count, fd);
    return count;
}

/**myCat function**/
int myCat(char *pathname)
{
    char buf[BLKSIZE];
    int fd, num;

    //open file and get file descriptor
    fd = f_open(pathname, "r");

    //converts a number into a string.
    char str[10];
    snprintf(str, 10, "%d", fd);

    //get the size of the file.
    int size = ip->i_size;

    //converts a number into a string.
    char str2[10];
    snprintf(str2, 10, "%d", size);

    //if the size is less than 1023 we will read in the file and save it to a buffer which we passed in as refercence.
    //We will then print out what is in the buffer.
    if(size < 1023)
    {
        num = read_file(str, buf, str2);
        printf("%s\n", buf);
    }
    //Otherwise we will read the file and keep printing it out because it is a really big file. We use a while loop to
    //see if the number of bytes read in are how many bytes in the file we are reading.
    else
    {
        num = read_file(str, buf, "1023");
        //Keep reading until the total file has been read in.
        while(num)
        {
            printf("%s\n", buf);
            num = read_file(str, buf, "1023");
        }
    }
    //close the file descriptor we are reading.
    my_close(str);
}

/**my_move function**/
int my_move(char *filename, char *pathname)
{
    char tempFile[60], tempPathname[60];
    int oino, dino;
    MINODE *omip, *dmip;

    //copy the filename and pathname into a temp string.
    strcpy(tempFile, filename);
    strcpy(tempPathname, pathname);

    //get the files inode number
    oino = getino(tempFile);

    //If the file inode number is 0 then we couldn't find the file and we will return.
    if(oino = 0)
    {
        puts("Error: File doesn't exist.");
        return;
    }

    //get the minode pointer of the file you're trying to move.
    omip = iget(dev, oino);

    //get the size of the file.
    int size = ip->i_size;

    //initalize the buffer to be the size of the file plus 1 to account for an out of bound error.
    char buf[size +1];

    //used to convert the number into a string.
    char str[10];
    snprintf(str, 10, "%d", size);

    //get the directories inode number.
    dino = getino(tempPathname);
    printf("dino: %d\n", dino);

    //if directory doesn't exist then we'll return.
    if(dino < 0)
    {
        puts("Error: Directory doesn't exist.");
        return;
    }

    //get the directory minode pointer.
    dmip = iget(dev, dino);

    //open the file for read.
    int fd = f_open(filename, "r");

    //converts a number (file descriptor) into a string.
    char str1[10];
    snprintf(str1, 10, "%d", fd);

    //read the file and store it into a buffer that is passed in as a reference.
    read_file(str1, buf, str);

    //close the file we were reading.
    my_close(str1);

    //cd into the directory that is passed in.
    cd(pathname);

    //creates a file of the file name that is passed in.
    create(filename);

    //open the file that was passed in for write mode.
    int fd2 = f_open(filename, "w");

    //write the file that we read in to the new file we created in the new directory.
    write_file(fd2, buf, size);

    //convert the file descriptor integer into a string.
    char sFD[10];
    snprintf(sFD, 10, "%d", fd2);

    //close the file descriptor.
    my_close(sFD);

    //go back to the previous directory.
    cd(" ");

    //unlink the file that is being kept in the current directory.
    unlink(filename);

}

/**lseek function**/
int my_lseek(int filename, int value)
{
    //convert the passed in filename and bytes into intergers.
    int fd = atoi(filename);
    int nBytes = atoi(value);

    int size;
    //check to see if the file descriptor passed in is a valid file descriptor.
    if(running->fd[fd] == 0)
    {
        puts("Error: File is not valid");
        return;
    }
    //Check to see if the file descriptor is open for read, write, or read/write mode.
    if((proc->fd[fd]->mode != 1) || (proc->fd[fd]->mode != 2) || (proc->fd[fd]->mode != 3))
    {
        puts("Not open for read, write, or read/write.");
        return;
    }
    //Don't really know why we have this here.
    if(running->fd[fd] && running->fd[fd]->refCount > 0)
    {
        //Need to get the minode pointer for the file so I can use its INODE to get its file size.
        size = running->fd[fd]->mptr->INODE.i_size;

        //If the file is open for read mode then we will set the offset to where the user wants to begin reading
        //from. If the place the user wants to read from is out of bounds then we throw an error and return.
        if(running->fd[fd]->mode == 1)
        {
            //Check to see if the bytes the user wants to read is within the bounds.
            if(nBytes <= size)
            {
                running->fd[fd]->offset = nBytes;
            }
            else
            {
                puts("Error: File size out of bound");
                return;
            }
        }
        //If the file is opened for write mode then we set the files offset to where ever the user wants to set it.
        else if(running->fd[fd]->mode == 2)
        {
            running->fd[fd]->offset = nBytes;
        }

        printf("The offset is set to %d of the file\n", running->fd[fd]->offset);
        return nBytes;
    }
}

/**Open file desciptors function**/
int openFD(void)
{
    int index = 0;
    for (index = 0; index < NFD; index++)
    {
        if(running->fd[index] != 0)
        {
            printf("FD[%d] is open\n", index);
        }
    }
    return;
}

/**myCopy function**/
int myCopy(char *pathname, char *pathname2)
{
    //check to see if the pathnames are empty. Else return.
    if (strcmp(pathname, "") == 0 || strcmp(pathname2, "") == 0)
    {
        puts("Supply valid filenames");
        return;
    }

    int fd, fd2;

    //open the file for read.
    fd = f_open(pathname, "r");

    //Convert the file descriptor number back into a character.
    char str[10];
    snprintf(str, 10, "%d", fd);

    //get the files size.
    int size = ip->i_size;

    //convert the file size into char.
    char S1[10];
    snprintf(S1, 10, "%d", size);

    //intialize a buffer the size of the file and plus 1 so we don't go out of bounds.
    char buf[size+1];
    int num;

    //read the file and return the number of bytes read.
    num = read_file(str, buf, S1);

    //close the file.
    my_close(str);

    //open a file for write.
    fd2 = f_open(pathname2, "w");

    //convert the file descriptor number into a string.
    char str2[10];
    snprintf(str2, 10, "%d", fd2);

    //write the file to the second file that we passed in.
    write_file(fd2, buf, size);

    //close the file that we wrote to.
    my_close(str2);
}


