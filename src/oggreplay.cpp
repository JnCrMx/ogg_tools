#include "oggdef.hpp"
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

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
		++it;
		throw std::runtime_error("capture pattern mismatch");
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

int getSampleRate(std::vector<char>::iterator it)
{
	OggFrame frame = readFrame(it);

	auto jt = frame.begin;
	jt += sizeof(OggHeader);
	jt += frame.header.page_segments;

	VorbisHeader vorbis = *(VorbisHeader*)&*jt;
	
	if(vorbis.packet_type != 0x01 || std::string(vorbis.vorbis) != "vorbis")
		throw std::runtime_error("illegal vorbis header: packet type "+std::to_string(vorbis.packet_type)+" and signature \""+vorbis.vorbis+"\"");

	return vorbis.audio_sample_rate;
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
			<< "\t" << "The inputs \"input1\" and \"input2\" must have the same sample rate." << std::endl
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

	int sampleRate1 = getSampleRate(data1.begin());
	int sampleRate2 = getSampleRate(data2.begin());
	if(sampleRate1 != sampleRate2)
	{
		std::cout << "input1 and input2 differ in sample rate: " << sampleRate1 << " Hz vs " << sampleRate2 << " Hz." << std::endl;
		std::cout << "The output file might not work as expected!" << std::endl;
	}

	OggFrame lastFrame;
	for(auto it = data1.begin(); it != data1.end();)
	{
		lastFrame = readFrame(it);
	}

	out.write(data1.data(), data1.size());

	auto it = interdata.begin();
	
	writeFrame(readFrame(it), out);
	writeFrame(lastFrame, out);
	writeFrame(readFrame(it), out);

	out.write(data2.data(), data2.size());

	std::vector<char> v(it, interdata.end());
	out.write(v.data(), v.size());

	out.write(spacerdata.data(), spacerdata.size());
	out.write(data1.data(), data1.size());
}
