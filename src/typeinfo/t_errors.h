#ifndef __T_ERRORS_H__
#define __T_ERRORS_H__


#include <exception>
#include <stdexcept>
#include <string>

using namespace std;

/*
=================================================

	all errors relating to the system (no game specifics!)

=================================================
*/

typedef unsigned int t_error;

static const t_error NO_ERROR = 0;
static const t_error ERROR    = 1;


struct BadFileException : public std::invalid_argument { BadFileException(const string& err) : std::invalid_argument(err){}; };

#endif
