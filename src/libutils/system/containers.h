#ifndef __LIB_CONTAINERS_H__
#define __LIB_CONTAINERS_H__


template<class T1, class T2, class T3>
struct Triple {
	T1 t1; T2 t2; T3 t3;
	Triple(T1 _t1, T2 _t2, T3 _t3) : t1(_t1), t2(_t2), t3(_t3) {
		t1 = _t1;
		t2 = _t2;
		t3 = _t3;
	}
	Triple() {}
};

#endif
