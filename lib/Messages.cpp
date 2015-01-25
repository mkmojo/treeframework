#include "Messages.h"
#include "treedef.hpp"
#include <cstring>

MessageType Message::readMessageType(char* buffer)
{
    return (MessageType)*(uint8_t*)buffer;
}

void BinAddMessage::handle(int senderID,
        LocalTree& handler)
{
    handler.handle(senderID, *this);
}

void BinRemoveMessage::handle(int senderID,
        LocalTree& handler)
{
    handler.handle(senderID, *this);
}

void SetFlagMessage::handle(
        int senderID, LocalTree& handler)
{
    handler.handle(senderID, *this);
}


void RemoveExtensionMessage::handle(
        int senderID, LocalTree& handler)
{
    handler.handle(senderID, *this);
}

void SetBaseMessage::handle(
        int senderID, LocalTree& handler)
{
    handler.handle(senderID, *this);
}

void SeqDataRequest::handle(
        int senderID, LocalTree& handler)
{
    handler.handle(senderID, *this);
}

void SeqDataResponse::handle(
        int senderID, LocalTree& handler)
{
    handler.handle(senderID, *this);
}
