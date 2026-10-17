// MidiParser.cpp
// ###### -

// include header
#include "MidiParser.hxx"
#include <cstdint>
#include <fstream>
#include <vector>

namespace vibe {

// constructor
MidiParser::MidiParser() {}

void MidiParser::load_file(std::string fpath) {
	// load the midi file

	// stream
	std::ifstream fin;

	// file
	printf("input file: %s\n", fpath.c_str());

	// open file
	fin.open(fpath, std::ifstream::binary);

	// check
	if(fin.fail()) {
		printf("File failed to load: %s\n", fpath.c_str());
		assert(0);
	}

	// count bytes
	int count = 0;

	// load in bytes
	while(!fin.eof()) {

		// get next byte
		std::byte mybyte;
		unsigned char mychar;
		fin >> mychar;
		mybyte = (std::byte)mychar;

		// debug
		// printf("load char %i: %s\n", count, &mychar);
		// print_byte(mybyte);

		// check
		if(fin.fail()) {
			printf("out of bytes\n");
		} else {

			all_bytes_.push_back(mybyte);
			count++;
		}
	}

	printf("bytes loaded: %i\n", count);

	num_bytes_ = count;

	// close file
	fin.close();

	// debug output
}

void MidiParser::parse_messages() {
	// actually parse midi messages

	// checks
	assert(!all_messages_.empty());

	message mymessage;

	// walk over messages
	for(int i = 0; i < all_messages_.size(); i++) {
		mymessage = all_messages_[i];

		// parse this message
		parse_message(mymessage);
	}
}

void MidiParser::parse_message(message mymessage) {
	// parse a single message

	// check ... has 8 bytes?


	// walk over 8 bytes within message
	for(int i = 0; i < 8; i++) {
	}
}

void MidiParser::bytes_to_messages() {
	// convert all bytes to list of messages

	// check that we have loaded bytes?
	assert(!all_bytes_.empty());

	// check that we have a integer multiple of 8?
	num_messages_ = num_bytes_ / 8;
	printf("num messages: %i\n", num_messages_);

	// initialize messages array

	// walk over bytes
	int count = 0;
	message mymessage;
	for(int i = 0; i < num_bytes_ - 1; i++) {

		mymessage[count] = all_bytes_[i];

		count++;

		// reset count
		if(count == 7) {
			count = 0;
			all_messages_.push_back(mymessage);
		}
	}
}


void MidiParser::bytes_to_header_chunk() {
	// parse the bytes into a header chunk

	// header chunk starts with 4 char header type (32 bits)
	// followed by 32 bit integer length
	// then additional data?
	printf("bytes to header chunk\n");

	char type[4];
	for(int i = 0; i < 4; i++) {
		type[i] = (char)all_bytes_[i];
	}
	printf("header type: %s\n", type);
	assert(std::string(type) == "MThd");

	// header length
	// stored MSB  LAST
	int32_t len = 0;
	for(int i = 0; i < 4; i++) {
		int idx = i + 4;
		len += (int32_t)all_bytes_[idx];
		len = len << (3 - i) * 8;

		// debug
		print_byte(all_bytes_[idx]);
	}
	printf("header length: %i\n", len);
	// is this always 6?
	assert(len == 6);

	// walk over format bytes
	int16_t format = 0;
	for(int i = 0; i < 2; i++) {
		int idx = i + 8;
		format += (int16_t)all_bytes_[idx];
		format = format << (1 - i) * 8;
	}
	printf("format: %i\n", format);

	// format is 0, 1, or 2
	assert(format == 0 || format == 1 || format == 2);
	print_header_format(format);

	// walk over ntrk bytes
	int16_t ntrks = 0;
	for(int i = 0; i < 2; i++) {
		int idx = i + 8;
		ntrks += (int16_t)all_bytes_[idx];
		ntrks = ntrks << (1 - i) * 8;
	}
	printf("ntrks: %i\n", ntrks);

	// ntrks is always 1 for a format==0
	if(format == 0) {
		assert(ntrks == 1);
	}

	// walk over division bytes
	int16_t division = 0;
	for(int i = 0; i < 2; i++) {
		int idx = i + 10;
		division += (int16_t)all_bytes_[idx];
		division = division << (1 - i) * 8;
	}
	printf("division: %i\n", division);
	print_header_division(division);
}


std::string MidiParser::byte_to_string(std::byte mybyte) {

	// std::string bytestr;
	// char* bytechars = new char [8];
	char bytechars[8];

	// binary masks
	for(int i = 0; i < 8; i++) {
		std::byte mask = (std::byte)std::pow(2, i);
		int mybit = (int)(mybyte & mask);
		// printf("%i",i);

		// check bits
		if(mybit > 0) {
			bytechars[7 - i] = '1';
		} else {
			bytechars[7 - i] = '0';
		}
	}

	// return
	return (std::string)bytechars;
}

void MidiParser::print_header_format(int format) {
	// print meaning of header format
	// 0, single multi-channel track
	// 1, one or more simultaneous
	// 2, one or more sequentially independent single track

	switch(format) {
	case 1:
		printf("single, multi-channel track\n");
		break;
	case 2:
		printf("one or more simultaneous\n");
		break;
	case 3:
		printf("one or more sequentially independent single track\n");
		break;
	default:
		printf("unknown format\n");
		break;
	}
}

void MidiParser::print_header_division(int division) {
	// print out meaning of division codes
	printf("fill out division meaning\n");
}

void MidiParser::print_message(message mymessage) {
	// print out a midi message
	// 8 bytes

	// printf("in print message\n");

	// walk over bytes
	// printf("msg: ");
	for(int i = 0; i < 8; i++) {
		printf("%s ", byte_to_string(mymessage[i]).c_str());
	}
	// printf("\n");
}
void MidiParser::print_byte(std::byte mybyte) {
	// print out a byte in bit format

	printf("%s\n", byte_to_string(mybyte).c_str());
}

void MidiParser::load_file() {
	// wrap to other function
	load_file(fpath_);
}

// setters
void MidiParser::set_fpath(std::string fpath) {
	// set the filepath

	// check it exists?

	// set
	fpath_ = fpath;
}

// getters

std::vector<std::byte> MidiParser::get_bytes() {
	// return
	return all_bytes_;
}

std::vector<message> MidiParser::get_messages() {
	// return
	return all_messages_;
}

// static

ShMidiParserPr MidiParser::create() {
	// factory
	return std::make_shared<MidiParser>();
}

} // namespace vibe
