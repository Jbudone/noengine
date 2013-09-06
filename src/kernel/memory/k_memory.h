#ifndef __K_MEMORY_H__
#define __K_MEMORY_H__


/***
 * Memory
 *
 *	    More data structures
 *
 * TODO
 *  - implement various storage types (pool, double-sided allocator, one-frame allocator)
 *  - pointer-fixup table (GUID)
 *  - SwapBuffer: do we need to store as T** or will T* work? Can we make the swap more atomic? (xor swap)
 *
 **/


#include "util.inc.h"

/*
=================================================

	SwapBuffer

 In order to avoid shared resource fighting and race
 conditions, the swap buffer allows you to write into one
 object and then use the other one for reading. For example
 the network may continue to receive packets and place them
 into the active buffer, then when the read takes place the
 we can swap() the buffer and read from the inactive buffer

=================================================
*/
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
