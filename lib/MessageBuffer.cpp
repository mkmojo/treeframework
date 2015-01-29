#include "MessageBuffer.h"

void MessageBuffer::MessageBuffer()
    :m_msgQueues(opt::numProc)
{
    for(int i=0;i<m_msgQueues.size();i++){
        m_msgQueues[i].reserve(MAX_MESSAGES);
    }
}
