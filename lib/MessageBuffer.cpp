#include "MessageBuffer.h"
#include "Common/Options.h"
#include <iostream>
#include "treedef.hpp"

using namespace std;
using treedef::TreeNode;
using treedef::NodeFlag;

//allocate space for message queues
template <typename U, typename V>
MessageBuffer<U, V>::MessageBuffer()
	: m_msgQueues(opt::numProc)
{
	for (unsigned i = 0; i < m_msgQueues.size(); i++)
		m_msgQueues[i].reserve(MAX_MESSAGES);
}

template <typename U, typename V> 
void MessageBuffer<U, V>::sendBinAddMessage(int nodeID, const TreeNode<U, V>& x)
{
	queueMessage(nodeID, new BinAddMessage(x), SM_BUFFERED);
}

template <typename U, typename V>
void MessageBuffer<U, V>::sendBinRemoveMessage(int nodeID, const TreeNode<U, V>& x)
{
	queueMessage(nodeID, new BinRemoveMessage(x), SM_BUFFERED);
}

// Send a set flag message
template <typename U, typename V>
void MessageBuffer<U,V>::sendSetFlagMessage(int nodeID,
		const TreeNode<U, V>& x, NodeFlag flag)
{
	queueMessage(nodeID, new SetFlagMessage(x, flag), SM_BUFFERED);
}

// Send a binary data request
template <typename U, typename V>
void MessageBuffer<U, V>::sendBinDataRequest(int nodeID,
		IDType group, IDType id, const TreeNode<U, V>& x)
{
	queueMessage(nodeID,
			new BinDataRequest(x, group, id), SM_IMMEDIATE);
}

// Send a binary data response
template <typename U, typename V>
void MessageBuffer<U, V>::sendBinDataResponse(int nodeID,
		IDType group, IDType id, const TreeNode<U, V>& x,
		 int multiplicity)
{
	queueMessage(nodeID,
			new BinDataResponse(x, group, id, extRec, multiplicity),
			SM_IMMEDIATE);
}

template <typename U, typename V>
void MessageBuffer<U, V>::queueMessage(
		int nodeID, Message* message, SendMode mode)
{
	if (opt::verbose >= 9)
		cout << opt::rank << " to " << nodeID << ": " << *message;
	m_msgQueues[nodeID].push_back(message);
	checkQueueForSend(nodeID, mode);
}

template <typename U, typename V>
void MessageBuffer<U, V>::checkQueueForSend(int nodeID, SendMode mode)
{
	size_t numMsgs = m_msgQueues[nodeID].size();
	// check if the messages should be sent
	if ((numMsgs == MAX_MESSAGES || mode == SM_IMMEDIATE)
			&& numMsgs > 0) {
		// Calculate the total size of the message
		size_t totalSize = 0;
		for(size_t i = 0; i < numMsgs; i++)
		{
			totalSize += m_msgQueues[nodeID][i]->getNetworkSize();
		}

		// Generate a buffer for all the messages
		char* buffer = new char[totalSize];

		// Copy the messages into the buffer
		size_t offset = 0;
		for(size_t i = 0; i < numMsgs; i++)
			offset += m_msgQueues[nodeID][i]->serialize(
					buffer + offset);

		assert(offset == totalSize);
		sendBufferedMessage(nodeID, buffer, totalSize);

		delete [] buffer;
		clearQueue(nodeID);

		m_txPackets++;
		m_txMessages += numMsgs;
		m_txBytes += totalSize;
	}
}

// Clear a queue of messages
template <typename U, typename V>
void MessageBuffer<U,V>::clearQueue(int nodeID)
{
	size_t numMsgs = m_msgQueues[nodeID].size();
	for(size_t i = 0; i < numMsgs; i++)
	{
		// Delete the messages
		delete m_msgQueues[nodeID][i];
		m_msgQueues[nodeID][i] = 0;
	}
	m_msgQueues[nodeID].clear();
}

// Flush the message buffer by sending all messages that are queued
template <typename U, typename V>
void MessageBuffer<U, V>::flush()
{
	// Send all messages in all queues
	for(size_t id = 0; id < m_msgQueues.size(); ++id)
	{
		// force the queue to send any pending messages
		checkQueueForSend(id, SM_IMMEDIATE);
	}
}

// Check if all the queues are empty
template <typename U, typename V>
bool MessageBuffer<U, V>::transmitBufferEmpty() const
{
	bool isEmpty = true;
	for (MessageQueues::const_iterator it = m_msgQueues.begin();
			it != m_msgQueues.end(); ++it) {
		if (!it->empty()) {
			cerr
				<< opt::rank << ": error: tx buffer should be empty: "
				<< it->size() << " messages from "
				<< opt::rank << " to " << it - m_msgQueues.begin()
				<< '\n';
			for (MsgBuffer::const_iterator j = it->begin();
					j != it->end(); ++j)
				cerr << **j << '\n';
			isEmpty = false;
		}
	}
	return isEmpty;
}
