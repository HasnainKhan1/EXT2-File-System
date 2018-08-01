#include "type.h"

///NEED TO FIX UNLINK SO THAT WE CAN GET THE INO NUMBER BACK FOR A FILE.

int main(int argc, char *argv[], char *env[])
{
    char cmd[60], pathname[60] = {0}, line[60], secPathname[60] = {0};
    int index;
    struct stat mystat;
    int *run = 1, value = 12;

    init();

    mount_root(*argv, argc);

    while(run == 1)
    {
        memset(pathname, 0, 60);

        printf("Enter a Command: ");

        fgets(line, 60, stdin);

        line[strlen(line)-1] = 0;

        sscanf(line, "%s %s %s", cmd, pathname, secPathname);
        if(!pathname[0])
        {
            strcpy(pathname, " ");
        }
        index = findCmd(cmd);
        printf("index: %d\n", index);
        switch(index)
        {
            case -1:puts("Error: Not a valid command");         break;
            case 0: ls(pathname);                               break;
            case 1: pwd(running->cwd);                          break;
            case 2: cd(pathname);                               break;
            case 3: Mkdir(pathname);                            break;
            case 4: rmdir(pathname);                            break;
            case 5: create(pathname);                           break;
            case 6: link(pathname, secPathname);                break;
            case 7: unlink(pathname);                           break;
            case 8: myStat(pathname, &mystat);                  break;
            case 9: my_chmod(pathname, secPathname);            break;
            case 10: sym_link(pathname, secPathname);           break;
            case 11: touch(pathname);                           break;
            case 12: f_open(pathname, secPathname);             break;
            case 13: my_close(pathname);                        break;
            case 14: myRead(pathname, secPathname);             break;
            case 15: myWrite(pathname);                         break;
            case 16: my_lseek(pathname, secPathname);           break;
            case 17: quit(&run);                                break;
            case 18: system("clear");                           break;
            case 19: myCopy(pathname, secPathname);             break;
            case 20: my_move(pathname, secPathname);            break;
            case 21: myCat(pathname, secPathname);              break;
            case 22: my_mount(pathname, secPathname);           break;
            case 23: my_umount(pathname);                       break;
            case 24: command();                                 break;

        }
    }

    return 0;
}
