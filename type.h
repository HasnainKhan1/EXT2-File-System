#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdbool.h>

int dev;
/**** same as in mount table ****/
int nblocks; // from superblock
int ninodes; // from superblock
int bmap;    // bmap block
int imap;    // imap block
int iblock;  // inodes begin block

/*************** type.h file ******************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

#define BLKSIZE     1024
#define NMINODE      100
#define NFD           16
#define NPROC          2

typedef struct minode
{
    INODE INODE;
    int dev, ino;
    int refCount;
    int dirty;
    int mounted;
    struct mntTable *mptr;
}MINODE;
//file table
typedef struct oft
{
    int  mode;
    int  refCount;
    MINODE *mptr;
    int  offset;
}OFT;

typedef struct proc
{
    struct proc *next;
    int          pid;
    int          uid;
    int          gid;
    MINODE      *cwd;
    OFT         *fd[NFD];
}PROC;

typedef struct mntTable
{
    int dev;         // dev number: 0=FREE
    int nblock;      // s_blocks_count
    int ninodes;     // s_inodes_count
    int bmap;        // bmap block#
    int imap;        // imap block#
    int iblock;      // inodes start block#
    MINODE *mountDirPtr;
    char deviceName[64];
    char mountedDirName[64];
}MTABLE;

// globals
MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
MTABLE mtable[4];

SUPER *sp;
GD    *gp;
INODE *ip;
//added a ext2 DIR entry type.
DIR   *dr;

char buf[BLKSIZE];
char *mydisk;


int get_block(int dev, int blk, char buf[ ]);
int put_block(int dev, int blk, char buf[ ]);
int init();
int tokenize(char *buf, char **p);
char* dir_name(char* pathname);
char* base_name(char* pathname);
int findCmd(char *command);
void ls(char *pathname);
void ls_file(INODE inode);
int quit(int *run);
int cd (char *pathname);
void pwd(MINODE *wd);
int rpwd(MINODE *wd);
int rmdir(char *pathname);
int rm_child(MINODE *pmip, char *child);
int clr_bit(char *buf, int bit);
int incFreeInodes(int dev);
int bdalloc(int dev, int ino);
int idalloc(int dev, int ino);
