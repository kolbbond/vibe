#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstring>
#include "common.hxx"
#include "Log.hxx"
#include "typedefs.hxx"

// filename macro for shortening the path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    
// macro for error
class vibe_error: public std::runtime_error {
private:
	std::string msg;

public:
	// constructor
	vibe_error(const std::string& arg, const char* file, const char* function, int line) : std::runtime_error(arg) {
		std::ostringstream o;
		o << KRED << KBLD << "error: " << KNRM << file << ":" << function << ":" << KGRN << KBLD << line << KNRM << ": "
		  << std::endl
		  << " " KYEL << arg << KNRM;
		msg = o.str();
	}

	// destructor
	~vibe_error() throw() {}
	const char* what() const throw() override {
		return msg.c_str();
	}
};

class vibe_debug {
private:
	std::string msg;

public:
	// constructor
	vibe_debug(const std::string& arg, const char* file, const char* function, int line) {
		std::ostringstream o;
		o << KYEL << KBLD << "debug: " << KNRM << file << ":" << function << ":" << KGRN << KBLD << line << KNRM << ": "
		  << std::endl
		  << " " KYEL << arg << KNRM << std::endl;
		msg = o.str();
		std::cout << msg;
	}

	// destructor
	~vibe_debug() throw() {}
	/*
	const char* what() const throw() override {
		return msg.c_str();
	}
    */
};

// macro for error
#define vibe_throw_line(arg) throw vibe_error(arg, __FILENAME__, __FUNCTION__, __LINE__);
#define VIBE_DEBUG(arg) vibe_debug(arg, __FILENAME__, __FUNCTION__, __LINE__);
