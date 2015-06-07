template<typename T> class Messager;

template<typename T> class Handler {
protected:
    T* pMessageData;
public:
    Handler(T* pMessageDataIn) : pMessageData(pMessageDataIn) { }
    virtual void handle(Messager<T>* pMessager){
        //cout << "calling base handler" << endl;
        //cout << "  do nothing." << endl;
    }
};

template<typename T> class addToQueueHandler : public Handler<T> {
public:
    addToQueueHandler(T* pMessageDataIn) : Handler<T>(pMessageDataIn){ }
    virtual void handle(Messager<T>* pMessager){
        //cout << "calling add to queue handler" << endl;
        pMessager->private_data.push_back(*(Handler<T>::pMessageData));
    }
};

template<typename T> class sortQueueHandler : public Handler<T> {
public:
    sortQueueHandler(T* pMessageDataIn) : Handler<T>(pMessageDataIn){ }
    virtual void handle(Messager<T>* pMessager){
        //cout << "calling sort queue handler" << endl;
        swap(pMessager->private_data[0], pMessager->private_data.back());
    }
};

template<typename T> class Message {
    T* pMessageData;
    Handler<T>* messagehandler;
public:
    Message(Handler<T>* handlerIn, T* pDataIn = nullptr):messagehandler(handlerIn),pMessageData(pDataIn) { }
    virtual void HandleMessage(int id, Messager<T>* pMessager){ messagehandler->handle(pMessager); }
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

template<typename T> class addToQueue : public Message<T> {
public:
    addToQueue(T* pDataIn) : Message<T>(new addToQueueHandler<T>(pDataIn)){ }
    virtual void HandleMessage(int id, Messager<T>* pMessager){
        //cout << "handling add to queue message" << endl;
        Message<T>::HandleMessage(id, pMessager);
    }
};

template<typename T> class sortQueue : public Message<T> {
public:
    sortQueue() : Message<T>(new sortQueueHandler<T>(nullptr)){ }
    virtual void HandleMessage(int id, Messager<T>* pMessager){
        //cout << "handling sort queue message" << endl;
        Message<T>::HandleMessage(id, pMessager);
    }
};


