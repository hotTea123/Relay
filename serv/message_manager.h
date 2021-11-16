#ifndef _MESSAGE_MANAGER_H
#define _MESSAGE_MANAGER_H

#include<vector>

#define HEADLEN 12
#define STRLEN 10

struct HEADERMSG    //报文头部，12bit
{
    int srcid;
    int decid;
    int len;
};                                   

// typedef struct DATAMSG     //数据报文
// {
//     HEADERMSG head;
//     std::vector<char> data;
// }DATAMSG;

#endif