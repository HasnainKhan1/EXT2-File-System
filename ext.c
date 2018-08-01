#include "type.h"

/********** Functions as BEFORE ***********/
int get_block(int dev, int blk, char buf[ ])
{
    lseek(dev, (long)blk*BLKSIZE, SEEK_SET);
    return read(dev, buf, BLKSIZE);
}

int put_block(int dev, int blk, char buf[ ])
{
    lseek(dev, (long)blk*BLKSIZE, SEEK_SET);
    return write(dev, buf, BLKSIZE);

}

/*****************************************************************************************************************
Initalizes the two processes. Sets the parent and child process uid to 0 and its current working directory       *
to NULL. The parent pid is set to 1 and the the child pid id set to 2.                                           *
All the refCounts in the minode array are set to 0 and the root is set to NULL.                                  *
******************************************************************************************************************/
int init()
{
    //sets the parent process uid to 0 and its current working directory to NULL
    proc[0].uid = 0;
    proc[0].cwd = 0;
    proc[0].pid = 1;

    //sets the child process uid to 0 and its current working directory to NULL
    proc[1].uid = 1;
    proc[1].cwd = 0;
    proc[1].pid = 2;

    //sets all refCounts to 0
    for (int i = 0; i < 100; i++)
    {
        minode[i].refCount = 0;
    }
    //set root to NULL
    root = NULL;
}

/*****************************************************************************************************************
The tokenize function takes a pathname and a reference to another string that'll be used to store the            *
indivisual tokenized strings into an array of strings. It returns the number of tokenzied strings.               *
******************************************************************************************************************/
int tokenize(char *buf, char **p)
{
    //returns number of tokens
    int n = 0;

	//tokenize it with either / or \n
    p[0] = strtok(buf, "/\n");
    //for loop to keep traversing through the entire pathname until you reach the end of the string.
	for(n = 0; p[n]; n++)
	{
        p[n+1] = strtok(NULL, " /\n");
	}

	return n;
}

/**finds the corresponding command from the users input and returns it's index.**/
int findCmd(char *command)
{

	char *cmdFunc[] = { "ls", "pwd", "cd", "mkdir", "rmdir", "creat","link", "unlink","stat", "chmod", "symlink",
	"touch", "open", "close", "read", "write", "lseek", "quit", "clear","copy", "mv", "cat", "mount", "umount", "cmd"};


	int i = 0;
	for (i = 0; i < 25; i++)
	{
		if (strcmp(command, cmdFunc[i]) == 0)
			return i;
	}
	return -1;
}

/**
First, start by string comparing the pathname to see if it is empty or not. If it is empty we'll get the inode
number of the current working directory and then call the ls_file function. If a pathname is given then the ls
will list all files in that directory. We will call the ls_file function and pass in the INODE pointer which is
pointing to the INODE.
**/
void ls(char *pathname)
{
    int ino, i = 0, len = 0;
    MINODE *mip;
    char temp[128], *tokenizedPath[128];

    strcpy(temp, pathname);

    //if ls is called without a pathname then we will print out all the files in the current working directory.
    if(strcmp(pathname, " ") == 0)
    {
        ino = running->cwd->ino;
    }
    else
    {
        //tokenize the pathname. Then get the inodes of the pathname.
        len = tokenize(temp, tokenizedPath);

        //if the length of the tokenized pathname is greater than 1 we will get the inode numbers of the individual
        //inode numbers of the pathname passed in.
        if (len > 1)
        {
            for(i = 0; i < len; i++)
            {
                ino = getino(tokenizedPath[i]);
            }
        }
        //Otherwise we will get the inode number of the single pathname is passed in.
        else
        {
            ino = getino(pathname);
        }
    }

    //get the inode pointer using the iget function.
    mip = iget(dev, ino);

    //call the ls_file function by passing in the inode pointer that is pointing the inode pointer.
    ls_file(mip->INODE);
}

/**
This is an auxiliary function which takes an INODE type. We call get_block to get a data block into buf.
We'll declare a directory variable called dr. Set another string cp equal to buf. While we haven't reached the
end of the buf we'll continue to read all the various directories and files within the current directory. We
use a while loop to advance cp by rec_len. It prints out the file name, its size, and its name length.
**/
void ls_file(INODE inode)
{
    int blk = inode.i_block[0];
    char buf[BLKSIZE], temp[256], *cp;

    // get data block into buffer
    get_block(dev, blk, buf);

    //get the dir entry of what is in the buffer.
    dr = (struct ext2_dir_entry_2 *)buf; // as dir_entry

    //set cp equal to the buffer.
    cp = buf;
    printf("inode# rec_len name_len name\n");

    //run through the buffer and print out all the items in it.
    while (cp < buf + BLKSIZE)
    {
        strncpy(temp, dr->name, dr->name_len); // make name a string
        temp[dr->name_len] = 0; // ensure NULL at end

        printf("%d\t %d\t %d\t %s\n", dr->inode, dr->rec_len, dr->name_len, temp);

        cp += dr->rec_len; // advance cp by rec_len
        dr = (struct ext2_dir_entry_2 *)cp;
    }
    printf("\n");
}

