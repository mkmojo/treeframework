// AP means across process
enum APMessage
{
	APM_NONE,
	APM_CONTROL,
	APM_BUFFED
};

enum APControl
{
	APC_SET_STATE,
	APC_ERODE_COMPLETE,
	APC_
};

struct ControlMessage
{
	int64_t id;
	APControl msgType;
	int argument;
}

class CommLayer
{
	public:
		CommLayer();
		~CommLayer();

		APMessage checkMessage(int &sendID);

		bool receiveEmpty();

		void barrier();

		void broadcast(int message);
		int receiveBroadcast();
		long long unsigned reduce(long long unsigned count);
		std::vector<unsigned> reduce(const std::vector<unsigned>& v);
		std::vector<unsigned> reduce(const std::vector<unsigned>& v);
		std::vector<long unsigned> reduce( 
				const std::vector<long unsigned>& v);

		void sendControlMessage(APControl m, int argument = 0);
		void sendControlMessageToNode(int nodeID, APControl m, int argument = 0);
		ControlMessage receiveControlMessage();
		uint64_t sendCheckPointMessage(int argument = 0);
		void sendBufferedMessage(int destID, char* msg, size_t size);
		void receiveBufferedMessage(MessagePtrVector& outmessages);
		uint64_t reduceInflight()
		{
			return reduce(m_txPackets - m_rxPackets);
		}
	private:
		uint64_t m_msgID;
		uint8_t* m_rxBuffer;
		MPI_Request m_request;

	protected:
		uint64_t m_rxPackets;
		uint64_t m_rxMessages;
		uint64_t m_rxBytes;
		uint64_t m_rxPackets;
		uint64_t m_txMessages;
		uint64_t m_txBytes;
}
