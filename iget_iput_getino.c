#include "type.h"

/*************************************************************************************************************
Load INODE given the device and the inode number of the item you're looking for into a minode array.         *
You will then end up returning a pointer to that minode array.                                               *
When the correct item is found we increment the refCount by 1 and return the pointer to that minode array.   *
Another thing the iget function does is it searches the minode array for a location where the refCount is 0. *
If a location with a refCount of 0 is found then it initialize all the minode array feilds. After all feilds *
have been initalized we use the mailmans algorithm to get a block containing the newly initalized INODE. Disp*
is which INODE in the block we want. We load the blk into a buf and let an INODE pointer point to the INODE  *
in the buf. Lastly copy the INODE into the inode pointers INODE and return.                                  *                                 *
*************************************************************************************************************/
MINODE *iget(int dev, int ino)
{
    int i, blk, disp;
    MINODE *mip;
    char buf[BLKSIZE];

    //search the minode array for the inode that has the same dev and ino that we're looking for.
    for(i = 0; i < NMINODE; i++)
    {
        //let mip point to the minode array
        //find a minode item that has the same dev and ino that we're looking for.
        if((minode[i].dev == dev) && minode[i].ino == ino)
        {
            mip = &minode[i];
            //if we find it we'll increment the refCount and return the found minode item.
            mip->refCount++;
            return mip;
        }
    }

    for(i = 0; i < NMINODE; i++)
    {
        //let mip point to the minode array
        mip = &minode[i];

        //if we find a minode[] with a refCount that is 0 then we initialize the following feilds.
        if(minode[i].refCount == 0)
        {
            mip->refCount = 1;
            mip->dev = dev;
            mip->ino = ino;
            mip->dirty = 0;
            mip->mounted = 0;
            mip->mptr = 0;
            break;
        }
    }
    //use the mailmans algorithm to get the inode blocks and each individual item in it.
    blk = ((ino - 1) / 8) + iblock;
    disp = (ino - 1) % 8;

    //loaded the inode block into the buf.
    get_block(dev, blk, buf);
    ip = (INODE *)buf + disp; //you add disp becasue it is the block you want.

    mip->INODE = *ip;

    return mip;
}

/*************************************************************************************************************
Given the INODE pointer the iput function starts by decrementing the refCount. If the refCount is greater    *
than 1 we will we will return. Otherwise we will check to see if the INODe pointer is marked dirty. If the   *
pointer isn't marked dirty we will return. Otherwise we will calculate the block number containing this INODE*
and we will also calulate the disp, which is the offset of INODE in buf using the mailmans algorithm. We load*
the INODE block that we'll be writing too using get_block. After that we let the INODE pointer point to the  *
item in the minode array that holds the INODE pointer. Lastly we use put_block to save it in our file system.*
**************************************************************************************************************/
int iput(MINODE *mip)
{
    int blk, disp;
    memset(buf, 0, BLKSIZE);
    //INODE *ip;
    //decrement the refCount of the minode that is being pointed at.
    mip->refCount--;

    if(mip->refCount > 0)
    {
        return 1;
    }
    //if the minode that is being pointed at is dirty than return until it has been saved.
    if(!mip->dirty)
    {
        return 1;
    }

    //displays the minode that is being pointed at that will be saved.
    printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);
    //calculates the block and disp at which we will write the INODE back to.
    blk = (((mip->ino) - 1) / 8) + 10;
    disp = ((mip->ino) - 1) % 8;

    //gets that INODE block that we will be writing to and the exact spot.
    get_block(dev, blk, buf);
    ip = (INODE *)buf + disp;


    *ip = mip->INODE;
    //memcpy(ip, &mip->INODE, sizeof(INODE));

    //write back the information to that spot in the actual INODE.
    put_block(dev, blk, buf);

    return 1;
}

/***************************************************************************************************************
This function takes in a pathname and checks to see if it is a complete pathname or not. It used the iget      *
function to get the correct INODE pointer depending on the pathname that is given. Given our tokenize function *
we use it to tokenize the pathname into seprate strings. It then uses a search function to see if the          *
the items do exists within the file system. If the items exist is returns the inode number. If not it returns  *
a 0. After the search function return to us the inode number we can use the iget function to get the INODE     *
pointer. The function return the inode number.                                                                 *
****************************************************************************************************************/
int getino(char *pathname)
{
    int i, ino, blk, disp, n;
    char buf[BLKSIZE], *name[128];
    INODE *ip;
    MINODE *mip;
    dev = root->dev; // only ONE device so far

    if (strcmp(pathname, "/")==0)
        return 2; //because root inode

    if (pathname[0]=='/')
        mip = iget(dev, 2);
    else
        mip = iget(dev, running->cwd->ino);
    memset(buf, 0, BLKSIZE);
    strcpy(buf, pathname);

    n = tokenize(buf, name); // n = number of token strings

    for (i=0; i < n; i++)
    {
        printf("===========================================\n");
        printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

        ino = search(mip, name[i]);

        if (ino==0)
        {
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);
        mip = iget(dev, ino);
    }
    iput(mip);

    return ino;
}

/**
The search function takes an INODE pointer and a string. This function searches through the data blocks of the
INODE that is passed in. It searches the INODE data blocks to see if it can find the item it is looking for.
Using get_block it loades the INODE pointers data blocks into the buf and then it traverses the data block
using a while loop. It using string compare to see if the name passed in is the name that is being pointed
at by the directory pointer dp. If the names are the same then the inode number of that item gets returned.
Else it increments the directory pointer. CP is basically pointing to what is in the buf and we can
increment CP as well by letting the directory pointer point at the rec_len.
**/
int search(MINODE *mip, char *name)
{
    //need to start searching from dir
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int ino, i, j;

    for(i = 0; i < 12; i++)
    {
        if(mip->INODE.i_block[i] == 0)
        {
            return 0;
        }
        get_block(dev, mip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;
        //each block size is 1024
        while(cp < (buf + 1024))
        {
            if(strcmp(name, dp->name) == 0)
            {
                //found inode
                ino = dp->inode;
                return ino;
            }
            //move the cp to start of next inode
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }

    }

    return 0;
}