/**quits the program.**/
int quit(int *run)
{
    //since the file system is dirty we need to write everything back to it.
    *run = 0;
}

/**************************************************************************************************************
When the user cd's with an empty pathname it'll return back to the home directory of the file system
it'll set the process type runnings current working directory to the root directory if we cd home.
If the user cd's given a pathname then we will get the inode number of the pathname entered and its
correspoinding inode pointer. Before we cd to that directory we will check to see if the inode pointer we
are pointing at is a file or directory. If the inode pointer is pointing to a file we will throw an error
and return. Otherwise we will set the current working directory equal to the inode that is being pointed at.
If this fails we will throw an error and return.
****************************************************************************************************************/
int cd (char *pathname)
{
    MINODE *mip;

    if (strcmp(pathname, " ") == 0)
    {
        running->cwd = root;
        puts("cd to HOME");
    }
    else
    {
        int ino = getino(pathname);
        mip = iget(dev, ino);
        if(S_ISREG(mip->INODE.i_mode))
        {
            puts("Error: This is a reg file.");
            return;
        }
        if(S_ISLNK(mip->INODE.i_mode))
        {
            puts("Error: This is a symlink file");
        }
        if(S_ISDIR(mip->INODE.i_mode))
        {
            //iput the current working directory.
            iput(running->cwd);
            //let the current working directory point to the inode pointer.
            running->cwd = mip;
        }
        else
        {
            puts("cd FAILED");
            return;
        }
    }
}

/****************************************************************************************************************
pwd takes in a inode pointers working directory. If the working directory is the root directory we will print
the backslash. If the working directory is inside another directory we will call the auxiliary rpwd function and
pass in the working directory there to it. After the rpwd function returns we will set the current working
directory to the current working directory.
*****************************************************************************************************************/
void pwd(MINODE *wd)
{
    MINODE *pCwd = running->cwd;
    if(wd == root)
    {
        printf("/");
    }
    else
    {
        rpwd(wd);
    }
    printf("\n");
    running->cwd = pCwd;
}

/*****************************************************************************************************************
Auxiliary function for pwd. This function checks to see if the working directory is equal to the root; if so
it will return. Otherwise we will set the working directories inode number to childIno. We will get the parent
inode number by passing in the parents name which is "..". We will get the parent inode pointer by calling the
iget function and pass in the working directories device and the parent inode number. We will set the current
working directory to the parent inode pointer. We will recurisively call rpwd with the parent inode pointer being
passed in. We will call get_block which will take in the working directory, the parent inode pointers data block,
and also a buffer. We'll declare a directory type called dr and typecast the buffer to be of type dir star.
We'll set cp eqaul to buf and traverse it using a while loop. It'll traverse while cp is less than buffer plus
the block size (1024). Inside the while loop we will string copy into a temporary string the name and length of
the current directory and then print it. We use a while loop to advance cp by rec_len. This will occur until we
print out the names of all the directories until we get to the current working directory.
*****************************************************************************************************************/
int rpwd(MINODE *wd)
{
    int parentIno, childIno;
    char *parent = "..";
    char pTemp[256], buf[BLKSIZE], *cp;
    MINODE *mip, *pmip;

    if (wd == root)
    {
        return;
    }

    childIno = wd->ino;
    parentIno = getino(parent);
    pmip = iget(wd->dev, parentIno);

    running->cwd = pmip;

    rpwd(pmip);

    get_block(wd->dev, pmip->INODE.i_block[0], buf);
    dr = (DIR *)buf;

    cp = buf;

    while(cp < (buf + BLKSIZE))
    {
        strncpy(pTemp, dr->name, dr->name_len);
        pTemp[dr->name_len] = 0;
        if(dr->inode == childIno)
        {
            printf("/%s", pTemp);
        }
        cp += dr->rec_len;
        dr = (DIR *)cp;
    }
}

/*****************************************************************************************************************
Function for mkdir takes a pathname. The pathname is broken up into a dirname and basename and stored in a       *
string called parent and child. We then get the parent inode number using the getino function by passing in the  *
parents name. We then get the parent inode pointer by using the iget function. We do a simple check to see if the*
parent inode pointer is a directory type using the S_ISDIR function. If it is a directory type then we will use  *
our search function see if the new directory we are trying to make already exists. Search should return 0, if it *
doesn't then we will return. Otherwise if the parent directory isn't a directory type we will return. Then       *
we will call the my_mkdir auxiliary function.                                                                    *
******************************************************************************************************************/
int Mkdir(char *pathname)
{
    char parent[215], child[215];
    int pino;
    MINODE *pmip;

    strcpy(parent, dirname(pathname));
    strcpy(child, basename(pathname));

    pino = getino(parent);
    pmip = iget(dev, pino);

    if(S_ISDIR(pmip->INODE.i_mode))
    {
        int ret = search(pmip, child);
        if(ret != 0)
        {
            puts("Dir already exits.");
            return;
        }
    }
    else
    {
        printf("Cannot mkdir %s\n", child);
        return;
    }
    my_mkdir(pmip, child, pino);
}

