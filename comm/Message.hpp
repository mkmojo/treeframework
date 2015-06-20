template<typename T> class Messager;

template<typename T> class Message {
public:
    T Data; //accessible through the public interface

    Message(){}
    Message(const T& DataIn) : Data(DataIn){}

    //allow copy construction
    Message(const Message<T>& rhs){
        Data = rhs.getData();
    }

    size_t getNetworkSize() const{
        return sizeof(uint8_t) + sizeof(T);
    }

    size_t serialize(char* buffer){
        size_t offset = 0;
        offset += Data.serialize(buffer + offset);
        return offset;
    }
    
    size_t unserialize(const char* buffer){
        size_t offset = 0;
        offset += Data.unserialize(buffer + offset);
        return offset;
    }
};
