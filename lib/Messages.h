#ifndef MESSAGES_H
#define MESSAGES_H 1

#include "treedef.hpp"
#include <ostream>

using treedef::TreeNode;
using treedef::NodeFlag;

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
template<typename U, typename V>
class Message
{
    public:
        Message() { }
        Message(const TreeNode<U, V>& x) : m_node(x) { }
        virtual ~Message() { }

        virtual void handle(
                int senderID, LocalTree& handler) = 0;

        virtual size_t getNetworkSize() const
        {
            return sizeof (uint8_t); // MessageType
            //+ TreeNode<U, V>::serialSize();
        }

        static MessageType readMessageType(char* buffer);

        friend std::ostream& operator <<(std::ostream& out,
                const Message& message)
        {
            return out << message.m_seq.str() << '\n';
        }

        TreeNode<U, V> m_node;
};

/** Add a TreeNode. */
template <typename U, typename V>
class BinAddMessage : public Message<U, V>
{
    public:
        BinAddMessage() { }
        BinAddMessage(const TreeNode<U, V>& x) : Message<U, V>(x) { }

        void handle(int senderID, LocalTree& handler);

        static const MessageType TYPE = MT_ADD;
};

/** Remove a TreeNode. */
template <typename U, typename V>
class BinRemoveMessage : public Message<U,V>
{
    public:
        BinRemoveMessage() { }
        BinRemoveMessage(const TreeNode<U, V>& x) : Message<U,V>(x) { }

        void handle(int senderID, LocalTree& handler);

        static const MessageType TYPE = MT_REMOVE;
};

/** Set a flag. */
template <typename U, typename V>
class SetFlagMessage : public Message<U,V>
{
    public:
        SetFlagMessage() { }
        SetFlagMessage(const TreeNode<U, V>& x, NodeFlag flag)
            : Message<U,V>(x), m_flag(flag) { }

        size_t getNetworkSize() const
        {
            return Message<U, V>::getNetworkSize() + sizeof m_flag;
        }

        void handle(int senderID, LocalTree& handler);

        static const MessageType TYPE = MT_SET_FLAG;
        uint8_t m_flag; // SeqFlag
};


/** Request vertex properties. */
template <typename U, typename V>
class BinDataRequest : public Message<U,V>
{
    public:
        BinDataRequest() { }
        BinDataRequest(const TreeNode<U, V>& x, IDType group, IDType id)
            : Message<U,V>(x), m_group(group), m_id(id) { }

        size_t getNetworkSize() const
        {
            return Message<U, V>::getNetworkSize()
                + sizeof m_group + sizeof m_id;
        }

        void handle(int senderID, LocalTree& handler);

        static const MessageType TYPE = MT_BIN_DATA_REQUEST;
        IDType m_group;
        IDType m_id;
};

/** The response to a request for vertex properties. */
template < typename U, typename V>
class BinDataResponse : public Message<U,V>
{
    public:
        BinDataResponse() { }
        BinDataResponse(const TreeNode<U, V>& x, IDType group, IDType id,
                int multiplicity) :
            Message<U, V>(x), m_group(group), m_id(id),
            m_multiplicity(multiplicity) { }

        size_t getNetworkSize() const
        {
            return Message<U, V>::getNetworkSize()
                + sizeof m_group + sizeof m_id
                + sizeof m_multiplicity;
        }

        void handle(int senderID, LocalTree& handler);

        static const MessageType TYPE = MT_BIN_DATA_RESPONSE;
        IDType m_group;
        IDType m_id;
        uint16_t m_multiplicity;
};
#endif
