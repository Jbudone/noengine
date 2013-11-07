#ifndef __LIB_CONTAINERS_H__
#define __LIB_CONTAINERS_H__


/***
 * Containers
 *
 *	    Various data structures
 *
 * TODO
 *  - ...
 *
 **/
#include <stdlib.h>
#define CHAR0 48 // ASCII dec code for 0

template<class T1, class T2, class T3>
struct Triple {
	Triple() { }
	Triple(T1 t1, T2 t2, T3 t3) : t1(t1), t2(t2), t3(t3) { }
	T1 t1; T2 t2; T3 t3;
};

/* Array_Resizable
 *
 * An array which is resized when an item is added or
 * removed. The array is an unordered set
 **/
template<class T>
struct Array_Resizable {
	Array_Resizable() : elements(0), size(0) { }
	void add(T element) {
		T* new_elements = (T*)malloc( sizeof(T) * size+1 );
		int i = 0;
		for ( ; i < size; ++i ) {
			new_elements[i] = elements[i];
		}
		new_elements[i] = element;
		++size;

		delete elements;
		elements = new_elements;
	}
	bool has(T element) {
		int i = 0;
		for ( ; i < size; ++i ) {
			if ( elements[i] == element ) return true;
		}
		return false;
	}
	void remove(T element) {
		if ( size == 0 ) return;
		T* new_elements = (T*)malloc( sizeof(T) * size-1 );
		int j = 0, k = 0;
		for ( ; j < size, k < size-1; ++j ) {
			if ( elements[j] == element ) continue;
			new_elements[k] = elements[j];
			++k;
		}
		--size;

		delete elements;
		elements = new_elements;
	}
	T& operator[](size_t index) {
		return elements[index];
	}

	T* elements;
	unsigned short size;
};


/*
=================================================

	Bitfield

  An extremely flexible container for storing
  variable-length data structures and easily retrieving it
  later. The way this is built is very efficient for storing
  and reading back data later

=================================================
*/
template<unsigned char SIZE>
struct Bitfield {
	Bitfield() {
		fields = new unsigned char[SIZE];
		for ( unsigned char i = 0; i < size; ++i ) {
			fields[i] = 0;
		}
	}

	Bitfield(const char* message) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < size; ++i ) {
			fields[i] = (unsigned char)(message[i]);
		}
	}

	Bitfield(int num) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < size; ++i ) {
			fields[i] = 0;
		}
		fields[0] = (unsigned char)num;
	}

	Bitfield(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); ++i ) {
			fields[i] = rhs.fields[i];
		}
	}

	~Bitfield() {
			delete[] fields;
	}

	Bitfield<SIZE>& operator=(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); ++i ) {
			fields[i] = rhs.fields[i];
		}
		return *this;
	}

	void copy(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); ++i ) {
			fields[i] = rhs.fields[i];
		}
	}
	
	operator const char*() const {
		char *theseFields = new char[size+1];
		for ( unsigned char i = 0; i < size; ++i ) {
			theseFields[i] = fields[i];
		}
		theseFields[size] = '\0';
		return theseFields;
		// return fields;
	}

	const char* getChar() {
		char *theseFields = new char[size+1];
		for ( unsigned char i = 0; i < size; ++i ) {
			theseFields[i] = (char)(fields[i]+48); // NOTE: pretty-print to show ascii
		}
		theseFields[size] = '\0';
		return theseFields;
	}

	template<class T> void set(T num, unsigned char position) {
		for ( unsigned char i = position; i < (position + sizeof(num)); ++i ) {
			fields[i] = (unsigned char)num;
			num >>= 8;
		}
	}

	template<class T> T fetch(unsigned char position) {
		T num = 0;
		unsigned char i = position;
		unsigned char offset = 0;
		for ( ; i < (position + sizeof(num)); ++i, offset++ ) {
			num += ( fields[i] << (offset * 8) );
		}
		return num;
	}

	template<int argsize> unsigned char append(const Bitfield<argsize>* args, unsigned char offset) {
		for ( unsigned char i = offset, j = 0; j < args->size; ++i, ++j ) {
			this->fields[i] = (char)args->fields[j];
		}
		return offset + args->size;
	}

	template<class T> unsigned char append(T args, unsigned char offset) {
		this->set<T>( args, offset );
		return offset + sizeof(T);
	}
	
	const unsigned char size = SIZE;
	unsigned char *fields;
};

typedef Bitfield<32>  Bfield32_t ;
typedef Bitfield<64>  Bfield64_t ;
typedef Bitfield<128> Bfield128_t;


#endif
