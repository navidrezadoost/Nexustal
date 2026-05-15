#pragma once

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace nexustal::domain
{
using UUID = std::string;
using Timestamp = std::chrono::system_clock::time_point;

inline auto generate_uuid() -> UUID
{
	static thread_local std::mt19937_64 generator{std::random_device{}()};
	std::uniform_int_distribution<unsigned int> distribution{0, 255};

	unsigned char bytes[16];
	for (auto& byte : bytes)
	{
		byte = static_cast<unsigned char>(distribution(generator));
	}

	bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
	bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

	std::ostringstream stream;
	stream << std::hex << std::setfill('0');
	for (int index = 0; index < 16; ++index)
	{
		stream << std::setw(2) << static_cast<int>(bytes[index]);
		if (index == 3 || index == 5 || index == 7 || index == 9)
		{
			stream << '-';
		}
	}

	return stream.str();
}
}
