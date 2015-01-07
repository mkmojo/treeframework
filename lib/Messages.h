#ifndef MESSAGES_H
#define MESSAGES_H 1

#include "Treedef.h"
#include <ostream>

class LocalTree;

enum MessageType
{
    MT_VOID,
    MT_ADD,
    MT_REMOVE,
    MT_SET_FLAG,
    MT_BIN_DATA_REQUEST,
    MT_BIN_DATA_RESPONSE,
};

enum MessageOp
{
    MO_VOID,
    MO_ADD,
    MO_REMOVE,
};

typedef uint32_t IDType;

/** The base class of all interprocess messages. */
class Message
{
    public:
        Message() { }
        Message(const TreeNode& x) : m_node(x) { }
        virtual ~Message() { }

        virtual void handle(
                int senderID, LocalTree& handler) = 0;

        virtual size_t getNetworkSize() const
        {
            return sizeof (uint8_t) // MessageType
                + TreeNode::serialSize();
        }

        static MessageType readMessageType(char* buffer);
        virtual size_t serialize(char* buffer) = 0;
        virtual size_t unserialize(const char* buffer);

        friend std::ostream& operator <<(std::ostream& out,
                const Message& message)
        {
            return out << message.m_seq.str() << '\n';
        }

        TreeNode m_node;
};

/** Add a TreeNode. */
class SeqAddMessage : public Message
{
    public:
        SeqAddMessage() { }
        SeqAddMessage(const TreeNode& x) : Message(x) { }

        void handle(int senderID, LocalTree& handler);
        size_t serialize(char* buffer);

        static const MessageType TYPE = MT_ADD;
};

/** Remove a Kmer. */
class SeqRemoveMessage : public Message
{
    public:
        SeqRemoveMessage() { }
        SeqRemoveMessage(const TreeNode& x) : Message(x) { }

        void handle(int senderID, LocalTree& handler);
        size_t serialize(char* buffer);

        static const MessageType TYPE = MT_REMOVE;
};

/** Set a flag. */
class SetFlagMessage : public Message
{
    public:
        SetFlagMessage() { }
        SetFlagMessage(const TreeNode& x, NodeFlag flag)
            : Message(seq), m_flag(flag) { }

        size_t getNetworkSize() const
        {
            return Message::getNetworkSize() + sizeof m_flag;
        }

        void handle(int senderID, LocalTree& handler);
        size_t serialize(char* buffer);
        size_t unserialize(const char* buffer);

        static const MessageType TYPE = MT_SET_FLAG;
        uint8_t m_flag; // SeqFlag
};


/** Request vertex properties. */
class SeqDataRequest : public Message
{
    public:
        SeqDataRequest() { }
        SeqDataRequest(const TreeNode& x, IDType group, IDType id)
            : Message(seq), m_group(group), m_id(id) { }

        size_t getNetworkSize() const
        {
            return Message::getNetworkSize()
                + sizeof m_group + sizeof m_id;
        }

        void handle(int senderID, LocalTree& handler);
        size_t serialize(char* buffer);
        size_t unserialize(const char* buffer);

        static const MessageType TYPE = MT_BIN_DATA_REQUEST;
        IDType m_group;
        IDType m_id;
};

/** The response to a request for vertex properties. */
class SeqDataResponse : public Message
{
    public:
        SeqDataResponse() { }
        SeqDataResponse(const TreeNode& x, IDType group, IDType id,
                ExtensionRecord& extRecord, int multiplicity) :
            Message(seq), m_group(group), m_id(id),
            m_extRecord(extRecord), m_multiplicity(multiplicity) { }

        size_t getNetworkSize() const
        {
            return Message::getNetworkSize()
                + sizeof m_group + sizeof m_id
                + sizeof m_extRecord + sizeof m_multiplicity;
        }

        void handle(int senderID, LocalTree& handler);
        size_t serialize(char* buffer);
        size_t unserialize(const char* buffer);

        static const MessageType TYPE = MT_BIN_DATA_RESPONSE;
        IDType m_group;
        IDType m_id;
        ExtensionRecord m_extRecord;
        uint16_t m_multiplicity;
};

#endif
