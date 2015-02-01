#ifndef TREEDEF_H
#define TREEDEF_H 1

class LocalOctTree
{
    void runControl();
    void run();
}

void LocalOctTree::runControl()
{
    SetStata(NAS_LOADING);
    while(m_state != NAS_DONE){
        swicth(m_stat){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();

                    m_numReachedCheckpoint++;
                    while(!checkpointReached())
                        pumpNetwork();

                    SetState(NAS_LOAD_COMPLETE);
                    m_comm.sendControlMessage(APC_SET_STATE,
                            NAS_LOAD_COMPLETE);
                    
                    m_comm.barrier();
                    pumpNetwork();
                    
                    SetState(NAS_DONE);
                    break;
                }
        }
    }
}

void LocalOctTree::run()
{
    SetState(NAS_LOADING);
    while(m_state != NAS_DONE){
        switch (m_state){
            case NAS_LOADING:
                {
                    loadPoints();
                    EndState();
                    SetState();
                    m_comm.sendCheckPointMessage();
                    break;
                }
            case NAS_LOAD_COMPLETE:
                {
                    m_comm.barrier();
                    pumpNetwork();
                    SetState(NAS_WAITING);
                }
            case NAS_WAITING:
                pumpNetwork();
                break;
            case NAS_DONE:
                break;
        }
    }
}



#endif