/*****************************************************************************************************************
Auxiliary function for mkdir. This function takes in a inode pointer, the new directory string, and the parent   *
inode number.  In order to make a directory, we need to allocate an inode from the inodes bitmap, and            *
a disk block from the blocks bitmap, which rely on test and set bits in the bitmaps. In order to maintain        *
file system consistency, allocating an inode must decrement the free inodes count in both superblock and group   *
descriptor by 1. Similarly, allocating a disk block must decrement the free blocks count in both superblock and  *
group descriptor by 1. It is also worth noting that bits in the bitmaps count from 0 but inode and block         *
numbers count from 1. We also need to intialize all the INODE fields so we can make the new directory.           *
REFER TO THE FIELDS BELOW TO ANSWER QUESTIONS ABOUT DIRECTORIES.                                                 *
The enter_child function places the new child and into the file system.                                          *
*****************************************************************************************************************/
int my_mkdir(MINODE *pmip, char *child, int pino)
{
    int ino, blk;

    ino = ialloc(dev);
    blk = balloc(dev);

    MINODE *mip = iget(dev, ino);

    INODE *ip = &mip->INODE; //an INODE pointer ip set equal to the inode pointers INODE type.
    ip->i_mode = 0x41ED; //The type and permissions are set to be a directory type.
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->pid; // group Id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = blk; // new DIR has one data block
    //set all the data blocks to zero.
    for (int i = 1; i < EXT2_N_BLOCKS; i++)
    {
        ip->i_block[i] = 0;
    }
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk

    char buf[BLKSIZE];
    memset(buf, 0, BLKSIZE); // optional: clear buf[ ] to 0

    DIR *dr = (DIR *)buf;

    // make . entry (makes the parent entry)
    dr->inode = ino;
    dr->rec_len = 12;
    dr->name_len = 1;
    dr->name[0] = '.';

    // make .. entry (makes the child entry): pino=parent DIR ino, blk=allocated block
    dr = (char *)dr + 12;
    dr->inode = pino;
    dr->rec_len = BLKSIZE-12; // rec_len spans block
    dr->name_len = 2;
    dr->name[0] = dr->name[1] = '.'; //this will set the child equal to ".."
    put_block(dev, blk, buf); // write to blk on disk

    enter_child(pmip, ino, child);

}

/**tst_bit functions**/
int tst_bit(char *buf, int bit)
{
    return buf[bit/8] & (1 << (bit % 8));
}
/**set_bit functions**/
int set_bit(char *buf, int bit)
{
    buf[bit/8] |= (1 << (bit % 8));
}

/**decrements the free inodes count in the SUPERBLOCK and GD BLOCK**/
int decFreeInodes(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count -= 1;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count -= 1;
    put_block(dev, 2, buf);
    return 1;
}


/**decrements the free blocks count in the SUPERBLOCK and GD BLOCK**/
int decFreeBlocks(int dev)
{
    char buf[BLKSIZE];
    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_blocks_count -= 1;
    put_block(dev, 1, buf);
    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_blocks_count -= 1;
    put_block(dev, 2, buf);
    return 1;
}

/**this function allocates an inode block**/
int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];

    // assume imap, bmap are globals from superblock and GD
    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++)
    {
        if (tst_bit(buf, i)==0)
        {
            set_bit(buf, i);

            put_block(dev, imap, buf);
            // update free inode count in SUPER and GD
            decFreeInodes(dev);
            return (i+1);
        }
    }
    return 0; // out of FREE inodes
}

/**this function allocates an block**/
int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // assume imap, bmap are globals from superblock and GD
    get_block(dev, bmap, buf);

    for (i=0; i < BLKSIZE; i++)
    {
        if (tst_bit(buf, i)==0)
        {
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            // update free inode count in SUPER and GD
            decFreeBlocks(dev);
            memset(buf, 0, BLKSIZE);
            put_block(dev, i+1, buf);
            return (i+1);
        }
    }
    return 0; // out of FREE blocks
}

