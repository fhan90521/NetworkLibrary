#pragma once
#include "OS/MyWindow.h"
#include "Memory/TlsObjectPool.h"
template <typename T>
class LockFreeQueue
{
private:
    struct Node
    {
        T data;
        Node* pNext=nullptr;
    };
    union Edge
    {
        struct BitPartial
        {
            LONG64 pNode : 47;
            LONG64 cnt : 17;
        }bitPartial;
        LONG64 entire=0;
    };
    alignas(64) Edge _head;        
    alignas(64) Edge _tail;
    alignas(64) LONG _size = 0;
    static TlsObjectPool<Node,true> _objectPool;
    
public:
    LockFreeQueue()
    {
        _head.entire = (LONG64)_objectPool.Alloc();
        _head.bitPartial.cnt = 0;
        _tail = _head;
    }
    ~LockFreeQueue()
    {
        T tmp;
        while (Dequeue(&tmp));
    }
    void Enqueue(const T& inPar)
    {
        Node* pNewNode = _objectPool.Alloc();
        pNewNode->data = inPar;
        Edge oldTail;
        Edge newTail;
        newTail.entire = (LONG64)pNewNode;
        while (true)
        {
            oldTail = _tail;
            Node* pOldTailNext = ((Node*)oldTail.bitPartial.pNode)->pNext;
            newTail.bitPartial.cnt= oldTail.bitPartial.cnt + 1;
            if (pOldTailNext == nullptr)
            {
                if (InterlockedCompareExchange64(&_tail.entire, newTail.entire, oldTail.entire) == oldTail.entire)
                {
                    ((Node*)oldTail.bitPartial.pNode)->pNext = pNewNode;
                    InterlockedIncrement(&_size);
                    break;
                }
            }
        }
    }

    bool Dequeue(T* outPar)
    {
        if (InterlockedDecrement(&_size) < 0)
        {
            InterlockedIncrement(&_size);
            return false;
        }
        Edge oldHead;
        Edge newHead;
        Edge curTail;
        while (1)
        {

            oldHead = _head;
            curTail = _tail;
            Node* pOldHeadpNext = ((Node*)oldHead.bitPartial.pNode)->pNext;
            if (pOldHeadpNext== nullptr)
            {
                //case 1 oldHead가 다른 스레드에 의해 dequeue 되고 alloc되어 pNext가 초기화된 상황
                //case 2 tail은 옮겨졌지만 pNext에 값이 아직 대입안되어 nullptr로 보이는 상황
                continue;
            }
            newHead.entire = (LONG64)pOldHeadpNext;
            newHead.bitPartial.cnt = oldHead.bitPartial.cnt+1;
            *outPar = pOldHeadpNext->data;
            if (InterlockedCompareExchange64(&_head.entire, newHead.entire, oldHead.entire) == oldHead.entire)
            {
                //이사이에서 newHead(pOldHeadpNext)가 dequeu enqueue되면서 pOldHeadpNext data가 사라진다.
                //outPar = pOldHeadpNext->data;
                _objectPool.Free((Node*)oldHead.bitPartial.pNode);
                break;
            }
        }
        return true;
    }
    LONG Size()
    {
        return _size;
    }
};
template<typename T>
TlsObjectPool<typename LockFreeQueue<T>::Node, true> LockFreeQueue<T>::_objectPool;