// midi parsing

#pragma once

#include <array>
#include <assert.h>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <memory>
#include <vector>

namespace vibe {

typedef std::shared_ptr<class MidiParser> ShMidiParserPr;
typedef unsigned long long uint64;
typedef std::array<std::byte, 8> message;

class MidiParser {

	// properties
protected:
	// midi file name
	std::string fpath_;

	// midi file stream
	std::ifstream mfile_;

	// all messages and bytes
	std::vector<message> all_messages_;
	std::vector<std::byte> all_bytes_;

	// total message and bytes
	int num_messages_;
	char header_type_[4];
	short header_length_;

	// total bytes
	int num_bytes_;

	// bits array
	std::vector<int> status_bits_;

	// byte struct
	struct midibyte_ {
		int status_bit : 1; // 1 bit wide
		int data_bits : 7; //
	};

	// methods
public:
	// constructor
	MidiParser();

	// factory
	static ShMidiParserPr create();

	// load the midi file
	void load_file();
	void load_file(std::string);

	std::string byte_to_string(std::byte mybyte);
	void bytes_to_messages();
	void print_byte(std::byte);
	void parse_messages();
	void parse_message(message);

	void bytes_to_header_chunk();
	void print_message(message);
	void print_header_format(int);
	void print_header_division(int);

	// setters
	void set_fpath(std::string);

	// getters
	std::vector<std::byte> get_bytes();
	std::vector<message> get_messages();
};

} // namespace vibe