/*****************************************************************************************************************
This functions takes in a inode pointer, and inode number, and a string that needs to be placed into the file
system. We use the string length of the child in the names length. We then use a for loop going through all 12
direct blocks until we find a free data block. We use get_block to get the correct data block into the buffer.
We typecast the buffer to a DIR star type. We let cp equal the buffer. We use a while loop to advance cp by
rec_len. We will run through the while loop until cp plus the rec_len is less than the buffer plus the
blocksize (1024).

The ideal_length is in a data block of the parent directory, each dir_entry has an ideal length. All dir_entries
rec_lengths equal the ideal_length, except for the last entry. The rec_len of the LAST entry is to the end of the
block, which may be larger than its ideal_length. We let remain is equal to the LAST entry's
rec_len - its ideal_length. If the remain is greater than the ideal_length then enter the new entry as the LAST
entry and trim the previous entry rec_len to its ideal_length. If no space in existing data
block(s): Allocate a new data block. Increment parent size by BLKSIZE. Enter new entry as the first entry in the
new data block with rec_len=BLKSIZE. Fill out all the fields. MORE INFORMATION IS BELOW ON FILLING THE FIELDS.
*****************************************************************************************************************/
int enter_child(MINODE *pip, int ino, char *child)
{
    int i = 0, ideal_length, need_length, remain;

    char buf[BLKSIZE];
    char *cp;

    int n_len = strlen(child);

    ///assume there are only 12 direct blocks.
    for (i = 0; i < 12; i++)
    {
        if(pip->INODE.i_block[i] == 0)
        {
            break;
        }
        get_block(pip->dev, pip->INODE.i_block[i], buf);
        dr = (DIR *)buf;
        cp = buf;

        while (cp + dr->rec_len < buf + BLKSIZE)
        {
            cp += dr->rec_len;
            dr = (DIR *)cp;
        }

        ideal_length = 4*((8 + dr->name_len + 3) / 4);
        need_length = 4 * ((8 + strlen(child) + 3) / 4);
        remain = dr->rec_len - ideal_length;

        //if remain is greater than ideal length then do the following.
        if (remain >= ideal_length)
        {
            dr->rec_len = ideal_length;
            cp +=ideal_length;
            dr = (DIR *) cp;

            dr->inode = ino; //set the directories inode number to the inode number passed in.
            strcpy(dr->name, child); //set the name of the new item by string copying the childs name.
            dr->name_len = strlen(child); //the length of the name is just the string length of the child.
            dr->rec_len = remain; //the rec length is the length of the re

            //write the block back to the disk.
            put_block(pip->dev, pip->INODE.i_block[i], buf);
            break;
        }
        //if remain isn't greater than ideal length then do the following.
        else
        {
            pip->INODE.i_block[i] = balloc(dev); //the parent inodes data block at the location will be set equal a newly allocated block.
            pip->INODE.i_size += BLKSIZE; //the parent inodes size is set equal to the block size.
            pip->INODE.i_blocks += 2; //the parent inodes blocks is set incrmented by 2.

            memset(buf, 0, BLKSIZE); //we memset the buffer.

            get_block(pip->dev, pip->INODE.i_block[i], buf); //we use get_block to read the data block into buffer.

            dr = (DIR *) buf; //typecast the buffer to a DIR star and set equal to a dr type.
            cp = buf; //set cp equal to the buffer.

            dr->rec_len = BLKSIZE; //the rec_len will be set eqaul to the block size.
            dr->name_len = strlen(child); //the name length will equal the length of the new item being passed in.
            dr->inode = ino; //the inode number will equal the inode number that is passed in.
            strcpy(dr->name, child); //we will copy the name of the item passed in into the name of the new item being made.

            put_block(pip->dev, pip->INODE.i_block[i], buf); //we will write the item back to the file system.
            break;
        }
    }
}

/******************************************************************************************************************
This works the same way mkdir works. We take the pathname and divide it into dirname and basename. Next we will
use getino and pass in the parents name to get the parent inode number. Next we will use the iget function to
get the parent inode pointer. We will then call the search function to see if the file we are trying to make
already exists. If search returns 0 then we continue to creat the file, else we return. We then will call
my_creat which takes in the parent inode pointer, the name of the new file, and also the parent inode number.
After my_create returns we will mark the parent inode dirty and write it back to the file system.
******************************************************************************************************************/
int create(char *pathname)
{
    char parent[125], child[125];
    int pino;
    MINODE *pmip;

    strcpy(parent, dirname(pathname));
    strcpy(child, basename(pathname));

    pino = getino(parent);
    pmip = iget(dev, pino);

    int ret = search(pmip, child);
    if(ret != 0)
    {
        if(S_ISREG(pmip->INODE.i_mode))
        {
            puts("Error: existing file");
            return;
        }
        return;
    }

    my_create(pmip, child, pino);

    pmip->dirty = 1;
    iput(pmip);
}

/**
This function takes in a inode pointer, the new file string, and a inode pointer. We start by allocating an inode
number and creating a new inode pointer that takes the mounted device and its new inode number that we allocated.
Next what we do is set a INODE pointer ip equal to the newly allocated mip inode pointer. We will set all the
INODE fields accordingly (SEE BELOW).
**/
int my_create(MINODE *pmip, char *child, int pino)
{
    int ino, blk;

    ino = ialloc(dev);

    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4; //set the type and permissions to files.
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->pid; // group Id
    ip->i_size = 0; // size in bytes
    ip->i_links_count = 1; //the link count gets to set to 1.
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    //data blocks are set to 0.
    for (int i = 1; i < EXT2_N_BLOCKS; i++)
    {
        ip->i_block[i] = 0;
    }
    mip->dirty = 1; // mark minode dirty

    iput(mip); // write INODE to disk

    enter_child(pmip, ino, child); //call the enter_child function to allocate room on the disk and write the new item to the file system.
}

