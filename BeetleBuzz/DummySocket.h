#pragma once
#include <iostream>
#include <chrono>

#pragma warning( push )
#include <codeanalysis\warnings.h>
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include <Boost/beast/core.hpp>
#include <Boost/beast/websocket.hpp>
#include <Boost/beast/websocket/ssl.hpp>
#include <Boost/asio/connect.hpp>
#include <Boost/asio/ip/tcp.hpp>
#include <Boost/asio/ssl/stream.hpp>
#pragma warning( pop )

#include "DKUtil.h"

namespace BeetleBuzz {
		namespace beast = boost::beast;
		namespace http = beast::http;
		namespace websocket = beast::websocket;
		namespace ssl = boost::asio::ssl;
		using tcp = boost::asio::ip::tcp;

	class DummySocket {

		std::vector<std::string> dummyData{ ":tmi.twitch.tv 001 beetlebuzz :Welcome, GLHF!",
											":tmi.twitch.tv 002 beetlebuzz :Your host is tmi.twitch.tv",
											":tmi.twitch.tv 003 beetlebuzz :This server is rather new",
											":tmi.twitch.tv 004 beetlebuzz :-",
											":tmi.twitch.tv 375 beetlebuzz :-",
											":tmi.twitch.tv 372 beetlebuzz :You are in a maze of twisty passages, all alike.",
											":tmi.twitch.tv 376 beetlebuzz :>",
											":tmi.twitch.tv CAP * ACK :twitch.tv/commands twitch.tv/tags" };
		size_t currentIndex = 0u;

		std::vector<std::string> prefixes{"@mod=1;subscriber=1;user-type=mod :modman!modman@modman.tmi.twitch.tv PRIVMSG #broadcaster :",
											"@mod=0;subscriber=1;user-type= :normie!normie@normie.tmi.twitch.tv PRIVMSG #broadcaster :",
											"@mod=0;subscriber=0;user-type= :plebian!plebian@plebian.tmi.twitch.tv PRIVMSG #broadcaster :",
											"@mod=0;subscriber=0;user-type= :timmy!timmy@timmy.tmi.twitch.tv PRIVMSG #broadcaster :" };
		std::vector<std::string> messages{"Hi!",
											"Beep Boop.",
											"I like this content",
											"KEKW",
											"!myfortune",
											"HAHA YAY!",
											"!DICE",
											"!myelement",
											"!dice20",
											"!dice256",
											"!mycolor",
											"!pickone juice | milk|water |green tea",
											"!pickone grass"};
		

	public:

		std::string synthesizeMessage() {
			if (currentIndex < 8) {
				return dummyData[currentIndex++];
			}
			++currentIndex;
			return prefixes[DKUtil::wellonsHash((uint32_t)currentIndex, 29) % prefixes.size()] + messages[DKUtil::wellonsHash((uint32_t)currentIndex * 2u, 47) % messages.size()];
		}

		size_t read(boost::beast::flat_buffer& buffer) {
			std::string msg{ synthesizeMessage() };

			auto writeBuffer{ buffer.prepare(msg.size()) };
			memcpy(writeBuffer.data(), msg.data(), msg.size());
			buffer.commit(msg.size());
			
			std::this_thread::sleep_for(std::chrono::milliseconds(300));

			return msg.size();
		}

		size_t write(const boost::asio::mutable_buffer & buffer) {
			//std::string str((char*)buffer.data(), buffer.size());
			//std::cout << "~~~Wrote to socket: " << str << std::endl;
			return buffer.size();
		}

		void close(websocket::close_code cc) {
			//std::cout << "DummySocket closed" << std::endl;
		}

		bool is_open() { return true; }
	};
}