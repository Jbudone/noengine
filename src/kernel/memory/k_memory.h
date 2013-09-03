#ifndef __K_MEMORY_H__
#define __K_MEMORY_H__



#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <boost/format.hpp>

using namespace std;

using boost::format;
using boost::str;
/*
 * Memory
 *
 * TODO
 *
 *  > implement various storage types (pool, double-sided allocator, one-frame allocator)
 *  > pointer-fixup table (GUID)
 *
 ***/

template<class T>
struct SwapBuffer {
	T **active   = 0;
	T **inactive = 0;
	T **swap() {
		if (onActive) {
			active = &_inactive;
			inactive = &_active;
		} else {
			active = &_active;
			inactive = &_inactive;
		}
		onActive = !onActive;
		return active;
	}
	SwapBuffer() {
		active = &_active;
		inactive = &_inactive;

		(*active) = new T();
		(*inactive) = new T();
	}
private:
	T *_active   = 0;
	T *_inactive = 0;
	bool onActive  = true;
};
	

namespace Memory {

	// template<int ElementSize, int ElementCount>
	// struct Pool {
	// 	void* pool;
	// 	size_t size, max;
	// 	Pool() : size(ElementSize), max(ElementCount) {
	// 		pool = malloc( ElementSize * ElementCount );
	// 	}
	// };

	namespace MemorySystem { };

	// template<int ElementSize, int ElementCount>
	// void* InitPool() {
	// 	return new Pool<ElementSize,ElementCount>();
	// }

};

#endif