/**rmdir function**/
int rmdir(char *pathname)
{
    int ino = 0, pino = 0, temp = 0, i = 0, count = 0;
    char tempname[256], child[128];
    MINODE *mip, *pmip;

    //(1). get in-memory INODE of pathname
    strcpy(tempname, basename(pathname));

    //if the pathname is empty then return.
    if(strcmp(tempname, " ") == 0)
    {
        printf("Specify Pathname!\n");
        return -1;
    }
    //otherwise we will get the inode number of the directory we are trying to remove.
    else
    {
        ino = getino(tempname);

        //if the inode number is 0 then we will return.
        if(ino <= 0)
        {
            printf("Directory doesn't exist!\n");
            return -1;
        }
    }

    //get the minode pointer of the directory.
    mip = iget(dev, ino);

    //minode is BUSY (refCount = 1) so we will return. Have to cd out of the directory before removing it.
    if(mip->refCount > 1)
    {
        printf("Directory is in use!\n");
        return -1;
    }

    //verify DIR is empty (traverse data blocks for number of entries = 2) IF it isn't empty we will return.
    if(mip->INODE.i_links_count > 2)
    {
        printf("Directory not empty!\n");
        iput(mip);
        return -1;
    }
    if(isEmpty(mip))
    {
        printf("Directory not empty!\n");
        iput(mip);
        return -1;
    }

    //(3).get parent's inode number and the data block into a buffer.
    //we memset the buffer.
    memset(buf, 0, BLKSIZE);
    get_block(dev, mip->INODE.i_block[0], buf);
    printf("mip: %d\n", mip->INODE.i_block[0]);

    //While there is a data block we will do the following:
    while(mip->INODE.i_block[i])
    {
        //get the data block into the buffer.
        get_block(dev, mip->INODE.i_block[0], buf);

        //Typecase the buffer to type DIR.
        DIR *dp = (DIR *)buf;

        //set cp equal to buffer.
        char *cp = buf;

        //while cp is less than the buffer and the blocksize we will do the following:
        while(cp < buf + BLKSIZE)
        {
            count++;
            //string compare the directory name to see if .. is in there. If it is then we will set the parent inode
            //number equal to the current inode number.
            if(strcmp(dp->name, "..") == 0)
            {
                pino = dp->inode;
            }
            //increment the cp buffer by rec length.
            cp+=dp->rec_len;

            //typecast cp to a dir pointer called dp.
            dp = (struct ext2_dir_entry_2 *) cp;
        }
        i++;
    }

    // deallocate its data blocks and inode block
    for(i = 0; i <= 11; i++)
    {
        if(mip->INODE.i_block[i] == 0)
        {
            continue;
        }

        //deallocate the data blocks of the directory.
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }

    //deallocate the inode block.
    idalloc(mip->dev,mip->ino);

    iput(mip); // clears mip->refCount = 0

    //get the minode pointer of the parent directory.
    pmip = iget(dev, pino);

    //call the auxiliary rmchild function by passing in the parent minode pointer and the directory name.
    rm_child(pmip, pathname);

    // decrement pip's link count
    pmip->INODE.i_links_count--;
    pmip->dirty = 1; // mark as dirty
    iput(pmip); //cleanup
}

