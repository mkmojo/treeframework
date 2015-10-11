#ifndef MESSAGE_HPP
#define MESSAGE_HPP

class Message {
    public:
        virtual size_t getNetworkSize() const = 0;

        virtual size_t serialize(char* buffer) = 0;

        virtual size_t unserialize(const char* buffer) = 0;
};
#endif
