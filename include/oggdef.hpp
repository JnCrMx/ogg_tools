#pragma once
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <vector>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

struct HeaderType {
	bool f_continue : 1;
    bool f_first_page : 1;
	bool f_last_page : 1;
};

struct __attribute__ ((packed)) OggHeader
{
	char capture_pattern[4];
	u8 version;
	HeaderType header_type;
	u64 granule_position;
	u32 bitstream_serial_number;
	u32 page_sequence_number;
	u32 CRC_checksum;
	u8 page_segments;
};

struct __attribute__ ((packed)) VorbisHeader
{
	u8 packet_type;
	char vorbis[6];
	u32 vorbis_version;
	u8 audio_channels;
	u32 audio_sample_rate;
	u32 bitrate_maximum;
	u32 bitrate_nominal;
	u32 bitrate_minimum;
	u8 blocksize_0 : 4;
	u8 blocksize_1 : 4;
};

struct OggFrame
{
	OggHeader header;
	std::vector<char>::iterator begin;
	std::vector<char>::iterator end;
};
