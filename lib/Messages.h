#ifndef MESSAGES_H
#define MESSAGES_H 1

#include "Point.h"
#include "Cell.h"
#include <stdint.h> //for unit8_t

//avoid circular header dependency
class LocalOctTree;

enum MessageType
{
    MT_VOID,
    MT_ADD
};

class Message
{
    public:
        Message(){}
        virtual ~Message(){}
        Message(const Point& p) : m_point(p){}
        static MessageType readMessageType(char* buffer);
        virtual void handle(int senderID, LocalOctTree& handler) = 0; 
        virtual size_t getNetworkSize() const
        {
            return sizeof (uint8_t)
                + Point::serialSize();
        }
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
