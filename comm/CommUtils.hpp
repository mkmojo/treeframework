#ifndef COMMUTILS_HPP
#define COMMUTILS_HPP
#include <vector>
#include <queue>
#include <iostream>

extern const unsigned RX_BUFSIZE = 16*1024;
extern const unsigned MAX_MESSAGES = 1024;

static int numProc = 64;
extern int numProc;
static int procRank = 0;
extern int procRank;

enum APMessage{
    APM_NONE,
    APM_BUFFERED,
};

//For reduce operation type
enum REDUCE_OP{
    SUM, MIN, MAX,
};

enum SendMode{
    SM_BUFFERED, SM_IMMEDIATE
};
#endif
