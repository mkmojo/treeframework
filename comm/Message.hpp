template<typename T> class Tree;

template<typename T> class Handler {
protected:
    T* pMessageData;
public:
    Handler(T* pMessageDataIn) : pMessageData(pMessageDataIn) { }
    virtual void handle(Tree<T>* pTree){
        //cout << "calling base handler" << endl;
        //cout << "  do nothing." << endl;
    }
};

template<typename T> class addToQueueHandler : public Handler<T> {
public:
    addToQueueHandler(T* pMessageDataIn) : Handler<T>(pMessageDataIn){ }
    virtual void handle(Tree<T>* pTree){
        //cout << "calling add to queue handler" << endl;
        pTree->private_data.push_back(*(Handler<T>::pMessageData));
    }
};

template<typename T> class sortQueueHandler : public Handler<T> {
public:
    sortQueueHandler(T* pMessageDataIn) : Handler<T>(pMessageDataIn){ }
    virtual void handle(Tree<T>* pTree){
        //cout << "calling sort queue handler" << endl;
        swap(pTree->private_data[0], pTree->private_data.back());
    }
};

template<typename T> class Message {
    T* pMessageData;
    Handler<T>* messagehandler;
public:
    Message(Handler<T>* handlerIn, T* pDataIn = nullptr):messagehandler(handlerIn),pMessageData(pDataIn) { }
    virtual void HandleMessage(Tree<T>* pTree){ messagehandler->handle(pTree); }
};

template<typename T> class addToQueue : public Message<T> {
public:
    addToQueue(T* pDataIn) : Message<T>(new addToQueueHandler<T>(pDataIn)){ }
    virtual void HandleMessage(Tree<T>* pTree){
        //cout << "handling add to queue message" << endl;
        Message<T>::HandleMessage(pTree);
    }
};

template<typename T> class sortQueue : public Message<T> {
public:
    sortQueue() : Message<T>(new sortQueueHandler<T>(nullptr)){ }
    virtual void HandleMessage(Tree<T>* pTree){
        //cout << "handling sort queue message" << endl;
        Message<T>::HandleMessage(pTree);
    }
};


