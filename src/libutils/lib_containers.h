#ifndef __LIB_CONTAINERS_H__
#define __LIB_CONTAINERS_H__

#include <stdio.h>
#include <string>
#include "libutils/lib_logger.h"
#define CHAR0 48
/*
 * Containers
 *
 * TODO
 *
 *  > ...
 *
 ***/

template<class T1, class T2, class T3>
struct Triple {
	Triple() { }
	Triple(T1 t1, T2 t2, T3 t3) : t1(t1), t2(t2), t3(t3) { }
	T1 t1; T2 t2; T3 t3;
};

template<unsigned char SIZE>
struct Bitfield {
	Bitfield() {
		fields = new unsigned char[SIZE];
		for ( unsigned char i = 0; i < size; i++ ) {
			fields[i] = 0;
		}
	}

	Bitfield(const char* message) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < size; i++ ) {
			fields[i] = (unsigned char)(message[i]);
		}
	}

	Bitfield(int num) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < size; i++ ) {
			fields[i] = 0;
		}
		fields[0] = (unsigned char)num;
	}

	Bitfield(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); i++ ) {
			fields[i] = rhs.fields[i];
		}
	}

	~Bitfield() {
			delete[] fields;
	}

	Bitfield<SIZE>& operator=(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); i++ ) {
			fields[i] = rhs.fields[i];
		}
		return *this;
	}

	void copy(const Bitfield<SIZE> &rhs) {
		fields = new unsigned char[SIZE]();
		for ( unsigned char i = 0; i < (rhs.size <= this->size ? rhs.size : this->size); i++ ) {
			fields[i] = rhs.fields[i];
		}
	}
	
	operator const char*() const {
		char *theseFields = new char[size+1];
		for ( unsigned char i = 0; i < size; i++ ) {
			theseFields[i] = fields[i];
		}
		theseFields[size] = '\0';
		return theseFields;
		// return fields;
	}

	const char* getChar() {
		char *theseFields = new char[size+1];
		for ( unsigned char i = 0; i < size; i++ ) {
			theseFields[i] = (char)(fields[i]+48); // NOTE: pretty-print to show ascii
		}
		theseFields[size] = '\0';
		return theseFields;
	}

	template<class T> void set(T num, unsigned char position) {
		for ( unsigned char i = position; i < (position + sizeof(num)); i++ ) {
			fields[i] = (unsigned char)num;
			num >>= 8;
		}
	}

	template<class T> T fetch(unsigned char position) {
		T num = 0;
		unsigned char i = position;
		unsigned char offset = 0;
		for ( ; i < (position + sizeof(num)); i++, offset++ ) {
			num += ( fields[i] << (offset * 8) );
		}
		return num;
	}

	template<int argsize> unsigned char append(const Bitfield<argsize>* args, unsigned char offset) {
		for ( unsigned char i = offset, j = 0; j < args->size; i++, j++ ) {
			this->fields[i] = (char)args->fields[j];
		}
		return offset + args->size;
	}

	template<class T> unsigned char append(T args, unsigned char offset) {
		this->set<T>(args,offset);
		return offset + sizeof(T);
	}
	
	const unsigned char size = SIZE;
	unsigned char *fields;
private:
	// unsigned char *fields;
};

typedef Bitfield<32>  Bfield32_t ;
typedef Bitfield<64>  Bfield64_t ;
typedef Bitfield<128> Bfield128_t;


#endif
