#pragma once
#ifndef DKUTIL_H
#define DKUTIL_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

namespace DKUtil {

	[[nodiscard]]
	std::string loadTextFile(std::string filepath);

	[[nodiscard]]
	std::vector<char> loadBinaryFile(std::string filepath);

	void saveBinaryFile(std::string filepath, const std::vector<char>& data);

	[[nodiscard]]
	inline char toLower(char c) { 
		return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	[[nodiscard]]
	inline bool isSpace(char c) {
		return std::isspace(static_cast<unsigned char>(c));
	}
	[[nodiscard]]
	inline bool isDigit(char c) {
		return std::isdigit(static_cast<unsigned char>(c));
	}

	//Get length of the text as if all trailing spaces were removed.
	[[nodiscard]]
	inline size_t getTrimmedLength(const std::string_view& str, size_t offset, size_t count) {
		if (count == 0u || offset >= str.size())
			return 0u;

		if (count > str.size() - offset) {
			count = str.size() - offset;
		}

		for (size_t i = offset+count-1u; i > offset; --i) {
			if (false == isSpace(str[i]))
				return (i+1u) - offset;
		}

		if (false == isSpace(str[offset]))
			return 1u;
		else
			return 0u;
	}

	inline uint32_t wellonsHash(uint32_t x, uint32_t seed = 0U);
	inline uint32_t wellonsHash2D(uint32_t x, uint32_t y, uint32_t seed = 0U);
	inline uint32_t wellonsHash3D(uint32_t x, uint32_t y, uint32_t z, uint32_t seed = 0U);
}

#endif