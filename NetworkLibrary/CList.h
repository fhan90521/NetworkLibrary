#pragma once
template <typename T>
class CList
{
public:
	struct Node
	{
		T _data;
		Node* _Prev=nullptr;
		Node* _Next=nullptr;
	};

	class iterator
	{
	private:
		friend CList;
		Node* _node;
	public:
		iterator(Node* node = nullptr)
		{
			_node = node;
		}
		iterator(const iterator& other)
		{
			_node = other._node;
		}
		iterator& operator=(const iterator& other)
		{
			_node = other._node;
			return *this;
		}

		const iterator operator ++(int)
		{
			iterator ret(_node);
			_node = _node->_Next;
			return ret;
		}

		iterator& operator++()
		{
			_node = _node->_Next;
			return *this;
		}

		const iterator operator --(int)
		{
			iterator ret(_node);
			_node = _node->_Prev;
			return ret;
		}

		iterator& operator--()
		{
			_node = _node->_Prev;
			return *this;
		}

		T& operator *()
		{
			return _node->_data;
		}

		bool operator ==(const iterator& other)
		{
			return _node == other._node;
		}

		bool operator !=(const iterator& other)
		{
			return _node != other._node;
		}
	};

public:
	CList()
	{
		_head._Next = &_tail;
		_tail._Prev = &_head;
	}
	~CList()
	{
		clear();
	}

	iterator begin()
	{
		return iterator(_head._Next);
	}
	iterator end()
	{
		return iterator(&_tail);
	}

	void push_front(const T& data)
	{
		Node* newNode = new Node();
		newNode->_data = data;
		
		Node* prevNew = _head._Next;
		prevNew->_Prev = newNode;
		newNode->_Next = prevNew;
		
		_head._Next = newNode;
		newNode->_Prev = &_head;

		_size++;
	}
	void push_front(T&& data)
	{
		Node* newNode = new Node();
		newNode->_data = data;

		Node* prevNew = _head._Next;
		prevNew->_Prev = newNode;
		newNode->_Next = prevNew;

		_head._Next = newNode;
		newNode->_Prev = &_head;

		_size++;
	}

	void push_back(const T& data)
	{
		Node* newNode = new Node();
		newNode->_data = data;

		Node* prevNew = _tail._Prev;
		prevNew->_Next = newNode;
		newNode->_Prev = prevNew;

		_tail._Prev = newNode;
		newNode->_Next = &_tail;

		_size++;
	}
	void push_back(T&& data)
	{
		Node* newNode = new Node();
		newNode->_data = data;

		Node* prevNew = _tail._Prev;
		prevNew->_Next = newNode;
		newNode->_Prev = prevNew;

		_tail._Prev = newNode;
		newNode->_Next = &_tail;

		_size++;
	}
	bool pop_front()
	{
		if (_size == 0)
		{
			return false;
		}
		_size--;

		Node* FirstNode = _head._Next;
		_head._Next= FirstNode->_Next;
		(FirstNode->_Next)->_Prev = &_head;

		delete FirstNode;

		return true;
	}
	bool pop_back()
	{
		if (_size == 0)
		{
			return false;
		}
		_size--;

		Node* FirstNode = _head._Next;
		_head._Next = FirstNode->_Next;
		(FirstNode->_Next)->_Prev = &_head;

		delete FirstNode;

		return true;
	}
	void clear()
	{
		for (iterator it=this->begin();it!=this->end();)
		{
			it=erase(it);
		}
	}
	int size() { return _size; }
	bool empty() { return _size == 0; }



	iterator erase(iterator iter)
	{
		Node* cur = iter._node;
		Node* prev = cur->_Prev;
		Node* next = cur->_Next;
		prev->_Next = next;
		next->_Prev = prev;
		delete cur;

		_size--;
		return iterator(next);
	}
		

	void remove(T Data)
	{
		CList<T>::iterator iter;
		for (iter = this->begin(); iter != this->end(); )
		{
			if (*iter == Data)
			{
				iter = erase(iter);
			}
			else
			{
				iter++;
			}
		}
	}

private:
	int _size = 0;
	Node _head;
	Node _tail;
};

















