#ifndef MESSAGES_H
#define MESSAGES_H 1

#include "Point.h"

enum MessageType
{
    MT_VOID,
    MT_ADD
};

class Message
{
    public:
        Message(){}
        Message(const Point& p) : m_point(p){}
        static MessageType readMessageType(char* buffer);
        virtual void handle(int senderID, LocalOctTree& handler) = 0;

        virtual size_t getNetworkSize() const
        {
            return sizeof (unit8_t)
                + Point::serialSize();
        }
        static MessageType readMessageType(char* buffer);
        virtual size_t unserialize(const char* buffer);
        virtual size_t serialize(char* buffer) = 0;

        Point m_point;
};

/** Add a Point. */
class SeqAddMessage : public Message
{
    public:
        SeqAddMessage() { }
        SeqAddMessage(const Point& p) : Message(p) { }

        void handle(int senderID, LocalOctTree& handler);
        size_t serialize(char* buffer);

        static const MessageType TYPE = MT_ADD;
};


#endif
