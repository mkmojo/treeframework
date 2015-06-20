template<typename T> class Messager;

template<typename T> class Message {
    T MessageData;
public:
    Message(T* pDataIn){ MessageData = *pDataIn; }

    virtual size_t getNetworkSize() const{
        return sizeof(uint8_t) + sizeof(T);
    }

    virtual size_t serialize(char* buffer){
        size_t offset = 0;
        buffer[offset++] = TYPE;
        offset += pMessageData->serialize(buffer + offset);
        return offset;
    }
    
    virtual size_t unserialize(const char* buffer){
        size_t offset = 0;
        offset++; //Message Type
        offset += pMessageData->unserialize(buffer + offset);
        return offset;
    }
};
