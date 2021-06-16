#include <assert.h>
#include <cstdint>
#include <ios>
#include <iterator>
#include <ostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>

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

	std::vector<char>::iterator begin, end;
	bool inside;

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

		if(ogg.header_type.f_first_page)
		{
			begin = it;
			inside = true;
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
		if(ogg.header_type.f_last_page)
		{
			end = it;
			inside = false;

			std::stringstream name;
			name << outputDir << "/" << std::hex << std::distance(data.begin(), begin) << "-" << std::distance(data.begin(), end) << ".ogg";

			std::ofstream out(name.str());
			std::vector<char> v(begin, end);
			out.write(v.data(), v.size());

			std::cout << "\t\t extracted " << std::distance(begin, end) << " bytes -> " << name.str();
		}
		else if(!inside)
		{
			auto thisEnd = it;

			std::stringstream name;
			name << outputDir << "/" << std::hex << std::distance(data.begin(), thisBegin) << "-" << std::distance(data.begin(), thisEnd) << ".ogg";

			std::ofstream out(name.str());
			std::vector<char> v(thisBegin, thisEnd);
			out.write(v.data(), v.size());

			std::cout << "\t\t extracted single frame (" << std::distance(thisBegin, thisEnd) << " bytes) -> " << name.str();
		}
		std::cout << std::endl;
	}
}
