#pragma once
#include "MyWindow.h"
#include"LockFreeObjectPool.h"
#include "TlsObjectPool.h"
template <typename T>
class LockFreeQueueBasic
{
private:
    union Next
    {
        struct BitPartial
        {
            LONG64 pNode : 47;
            LONG64 id : 17;
        }bitPartial;
        LONG64 entire;
    };
    struct Node
    {
        T data;
        Next next;
    };
    union Edge
    {
        struct BitPartial
        {
            LONG64 pNode : 48;
            LONG64 cnt : 16;
        }bitPartial;
        LONG64 entire=0;
    };
    SHORT _id;
    alignas(64) Edge _head;
    alignas(64) Edge _tail;
    alignas(64) LONG _size = 0;
    inline static SHORT _newId = 0;
    static TlsObjectPool<Node,false> _objectPool;
public:
    LockFreeQueueBasic()
    {
        _id = InterlockedIncrement16(&_newId);
        Node* pNode = _objectPool.Alloc();
        (pNode->next).bitPartial.id = _id;
        (pNode->next).bitPartial.pNode = NULL;
        _head.entire = (LONG64)pNode;
        _tail.entire = _head.entire;
    }
    ~LockFreeQueueBasic()
    {
        T tmp;
        while (Dequeue(&tmp));
    }
    void Enqueue(const T& inPar)
    {
        Node* pNewNode = _objectPool.Alloc();
        pNewNode->data = inPar;
        (pNewNode->next).bitPartial.id = _id;
        (pNewNode->next).bitPartial.pNode = NULL;
        
        Edge oldTail;
        Edge newTail;

        Next oldTailNext;
        Next newNext;
        newNext.entire = (LONG64)pNewNode;
        newNext.bitPartial.id = _id;
        while (true)
        {
           
            oldTail = _tail;
            oldTailNext = ((Node*)oldTail.bitPartial.pNode)->next;
            if (oldTailNext.bitPartial.pNode == NULL)
            {
                newTail.entire = (LONG64)pNewNode;
                newTail.bitPartial.cnt = oldTail.bitPartial.cnt+1;
                //oldTail dequeue 되고 alloc되어 nullptr로 초기화되서 뒤에 붙는 경우가 존재
                if (InterlockedCompareExchange64(&((((Node*)oldTail.bitPartial.pNode)->next).entire), newNext.entire, oldTailNext.entire) == oldTailNext.entire)
                {
                    InterlockedIncrement(&_size);
                    InterlockedCompareExchange64(&_tail.entire, newTail.entire, oldTail.entire);
                    break;
                }
            }
            else
            {
                //2번째 CAS가 실패해서 TAIL의 NEXT가 NULLPTR이 아닐 때
                //tail을 항상 최신화 시켜주어야 한다.
                newTail.entire = oldTailNext.bitPartial.pNode;
                newTail.bitPartial.cnt = oldTail.bitPartial.cnt + 1;
                InterlockedCompareExchange64(&_tail.entire, newTail.entire, oldTail.entire);
            }
        }
    }

    bool Dequeue(T* outPar)
    {
        int size = InterlockedDecrement(&_size);
        if(size < 0)
        {
            InterlockedIncrement(&_size);
            return false;
        }

        Edge oldHead;
        Edge newHead;
        Edge oldTail;
        Edge newTail;

        Next oldHeadNext;
        Next oldTailNext;
        while (1)
        {
            oldHead = _head;
            oldHeadNext = ((Node*)oldHead.bitPartial.pNode)->next;
            oldTail = _tail;
            oldTailNext = ((Node*)oldTail.bitPartial.pNode)->next;
           
       
            // tail을 항상 최신화 시켜 head가 tail뒤로 못 넘어가게 한다 tail을 head보다 나중에 load해야 tail이 head에 대하여 항상 더 최신화되어 있을 수 있다
            if (oldTailNext.bitPartial.pNode != NULL)
            {
                newTail.entire = oldTailNext.bitPartial.pNode;
                newTail.bitPartial.cnt = oldTail.bitPartial.cnt + 1;
                InterlockedCompareExchange64(&_tail.entire, newTail.entire, oldTail.entire);
                continue;
            }

            if (oldHeadNext.bitPartial.pNode == NULL)
            {
                //case 1 oldHead가 다른 스레드에 의해 dequeue 되고 alloc되어 pNext가 초기화된 상황
                //case 2 뒤에 꼬리를 물어 enqueue 되어 size!=0이지만 아직 연결이 안된경우 [] []->[]->[]
                continue;
            }

            newHead.entire = oldHeadNext.bitPartial.pNode;
            newHead.bitPartial.cnt = oldHead.bitPartial.cnt+1;
                
            *outPar = ((Node*)oldHeadNext.bitPartial.pNode)->data;
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
TlsObjectPool<typename LockFreeQueueBasic<T>::Node, false> LockFreeQueueBasic<T>::_objectPool;