#ifndef MESSAGES_H
#define MESSAGES_H 1

#include "Treedef.h"

class Message
{
    public:
        Message(){}
        Message(const Point& point) : m_point(point){}
        static MessageType readMessageType(char* buffer);
        virtual void handle(int senderID, LocalOctTree& handler) = 0;
        virtual size_t unserialize(const char* buffer);
        virtual size_t serialize(char* buffer) = 0;

        Node m_node;
};
#endif
