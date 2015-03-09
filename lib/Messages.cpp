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

MessageType Message::readMessageType(char* buffer)
{
    return (MessageType)*(uint8_t*)buffer;
}

size_t Message::unserialize(const char* buffer)
{
    size_t offset = 0;
    offset++; //Message Type
    offset += m_point.unserialize(buffer + offset);
    return offset;
}
