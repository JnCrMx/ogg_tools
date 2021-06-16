#include "oggdef.hpp"
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

void readFile(std::string path, std::vector<char>& vector)
{
	std::ifstream in(path, std::ios::binary | std::ios::ate);
	int len = in.tellg();
	vector.resize(len);
	in.seekg(0);
	in.read(vector.data(), len);
}

void writeFrame(OggFrame frame, std::ofstream& out)
{
	std::vector<char> v(frame.begin, frame.end);
	out.write(v.data(), v.size());
}

OggFrame readFrame(std::vector<char>::iterator& it)
{
	auto thisBegin = it;

	OggHeader ogg = *(OggHeader*)&*it;
	if(std::string(ogg.capture_pattern) != "OggS")
	{
		throw std::runtime_error("Ogg capture pattern mismatch");
	}

	it += sizeof(OggHeader);

	int sum = 0;
	for(int i=0; i<ogg.page_segments; i++)
	{
		sum += (u8)*it;
		it++;
	}
	it += sum;

	auto thisEnd = it;
	return {ogg, thisBegin, thisEnd};
}

struct AudioAttributes
{
	u8 channels;
	u32 sample_rate;

	std::string to_string()
	{
		return "channels = "+std::to_string(channels)+", sample_rate = "+std::to_string(sample_rate)+" Hz";
	}

	bool operator==(AudioAttributes other)
	{
		return channels == other.channels && sample_rate == other.sample_rate;
	}
};

AudioAttributes getAudioAttributes(std::vector<char>::iterator it)
{
	OggFrame frame = readFrame(it);

	auto jt = frame.begin;
	jt += sizeof(OggHeader);
	jt += frame.header.page_segments;

	VorbisHeader vorbis = *(VorbisHeader*)&*jt;

	if(vorbis.packet_type != 0x01 || std::string(vorbis.vorbis) != "vorbis")
		throw std::runtime_error("illegal vorbis header: packet type "+std::to_string(vorbis.packet_type)+" and signature \""+vorbis.vorbis+"\"");

	return {vorbis.audio_channels, vorbis.audio_sample_rate};
}

int main(int argc, char* argv[])
{
	if(argc != 6)
	{
		std::cerr << "Usage: oggreplay <input1> <intermediate> <input2> <spacer> <output>" << std::endl
			<< std::endl
			<< "\t" "input1          ogg file that is played first" << std::endl
			<< "\t" "intermediate    intermediate ogg (not played at all)" << std::endl
			<< "\t" "input2          ogg file that is played second" << std::endl
			<< "\t" "spacer          mp3 file that is used to confuse the length detection (should be ~1:00 of silence)" << std::endl
			<< "\t" "output          output ogg file" << std::endl
			<< std::endl
			<< "\t" << "The inputs \"input1\" and \"input2\" must have the same sample rate and channel count." << std::endl
			<< "\t" << "The sample rate of the \"intermediate\" input does not seem to matter at all." << std::endl;
		return 1;
	}

	std::string input1 = argv[1];
	std::string intermediate = argv[2];
	std::string input2 = argv[3];
	std::string spacer = argv[4];
	std::string output = argv[5];

	std::vector<char> data1, interdata, data2, spacerdata;
	readFile(input1, data1);
	readFile(intermediate, interdata);
	readFile(input2, data2);
	readFile(spacer, spacerdata);

	std::ofstream out(output, std::ios::binary);

	auto audio_attributes1 = getAudioAttributes(data1.begin());
	auto audio_attributes2 = getAudioAttributes(data2.begin());
	if(audio_attributes1 != audio_attributes2)
	{
		std::cout << "input1 and input2 differ in sample rate or channel count: " << std::endl << std::left
			<< "[input1] " << std::setw(std::max(input1.size(), input2.size())) << input1 << " :   " << audio_attributes1.to_string() << std::endl
			<< "[input2] " << std::setw(std::max(input1.size(), input2.size())) << input2 << " :   " << audio_attributes2.to_string() << std::endl;
		std::cout << "The output file might not work as expected!" << std::endl;
	}

	// find last frame of input1
	OggFrame lastFrame;
	for(auto it = data1.begin(); it != data1.end();)
	{
		lastFrame = readFrame(it);
	}

	// write entire input1
	out.write(data1.data(), data1.size());

	auto it = interdata.begin();

	// write first frame of intermediate
	writeFrame(readFrame(it), out);
	// write last frame of input1 (already written before)
	writeFrame(lastFrame, out);
	// write second frame of intermediate
	writeFrame(readFrame(it), out);

	// write entire input2
	out.write(data2.data(), data2.size());

	// write remaining part of intermediate
	std::vector<char> v(it, interdata.end());
	out.write(v.data(), v.size());

	// write spacer (a random, relatively long MP3 file to confuse Chromium's duration detection)
	out.write(spacerdata.data(), spacerdata.size());
	// write entire input1 again (this is where Chromium will read the total duration of the entire audio file from)
	out.write(data1.data(), data1.size());
}
