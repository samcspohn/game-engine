#pragma once

#include <vector>
#include <deque>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>
#include <mutex>
// #include "Components/Component.h"
#include "serialize.h"

using namespace std;

template <typename t>
class componentStorage;

template <typename t>
class fast_list
{
private:
	mutex m;

public:
	SER_HELPER()
	{
		ar &data &iterators;
	}
	vector<t> data;

	struct _itr
	{
		bool operator==(const int &rhs)
		{
			return index == rhs;
		}
		bool operator!=(const int &rhs)
		{
			return index != rhs;
		}
		bool operator==(const _itr &rhs)
		{
			return index == rhs.index;
		}
		bool operator!=(const _itr &rhs)
		{
			return index != rhs.index;
		}
		bool operator<(const _itr &rhs) const
		{
			return index < rhs.index;
		}
		t &operator*()
		{
			return this->fl->data.at(index);
		}

		//t operator->() {
		//	return data[ref];
		//}
		_itr(){};
		_itr(int i, fast_list *_fl)
		{
			index = i;
			fl = _fl;
		};
		_itr(const _itr &other)
		{
			index = other.index;
			fl = other.fl;
		};
		int index;
		fast_list *fl;
		unsigned int it;
		SER_HELPER()
		{
			ar &fl &index &it;
		}
	};
	vector<unique_ptr<_itr>> iterators;

	struct iterator
	{
	public:
		SER_HELPER()
		{
			ar &itr;
		}
		bool isNull()
		{
			return itr == nullptr;
		}
		bool operator==(const int &rhs)
		{
			return *itr == rhs;
		}
		bool operator!=(const int &rhs)
		{
			return *itr != rhs;
		}
		t *operator->() const
		{
			return &itr->fl->data.at(itr->index);
		}
		operator unsigned int()
		{
			return this->itr->index;
		}
		t &data()
		{
			return itr->fl->data[itr->index];
		}
		t &operator*()
		{
			return itr->fl->data[itr->index];
		}
		const t &operator*() const
		{
			return itr->fl->data[itr->index];
		}
		iterator(){};
		iterator(_itr *i)
		{
			itr = i;
		}
		//iterator(const _itr& other) {  };
		_itr *itr = nullptr;
		//int _Ptr;
	};

	iterator back()
	{
		return iterator(iterators.back().get());
	}
	iterator begin()
	{
		return iterator(iterators.front().get());
	}

	//////////// end iterator
	fast_list()
	{
		//iteratorMap[0] = new _itr(0, this);
	}

	int size()
	{
		return data.size();
	}
	t &operator[](int index)
	{
		if (index > data.size())
			cout << "out of bounds" << endl;
		// throw exception("out of bounds");
		return data[index];
	}
	t &front()
	{
		return data[0];
	}
	iterator push_back(const t &element)
	{
		m.lock();
		iterators.emplace_back(new _itr(data.size(), this));
		_itr* it = iterators.back().get();
		// auto it = make_unique<_itr>(data.size(), this);
		data.push_back(element);
		it->it = iterators.size() - 1;
		m.unlock();
		return iterator(it);
	}

	void erase(iterator &itr)
	{
		if (itr.itr->fl != this)
			throw;
		m.lock();
		int index = itr.itr->index;
		std::swap(data[index],data.back());
		iterators.back()->index = itr.itr->index;
		iterators.back()->it = itr.itr->it;
		std::swap(iterators[itr.itr->it],iterators.back()); //this pointer = back pointer
		iterators.pop_back();
		data.pop_back();
		// delete itr.itr;

		m.unlock();
		//m.unlock();
	}
	void clear()
	{
		// for (typename vector<_itr *>::iterator i = iterators.begin(); i != iterators.end(); i++)
		// 	delete *i;
		iterators.clear();
		data.clear();
	}

	void swap(unsigned int l, unsigned int r)
	{
		_itr *itl = iterators[l];
		_itr *itr = iterators[r];
		itl->it = r;
		itr->it = l;
		iterators[l] = itr;
		iterators[r] = itl;
		unsigned int temp = itl->index;
		itl->index = itr->index;
		itr->index = temp;
		t tempt = std::move(data[l]);
		data[l] = std::move(data[r]);
		data[r] = std::move(tempt);
	}