/**rm_child function**/
int rm_child(MINODE * pmip,char name[])
{
    int i = 0, prev_len = 0, cur_len = 0, run_total = 0;
    char buf[BLKSIZE];

    //get the data block into the buffer.
    get_block(dev, pmip->INODE.i_block[0],buf);

    //while there a data blocks we will do the following:
    while (pmip->INODE.i_block[i])
    {
        //get the first data block into the buffer.
        get_block(dev, pmip->INODE.i_block[0],buf);

        //typecast the buffer into a dir type.
        DIR *dp = (DIR *)buf;

        //set cp equal to the buffer.
        char *cp = buf;

        //While cp is less than the bufferr plus the blocksize we will do the following:
        while (cp < buf + BLKSIZE)
        {
            char temp[128];

            //string copy the directory name into a temporary string.
            strcpy(temp, dp->name);

            //get rid of the null character in the temp string.
            temp[dp->name_len] = 0;

            //if the string len of the name isn't equal to the names length then we string copy the name into a temp
            //string and then we get rid of the null character.
            if (strlen(dp->name) != dp->name_len)
            {
                strcpy(temp, dp->name);
                temp[dp->name_len] = 0;
            }

            //well incrmement run total equal to the rec len.
            run_total += dp->rec_len;
            //we'll set the current length equal to the rec len.
            cur_len = dp->rec_len;

            //calculate the ideal length.
            int ideal_length = 4 *( (8 + dp->name_len + 3)/4 );

            //string compare the name to the name that is passed in or to temp we got from dp->name.
            if(strcmp(dp->name, name) == 0 || strcmp(temp, name) == 0)
            {
                //check to see if the rec length is equal to 1012 if so then we will deallocate the data block.
                //and we'll decrement the file size by blocksize.
                if (dp->rec_len == 1012)
                {
                    //deallocate the data block and we'll decrement the file size by blocksize.
                    bdalloc(pmip->dev, pmip->INODE.i_block[i]);
                    pmip->INODE.i_size -= BLKSIZE;

                    //We'll run through all 12 data blocks.
                    int k = i;
                    while (k < 12 && pmip->INODE.i_block[k+1])
                    {
                        //We'll set the data block
                        pmip->INODE.i_block[k+1] = pmip->INODE.i_block[k];
                    }

                }
                else if (dp->rec_len > ideal_length)
                {
                    //last
                    cp -= prev_len;
                    dp = (DIR *)cp;
                    dp->rec_len += cur_len;
                }
                else
                { //anywhere
                    int len = BLKSIZE - ((cp+cur_len) - buf);
                    memmove(cp, cp+cur_len, len);

                    while (cp + dp->rec_len < &buf[1024] - cur_len)
                    {
                        cp += dp->rec_len;
                        dp = (DIR *)cp;
                    }
                    //Add the deleted length.
                    dp->rec_len += cur_len;
                }
                //put the data block back from the current buffer.
                put_block(dev, pmip->INODE.i_block[i], buf);
                return;
            }

            //set the previous length to the rec length of the current item.
            prev_len = dp->rec_len;

            // advance cp by rec_len
            cp += dp->rec_len;

            // pull dp to next entry
            dp = (struct ext2_dir_entry_2 *)cp;
        }
        i++;
      }
}

/**isEmpty function basically checks to see if the directory **/
int isEmpty(MINODE *mip)
{
  int i;
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], temp[128];

  for(i = 0; i < 12; i++)
  {
    if(ip->i_block[i] == 0) { return 0; }

    get_block(dev, ip->i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    while(cp < buf+BLKSIZE)
    {
      memset(temp, 0, 128);
      strncpy(temp, dp->name, dp->name_len);
      if(strncmp(".", temp, 1) != 0 && strncmp("..", temp, 2) != 0)
      {
        return 1;
      }
      cp += dp->rec_len;
      dp = (DIR*)cp;
    }
  }
}

/**clr_bit function**/
int clr_bit(char *buf, int bit)
{
    //clear bit in char buf[BLKSIZE]
    buf[bit/8] &= ~(1 << (bit%8));
}

/**incFreeInodes function**/
int incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count += 1;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count += 1;
  put_block(dev, 2, buf);
}

/**bdalloc function**/
int bdalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];

    get_block(dev, 3, buf);
    clr_bit(buf, ino-1);
    put_block(dev, 3, buf);
    incFreeInodes(dev);
}

/**idalloc function**/
int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  if (ino > ninodes)
  {
    // niodes global
    printf("inumber %d out of range\n", ino);
    return;
  }
  // get inode bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);
  // write buf back
  put_block(dev, imap, buf);
  // update free inode count in SUPER and GD
  incFreeInodes(dev);
}

/**
This function makes a hard link between two files by takeing a pathname and a second pathname which will then
be split into a basname and a dirname and
stored in seperate variables. Then we will use the use getino to get the old files inode number. If the old files
inode number is something less than 0 then we throw an error and return since the file doesn't exist. Next we will
use the iget function to get the old files inode pointer. We will check to see if the old files inode pointer
is a dictionary, because if it is a dictionary then we will return because you can't link a dictionary to a new
file. Next we will use getino again but this time we'll pass in the new files name. We'll also call iget and pass
in the new files inode number, this will give us the new files inode pointer. Then we'll use search should
return zero since the new file we're about to insert ins't in the directory the parent is in. The search function
will return the inode number of the new file if it already exists which will then throw an error and return. If
the new file doesn't exist we will call the auxiliary function enter_child to store the newly linked file in the
file system. We increment the old files inode pointers links count by 1, we will also mark it dirty and then
write the old file and new file pointer back to the file system.
**/
int link(char *pathname, char *secPathname)
{
    MINODE *omip, *pmip;
    int oino, pino;
    char oldFile[128], newFile[128], parent[128], child[128];

    strcpy(oldFile,pathname);
    //Parent is the new file.
    strcpy(parent, (dirname(secPathname)));

    strcpy(newFile,secPathname);
    //child is the newFile
    strcpy(child, basename(secPathname));


    //verify the old file exists and is not a directory.
    oino = getino(oldFile);

    //if file doesn't exist just return to main.
    if(oino < 0)
    {
        puts("File doesn't exist.");
        return;
    }

    omip = iget(dev, oino);

    //get the inode and chec
    if(S_ISDIR(omip->INODE.i_mode))
    {
        puts("This is a directory.");
        return;
    }

    pino = getino(parent);

    pmip = iget(dev, pino);

    //search should return zero since the new file we're
    //about to insert ins't in the directory the parent is in.
    int ret = search(pmip, child);


    if(ret != 0)
    {
        puts("Error: file already exists.");
        return;
    }

    enter_child(pmip, oino, child);

    omip->INODE.i_links_count++; // inc INODE’s links_count by 1
    omip->dirty = 1; // for write back by iput(omip)
    iput(pmip);
    iput(omip);
}

