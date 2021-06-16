#include <assert.h>
#include <cstdint>
#include <ios>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <map>

#include "oggdef.hpp"

int main(int argc, char* argv[]) 
{
	assert(argc >= 2);

	std::vector<char> data;
	{
		std::ifstream in(argv[1], std::ios::binary | std::ios::ate);
		int len = in.tellg();
		data.resize(len);
		in.seekg(0);
		in.read(data.data(), len);
	}

	std::string outputDir="./";
	if(argc >= 3)
	{
		outputDir = argv[2];
	}

	auto it = data.begin();
	
	std::map<u32, std::vector<OggFrame>> streams;

	while(it != data.end())
	{
		auto thisBegin = it;

		OggHeader ogg = *((OggHeader*)&(*it)); 
		if(std::string(ogg.capture_pattern) != "OggS")
		{
			std::cout << "Wrong signature @ " << std::hex << std::distance(data.begin(), it) << " : " << ogg.capture_pattern << " != OggS" << std::endl;
			std::cout << std::hex 
				<< (int)(unsigned char)ogg.capture_pattern[0] << " "
				<< (int)(unsigned char)ogg.capture_pattern[1] << " "
				<< (int)(unsigned char)ogg.capture_pattern[2] << " "
				<< (int)(unsigned char)ogg.capture_pattern[3] << std::endl;

			std::string signature;
			do
			{
				it++;
				signature = std::string(it, it+4);
			}
			while(signature != "OggS" && it != data.end());
			continue;
		}

		std::cout << ogg.capture_pattern << " @ " << std::hex << std::distance(data.begin(), it) << std::dec << " ; ";
		std::cout << "position = " << ogg.granule_position << ", serial = " << ogg.bitstream_serial_number << " ; ";
		std::cout << "seq " << ogg.page_sequence_number << " ; ";
		std::cout << "continue = " << std::boolalpha << ogg.header_type.f_continue 
			<< ", first page = " << ogg.header_type.f_first_page
			<< ", last page = " << ogg.header_type.f_last_page << "; ";

		it += sizeof(OggHeader);

		int sum = 0;
		for(int i=0; i<ogg.page_segments; i++)
		{
			sum += (u8)*it;
			it++;
		}
		std::cout << (int)ogg.page_segments << " segments -> " << sum << " bytes";

		it += sum;
		std::cout << std::endl;

		auto thisEnd = it;

		OggFrame frame = {
			ogg,
			thisBegin,
			thisEnd
		};
		streams[ogg.bitstream_serial_number].push_back(frame);
	}

	for(auto it = streams.begin(); it != streams.end(); ++it)
	{
		std::string name = std::to_string(it->first);

		std::vector<OggFrame> frames = it->second;
		std::sort(frames.begin(), frames.end(), [](OggFrame f1, OggFrame f2){
			return f1.header.page_sequence_number < f2.header.page_sequence_number;
		});
		auto last = std::unique(frames.begin(), frames.end(), [](OggFrame f1, OggFrame f2){
			return f1.header.page_sequence_number == f2.header.page_sequence_number;
		});
		if(last != frames.end())
		{
			std::cout << "[" << name << "] Found " << std::distance(last, frames.end()) << " duplicate frames" << std::endl;
			frames.erase(last, frames.end());
		}

		std::ofstream out(outputDir+"/"+name+".ogg");
		for(OggFrame frame : frames)
		{
			std::vector<char> v(frame.begin, frame.end);
			out.write(v.data(), v.size());
		}
	}
}
