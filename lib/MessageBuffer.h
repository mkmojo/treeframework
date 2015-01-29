#ifndef MESSAGE_BUFFER_H
#define MESSAGE_BUFFER_H 1

class MessageBuffer;

#include "CommLayer.h"
#include "Messages.h"
#include <vector>

typedef std::vector<Message*> MsgBuffer;
typedef std::vector<MsgBuffer> MessageQueues;

enum sendMode
{
    SM_BUFFERED,
    SM_IMMEDIATE
};

class MessageBuffer : CommLayer
{
    public:
        
    private:
        static const size_t MAX_MESSAGES=100;
        MessageQueues m_msgQueues;
};

#endif