/**
This function takes a filename and then removes and links it may have and it also removes the file if there are
no links to it. We take the filename and break it up into its basname and dirname. We start by getting the files
inode number by passing in the filename, then we get the minode pointer of the filename. We check to see if the
filename passed in isn't a directory, if it's a directory we will throw an error and return. Otherwise we will
get the parent files inode number and also its minode pointer. We will pass the parent minode pointer and the
file we will be unlinking into the rm_child auxilary function. After we return from rm_child function we will
mark the parent minode pointer dirty and the write it back to the file system. We will also decrement the
link count by 1. Lastly, we'll write the minode pointer back to the disk.
**/
int unlink(char *filename)
{
  int ino, pino;
  MINODE *mip, *pmip;
  char parent[128], child[128];

  //(1).get filenmae's minode and inode number.
  ino = getino(filename);
  mip = iget(dev, ino);

  //check it's a REG or symbolic LNK file; can not be a DIR
  if(S_ISDIR(mip->INODE.i_mode))
  {
      puts("Error: This is a directory.");
      return;
  }
  //(2). // remove name entry from parent DIR’s data block:
  strcpy(parent, dirname(filename));
  strcpy(child, basename(filename));

  pino = getino(parent);
  pmip = iget(dev, pino);

  rm_child(pmip, child);

  pmip->dirty = 1;
  iput(pmip);

  //(3).// decrement INODE's link_count by 1
  mip->INODE.i_links_count--;

  if (mip->INODE.i_links_count > 0)
  {
    mip->dirty = 1;
  }
  // deallocate its block and inode
  else if(mip->INODE.i_links_count == 0)
  {
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev,mip->ino);
  }
  iput(mip);  // release mip
}



int myStat(char *pathname, struct stat *mystat)
{
    int ino;
    MINODE *mip;
    ino = getino(pathname);
    mip = iget(dev, ino);
  // copy entries of INODE into stat struct
	mystat->st_dev = dev;
	mystat->st_ino = ino;
	mystat->st_mode = mip->INODE.i_mode;
	mystat->st_uid = mip->INODE.i_uid;
	mystat->st_size = mip->INODE.i_size;
	mystat->st_blksize = BLKSIZE;
	mystat->st_atime = mip->INODE.i_atime;
	mystat->st_ctime = mip->INODE.i_ctime;
	mystat->st_mtime = mip->INODE.i_mtime;
	mystat->st_gid = mip->INODE.i_gid;
	mystat->st_nlink = mip->INODE.i_links_count;
	mystat->st_blocks = mip->INODE.i_blocks;

	// print the entries of the stat struct
	printf("dev=%d\t", mystat->st_dev);
	printf("ino=%d\t", mystat->st_ino);
	printf("mode=%4x\n", mystat->st_mode);
	printf("uid=%d\t", mystat->st_uid);
	printf("gid=%d\t", mystat->st_gid);
	printf("nlink=%d\t", mystat->st_nlink);
	printf("size=%d\n", mystat->st_size);
	printf("atime=%s", ctime(&(mystat->st_atime)));
	printf("mtime=%s", ctime(&(mystat->st_mtime)));
    printf("ctime=%s", ctime(&(mystat->st_ctime)));

    iput(mip);
}


/*
+------------------------+-----------+
| chmod u=rwx,g=rwx,o=rx | chmod 775 | For world executables files
| chmod u=rwx,g=rx,o=    | chmod 750 | For executables by group only
| chmod u=rw,g=r,o=r     | chmod 644 | For world readable files
| chmod u=rw,g=r,o=      | chmod 640 | For group readable files
| chmod u=rw,go=         | chmod 600 | For private readable files
| chmod u=rwx,go=        | chmod 700 | For private executables
+------------------------+-----------+
*/
int my_chmod(char *pathname, char *secPathname)
{
    int ino, i = ~0x1FF;
    MINODE *mip;
    char *ptr;
    int mode;
    puts(pathname);
    puts(secPathname);

      //convert string to unsigned long integer
    sscanf(pathname, "%x", &mode);
      //mode = atol(pathname);
    printf("mode: %x\n", mode);
    ino = getino(secPathname);

    if(ino <= 0)
    {
        printf("invalid pathname!\n");
        return;
    }

    mip = iget(dev, ino);
    printf("previous permissions: %x\n", mip->INODE.i_mode);
    //bitwise and assignment

    mip->INODE.i_mode &= i;
    //bitwise or assignment

    mip->INODE.i_mode |= mode;
    printf("new permissions: %x\n", mip->INODE.i_mode);
    mip->dirty = 1;
    iput(mip);
}

