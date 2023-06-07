#include "DKUtil.h"
namespace DKUtil {
	std::string loadTextFile(std::string filepath) {
		auto binaryData = loadBinaryFile(filepath);
		return std::string(binaryData.data(), binaryData.size());
	}

	std::vector<char> loadBinaryFile(std::string filepath) {
		std::ifstream filestream(filepath, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);

		if (!filestream.is_open() || filestream.fail())
			throw std::runtime_error(std::string("File could not be opened: ") + filepath);

		size_t fileSize = filestream.tellg();
		std::vector<char> data(fileSize);

		filestream.seekg(0, std::ios_base::beg);
		filestream.read(data.data(), fileSize);

		filestream.close();

		return data;
	}

	void saveBinaryFile(std::string filepath, const std::vector<char>& data) {
		std::ofstream filestream(filepath, std::ios_base::out | std::ios_base::binary);
		if (!filestream.is_open() || filestream.fail())
			throw std::runtime_error(std::string("File could not be opened for saving: ") + filepath);

		filestream.write(data.data(), data.size());
		filestream.close();
	}
	
	constexpr bool isWhitespace(char c) {
		switch (c) {
		case ' ':  //space.'\u0020'
		case '\r': //Carriage return / end of line.'\u000D'
		case '\n': //Line feed / new line.'\u000A'
		case '\t': //Tab. '\u0009'
		//case '\000B': //Vertical tab.
			return true;
		default:
			return false;
		}
	}


	extern inline uint32_t wellonsHash(uint32_t x, uint32_t seed) {

		/*
		constexpr uint32_t BIT_NOISE0 = UINT32_C(0x7feb352d);
		constexpr uint32_t BIT_NOISE1 = UINT32_C(0x846ca68b);

		x ^= x >> 16;
		x *= BIT_NOISE0;
		x ^= x >> 15;
		x *= BIT_NOISE1;
		x ^= x >> 16;

		return x;
		*/
		constexpr uint32_t BIT_NOISE0 = UINT32_C(0xed5ad4bb);
		constexpr uint32_t BIT_NOISE1 = UINT32_C(0xac4c1b51);
		constexpr uint32_t BIT_NOISE2 = UINT32_C(0x31848bab);
		x ^= x >> 17;
		x += seed;
		x *= BIT_NOISE0;
		x ^= x >> 11;
		x *= BIT_NOISE1;
		x ^= x >> 15;
		x *= BIT_NOISE2;
		x ^= x >> 14;
		return x;

	}

	extern inline uint32_t wellonsHash2D(uint32_t x, uint32_t y, uint32_t seed) {
		constexpr uint32_t PRIME = 198491317u;
		return wellonsHash(x + PRIME * y, seed);
	}

	extern inline uint32_t wellonsHash3D(uint32_t x, uint32_t y, uint32_t z, uint32_t seed) {
		constexpr uint32_t PRIME0 = 198491317u;
		constexpr uint32_t PRIME1 = 6542989u;
		return wellonsHash(x + (PRIME0 * y) + (PRIME1 * z), seed);
	}
}