#pragma once
#include <deque>
#include <set>
#include <deque>
#include <array>
#include <vector>
using namespace std;

//#define ARRAY_HEAP_POINTERS
#ifndef ARRAY_HEAP_POINTERS


template<typename t>
class array_heap {
public:
	struct ref {
		int index;
		array_heap<t>* a;
		t* operator->() {
			return &(a->data.at(index));
		}
		 operator int() {
			return index;
		}
		t& operator*() {
			return (*a->data)[index];
		}
		t& data() {
			return a->data.at(index);
		}
	};
	friend array_heap::ref;
	t& operator[](unsigned int i) {
		if (i >= extent)
			throw -10;//"out of bounds";
		//if(!valid[i])
		//	throw exception("invalid");
		return data.at(i);
	}
	ref _new() {
		ref ret;
		ret.a = this;
		if (avail.size() > 0) {
			ret.index = *avail.begin();
			avail.erase(avail.begin());;
			valid[ret.index] = true;
			if (ret.index >= extent) {
				extent = ret.index + 1;
			}
		}
		else {
			ret.index = data.size();
			data.push_back(t());
			valid.push_back(true);
			++extent;
		}
		return ret;
	}
	void _delete(ref r) {
		if (r.a != this)
			throw;
		avail.emplace(r.index);
		valid[r.index] = false;
		//(*data)[r.index] = t();
		if (r.index == extent - 1) {
			for (; extent > 0 && !valid[extent - 1]; --extent) {
				avail.erase(extent - 1);
			}
			//++extant;
			data.resize(extent);
			valid.resize(extent);
		}
	}

	float density() {
		if (extent == 0)
			return INFINITY;
		return 1 - (float)avail.size() / (float)extent;
	}

	unsigned int size() {
		return extent;
	}
	deque<t> data = deque<t>();
private:
	set<uint32_t> avail = set<uint32_t> ();
	uint32_t extent = 0;
	std::vector<bool> valid = std::vector<bool>();
};

#else


#define _DEQUE_BLOCK_SIZE 1024
template<typename t>
class _deque {
public:
	deque<vector<t> > data = deque<vector<t> >();
	size_t _size = 0;
	_deque() {
		data.push_back(vector<t>());
		data.back().reserve(_DEQUE_BLOCK_SIZE);
	}
	inline void push_back(const t& a) {
		if (_size == data.size() * _DEQUE_BLOCK_SIZE) {
			data.push_back(vector<t>());
			data.back().reserve(_DEQUE_BLOCK_SIZE);
		}
		data.back().push_back(a);
		++_size;
	}
	inline size_t size() {
		return _size;
	}
	inline t& operator[](size_t i) {
		return data[i / _DEQUE_BLOCK_SIZE][i % _DEQUE_BLOCK_SIZE];
	}
	inline t& at(size_t i) {
		return (t&)data.at(i / _DEQUE_BLOCK_SIZE).at(i % _DEQUE_BLOCK_SIZE);
	}
	inline void resize(size_t size) {
		data.resize(size / _DEQUE_BLOCK_SIZE + 1);
		data.back().reserve(_DEQUE_BLOCK_SIZE);
		data.back().resize(size % _DEQUE_BLOCK_SIZE);
		//size_t m = BLOCK_SIZE - size % BLOCK_SIZE;
		//for(int i = 0;)
		_size = size;
	}
};

template<typename t>
class array_heap {
public:
	struct ref {
		int index;
		array_heap<t>* a;
		t* p;
		t* operator->() {
			return p;// &(a->data.at(index));
		}
		operator int() {
			return index;
		}
		t& operator*() {
			return *p;// (*a->data)[index];
		}
		t& data() {
			return *p;// a->data.at(index);
		}
	};
	friend array_heap::ref;
	t& operator[](unsigned int i) {
		if (i >= extent)
			cout << "out of bounds" << endl;
			// throw exception("out of bounds");
		//if(!valid[i])
		//	throw exception("invalid");
		return data.at(i);
	}
	ref _new() {

		ref ret;
		ret.a = this;
		if (avail.size() > 0) {
			ret.index = *avail.begin();
			avail.erase(avail.begin());;
			valid[ret.index] = true;
			if (ret.index >= extent) {
				extent = ret.index + 1;
			}
		}
		else {
			ret.index = data.size();
			data.push_back(t());
			valid.push_back(true);
			++extent;
		}
		ret.p = (t*)(&(data.at(ret.index)));

		return ret;
	}
	void _delete(ref r) {

		if (r.a != this)
			throw;
		//data[r.index].~t();
		avail.emplace(r.index);
		valid[r.index] = false;
		//(*data)[r.index] = t();
		if (r.index == extent - 1) {
			for (; extent > 0 && !valid[extent - 1]; --extent) {
				avail.erase(extent - 1);
			}
			//++extant;
			data.resize(extent);
			valid.resize(extent);
		}
	}

	float density() {
		if (extent == 0)
			return INFINITY;
		return 1 - (float)avail.size() / (float)extent;
	}

	unsigned int size() {
		return extent;
	}
	_deque<t> data = _deque<t>();
private:
	set<uint32_t> avail = set<uint32_t>();
	uint32_t extent;
	std::vector<bool> valid = vector<bool>();
};
#endif // ARRAY_HEAP_POINTERS