/**
This function takes a pathname and a second pathname of the file you want to soft link too. We use dirname
and basname functions to split up the pathname and second pathname. We then use the getino function to get the
old files inode number. We check to see if the old files inode number is something less than 0, if so then we
just return. Otherwise we will use iget to get the old files minode pointer. We'll check to see if the old files
minode pointer is a directory, if so then we will return. Afterwards we will use getino and get the new files
inode number, we will also get the parents minode pointer. We will pass the parent minode pointer and the childs
name into the search function to see if the new file exists. If the new file exists then we will return, else we
will do the following:

We will allocate an inode number using ialloc for the new file and we will then create an minode pointer for the
new file. We will intialize all the INODE fields for the new files minode pointer. There is no need to allocate
data blocks or the softlink. String copy the new file into the data blocks of the old file, make sure to typecast
it to a char star since we're storing a string in it. Then we call the enter_child function to make room for the
new file that we just soft linked. Mark the parent and child minode pointer as dirty and use iput to write them
back to the disk.
**/
int sym_link(char *pathname, char *secPathname)
{
    MINODE *omip, *pmip;
    int oino, pino, ret, ino, blk;;
    char oldFile[60], newFile[60], parent[60], child[60];

    ///Step 1
    strcpy(oldFile, pathname);
    strcpy(parent, (dirname(secPathname)));

    strcpy(newFile, secPathname);
    strcpy(child, (basename(secPathname)));

    //gets the old files inode number.
    oino = getino(oldFile);

    if(oino < 0)
    {
        puts("Error: File doesn't exist.");
        return;
    }

    omip = iget(dev, oino);


    if(S_ISDIR(omip->INODE.i_mode))
    {
        puts("Error: This is a directory.");
        return;
    }

    pino = getino(parent);
    pmip = iget(dev, pino);

    ret = search(pmip, child);

    if(ret < 0)
    {
        puts("Error: File already exists");
        return;
    }

    ///Step 2
    ///Creat new_file and change the type to sLink;
    ino = ialloc(dev);

    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;

    ip->i_mode = 0xa1ff; //permission and type for the files.
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->pid; // group Id
    ip->i_size = strlen(oldFile); // file name size gets stored in the i_size.
    ip->i_links_count = 1; //set the links count to 1
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); //time of access.
    ip->i_blocks = 0; // LINUX: Blocks count in 512-byte chunks

    ///Step 3
    strcpy((char *)ip->i_block, oldFile); //string copy the new file into the data blocks of the old file.

    enter_child(pmip, ino, child); //use enter child to make room for the file in the file system

    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk

    pmip->dirty = 1; //mark the parent minode as dirty.
    iput(pmip); //place it back into the file system/disk

}

/**
This function takes in a pathname and breaks it into its respective dirname and basname. We then take the old
files and get its inode. We take the inode number and get the old files minode pointer as well. We use search to
see if the file we are trying to touch exists. If the file we are trying to touch does exist then we just update
its acess time. If the file doesn't exist then we will create the file. I could've just called the creat function
but I decided to just write everything myself.
**/
int touch(char *pathname)
{
    char parent[125], child[125];
    int pino, cino;
    MINODE *pmip, *cmip;

    strcpy(parent, dirname(pathname));
    strcpy(child, basename(pathname));

    pino = getino(parent);

    pmip = iget(dev, pino);

    int ret = search(pmip, child);
    printf("ret: %d\n", ret);
    if(ret != 0)
    {
        puts("File exists, updating its access time.");
        cino = getino(child);
        cmip = iget(dev, cino);

        INODE *ip = &(cmip->INODE);
        ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);

        return;
    }
    puts("File doesn't exists, creating file.");
    my_touch(pmip, child, pino);

    pmip->dirty = 1;
    iput(pmip);
}

/**This function is exactly the same as my_creat function which is an auxiliary function used to make a file that
doesn't already exist. It does the same thing that my_crea does.
**/
int my_touch(MINODE *pmip, char *child, int pino)
{
        int ino, blk;

    ino = ialloc(dev);

    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4;
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->pid; // group Id
    ip->i_size = 0; // size in bytes
    ip->i_links_count = 1;
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    for (int i = 1; i < EXT2_N_BLOCKS; i++)
    {
        ip->i_block[i] = 0;
    }
    mip->dirty = 1; // mark minode dirty

    iput(mip); // write INODE to disk

    enter_child(pmip, ino, child);
}

int command(void)
{
    puts("Here is a list of command.");
    puts("[ls|pws|cd|mkdir|rmdir|creat|link|unlink|stat|chmod|symlink|touch|open|close|read|write|lseek|copy|move|cat|mount|unmount|clear|quit]");
    return;
}
