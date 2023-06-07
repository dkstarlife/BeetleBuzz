#pragma once

#ifndef BEETLEBUZZ_USE_DUMMYSOCKET
#define BEETLEBUZZ_USE_DUMMYSOCKET false
#endif

#define BEETLEBUZZ_VERSION 1000u

#include <iostream>
#include <string>
#include <cstdlib>
#include <string>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <chrono>
#include <sstream>

#pragma warning( push )

#include <codeanalysis\warnings.h>
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )

#include <Boost/asio/ssl.hpp>
#include <Boost/beast/core.hpp>
#include <Boost/beast/ssl.hpp>
#include <Boost/beast/websocket.hpp>
#include <Boost/beast/websocket/ssl.hpp>
#include <Boost/asio/connect.hpp>
#include <Boost/asio/ip/tcp.hpp>
#include <Boost/asio/ssl/stream.hpp>
#pragma warning( pop )

#include "DKUtil.h"
#include "BeetleBuzzTypes.h"
#include "CommandSearchTree.h"
#include "Message.h"

#include "ColorList.h"
#include "FortuneList.h"
#if BEETLEBUZZ_USE_DUMMYSOCKET == true
#include "DummySocket.h"
#endif

namespace BeetleBuzz {

	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace websocket = beast::websocket;
	namespace ssl = boost::asio::ssl;
	using tcp = boost::asio::ip::tcp;

	using namespace std::chrono_literals;

	class BeetleBuzz {
		std::vector<std::pair<ChatCommand, CommandState>> commands{
			{ChatCommand("!Dice", std::bind(&BeetleBuzz::diceCmd, this, std::placeholders::_1, std::placeholders::_2)) , CommandState(1s)},
			{ChatCommand("!MyColor", std::bind(&BeetleBuzz::personalColorCmd, this, std::placeholders::_1, std::placeholders::_2)) , CommandState(1s) },
			{ChatCommand("!MyElement", std::bind(&BeetleBuzz::personalElementCmd, this, std::placeholders::_1, std::placeholders::_2)) , CommandState(1s) },
			{ChatCommand("!MyFortune", std::bind(&BeetleBuzz::dailyFortuneCmd, this, std::placeholders::_1, std::placeholders::_2)) , CommandState(1s)},
			{ChatCommand("!PickOne", std::bind(&BeetleBuzz::pickOneCmd, this, std::placeholders::_1, std::placeholders::_2)) , CommandState(1s)}
		};

		CommandSearchTree searchTree;

		boost::asio::io_context ioContext;
		ssl::context sslContext{ ssl::context::tls_client };
		tcp::resolver tcpResolver{ ioContext };
		tcp::endpoint endPoint;
#if BEETLEBUZZ_USE_DUMMYSOCKET == false
		websocket::stream<beast::ssl_stream<tcp::socket>> socketStream{ ioContext, sslContext };
#else
		DummySocket socketStream;
#endif
		beast::flat_buffer inBuffer;
		beast::flat_buffer outBuffer;

		const std::function<void(TTV::Message&)> messageRecieveCallback;

		void messageReadLoop(std::stop_token stopToken);
		void actionLoop(std::stop_token stopToken);

		void performDisconnect();

		void sendMessage(const TTV::Message& msg);

		bool diceCmd(TTV::Message& input, CommandState& cmdState);
		bool personalElementCmd(TTV::Message& input, CommandState& cmdState);
		bool personalColorCmd(TTV::Message& input, CommandState& cmdState);
		bool dailyFortuneCmd(TTV::Message& input, CommandState& cmdState);
		bool pickOneCmd(TTV::Message& input, CommandState& cmdState);

		struct {
			uint32_t msgCount = 0u;
			const uint32_t msgLimit = 19u; //20 for pleb, 100 for mod, 7500 for verified bot.
			const uint32_t lowPriorityLimit = msgLimit - 2;
			const std::chrono::steady_clock::duration msgFrameDuration = std::chrono::seconds(30); 
			std::chrono::steady_clock::time_point lastResetTime = std::chrono::steady_clock::now();

			bool attemptIncrement() {
				update();
				if (msgCount < lowPriorityLimit) {
					++msgCount;
					return true;
				}
				else
					return false;
			}
			bool attemptIncrementPriority() {
				update();
				if (msgCount < msgLimit) {
					++msgCount;
					return true;
				}
				else
					return false;
			}

			void update() {
				if (std::chrono::steady_clock::now() - lastResetTime > msgFrameDuration) {
					msgCount = 0u;
					lastResetTime = std::chrono::steady_clock::now();
				}
			}
		}messageCounter;

		std::queue<TTV::Message> privmsgSendQueue;
	public:
		bool isConnected{ false };

		std::jthread actionThread;
		std::queue<Action> actionQueue;
		std::mutex actionQueueMutex;

		std::jthread messageThread;
		std::queue<TTV::Message> messageQueue;
		std::mutex messageQueueMutex;

		constexpr std::vector<std::string> getCommandStrings() {
			std::vector<std::string> commandStrings;
			commandStrings.reserve(commands.size());
			for (const auto& cmdPair : commands) {
				commandStrings.push_back(cmdPair.first.command);
			}
			return commandStrings;
		}

		BeetleBuzz(std::function<void(TTV::Message&)> messageRecieveCallback) : searchTree{ getCommandStrings() }, messageRecieveCallback{ messageRecieveCallback } {
			
		}

		bool connect(std::string NICK, std::string PASS, std::string capabilities = "CAP REQ :twitch.tv/commands twitch.tv/tags");
		void disconnect();

		void joinRoom(std::string channelName);
		void leaveRoom(std::string channelName);


		bool nullOp(TTV::Message& input) { return true; };
	};

}