	// ~fast_list()
	// {
	// 	for (typename vector<_itr *>::iterator i = iterators.begin(); i != iterators.end(); i++)
	// 		delete *i;
	// }
};

template <typename t>
class fast_list_deque
{
private:
	mutex m;

public:
	deque<t> data = deque<t>();

	struct _itr
	{
	public:
		bool operator==(const int &rhs)
		{
			return index == rhs;
		}
		bool operator!=(const int &rhs)
		{
			return index != rhs;
		}
		bool operator==(const _itr &rhs)
		{
			return index == rhs.index;
		}
		bool operator!=(const _itr &rhs)
		{
			return index != rhs.index;
		}
		bool operator<(const _itr &rhs) const
		{
			return index < rhs.index;
		}
		t &operator*()
		{
			return this->fl->data.at(index);
		}

		//t operator->() {
		//	return data[ref];
		//}
		_itr(){};
		_itr(int i, fast_list_deque *_fl)
		{
			index = i;
			fl = _fl;
		};
		_itr(const _itr &other)
		{
			index = other.index;
			fl = other.fl;
		};
		int index;
		fast_list_deque *fl;
		unsigned int it;
	};
	deque<_itr *> iterators;
	struct iterator
	{
	public:
		bool isNull()
		{
			return itr == nullptr;
		}
		bool operator==(const int &rhs)
		{
			return *itr == rhs;
		}
		bool operator!=(const int &rhs)
		{
			return *itr != rhs;
		}
		t *operator->() const
		{
			return &itr->fl->data.at(itr->index);
		}
		t &data()
		{
			return itr->fl->data[itr->index];
		}
		t &operator*()
		{
			return itr->fl->data[itr->index];
		}
		iterator(){};
		iterator(_itr *i)
		{
			itr = i;
		}
		void erase()
		{
			itr->fl->erase(*this);
		}
		_itr *itr = nullptr;
	};

	iterator back()
	{
		return iterator(iterators.back());
	}
	iterator begin()
	{
		return iterator(iterators.front());
	}

	//////////// end iterator
	fast_list_deque() : m()
	{
		//iteratorMap[0] = new _itr(0, this);
	}

	int size()
	{
		return data.size();
	}
	t &operator[](int index)
	{
		if (index > data.size())
			cout << "out of bounds" << endl;
		// throw exception("out of bounds");
		return data[index];
	}
	t &front()
	{
		return data[0];
	}
	iterator push_back(const t &element)
	{
		m.lock();
		_itr *it = new _itr(data.size(), this);
		data.push_back(element);
		it->it = iterators.size();
		iterators.push_back(it);
		m.unlock();
		return iterator(it);
	}

	void erase(iterator &itr)
	{
		if (itr.itr->fl != this)
			throw;
		m.lock();
		int index = itr.itr->index;
		data[index] = std::move(data.back());
		iterators.back()->index = index;		   //itr->index
		iterators.back()->it = itr.itr->it;		   // int
		iterators[itr.itr->it] = iterators.back(); //this pointer = back pointer
		iterators.erase(--(iterators.end()));

		//        swap(itr.itr->index, data.size() - 1);
		data.pop_back();
		delete itr.itr;

		m.unlock();
		//m.unlock();
	}
	void clear()
	{
		m.lock();
		for (typename deque<_itr *>::iterator i = iterators.begin(); i != iterators.end(); i++)
			delete *i;
		iterators.clear();
		data.clear();
		m.unlock();
	}

	void swap(unsigned int l, unsigned int r)
	{
		_itr *itl = iterators[l];
		_itr *itr = iterators[r];
		itl->it = r;
		itr->it = l;
		iterators[l] = itr;
		iterators[r] = itl;
		unsigned int temp = itl->index;
		itl->index = itr->index;
		itr->index = temp;
		t tempt = std::move(data[l]);
		data[l] = std::move(data[r]);
		data[r] = std::move(tempt);
	}

	~fast_list_deque()
	{
		for (typename deque<_itr *>::iterator i = iterators.begin(); i != iterators.end(); i++)
			delete *i;
	}
	//	friend class componentStorage;
	//    friend void sort();
};
