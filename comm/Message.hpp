#ifndef MESSAGE_HPP
#define MESSAGE_HPP

enum MessageType{
    MT_POINT;
    MT_NODE;
}

class Message {
    public:
        virtual size_t getNetworkSize() const = 0;

        virtual size_t serialize(char* buffer) = 0;

        virtual size_t unserialize(const char* buffer) = 0;
};
#endif
