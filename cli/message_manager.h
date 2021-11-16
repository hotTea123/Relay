#ifndef _MESSAGE_MANAGER_H
#define _MESSAGE_MANAGER_H

#include<vector>

#define HEADLEN 12

struct HEADERMSG    //报文头部，12bit
{
    int srcid;
    int decid;
    int len;
};


#endif