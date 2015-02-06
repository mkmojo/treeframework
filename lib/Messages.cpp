#include "Messages.h"
#include "LocalOctTree.h"

void SeqAddMessage::handle(int senderID,
        LocalOctTree& handler)
{
    handler.handle(senderID, *this);
}

size_t SeqAddMessage::serialize(char* buffer)
{
    size_t offset = 0;
    buffer[offset++] = TYPE;
    offset += m_point.serialize(buffer + offset);
    return offset;
}

size_t SeqAddMessage::unserialize(char* buffer)
{
    size_t offset = 0;
    offset++; //Message Type
    offset += m_point.unserialize(buffer + offset);
    return offset;
}
