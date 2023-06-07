#include "BeetleBuzz.h"

namespace BeetleBuzz {

	void BeetleBuzz::actionLoop(std::stop_token stopToken) {
		auto lastSentPrivmsgTime{ std::chrono::steady_clock::now() };
		auto privmsgCooldown{ 500ms }; //A cooldown between sending messages in chat to avoid spamming (may need adjusting).

		while (stopToken.stop_requested() == false) {

			//If there is nothing waiting in the queues, sleep for a bit.
			messageCounter.update();
			if (messageQueue.empty() 
				&& (actionQueue.empty() || messageCounter.msgCount >= messageCounter.msgLimit)
				&& (privmsgSendQueue.empty() || messageCounter.msgCount >= messageCounter.lowPriorityLimit)) {
				//std::this_thread::sleep_for(std::chrono::milliseconds(10));
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10ms);
			}
			else {
				actionQueueMutex.lock();
				//TODO Maybe change to handle all pending actions, or at least more than one.
				if (actionQueue.empty() == false) {
					Action& action = actionQueue.front();
					//handle the action.
					switch (action.type) {
					case ActionType::SEND_MSG: {
						if (messageCounter.attemptIncrementPriority() == false) {
							goto skipActionPop;
						}
						TTV::Message* msgPtr{ static_cast<TTV::Message*>(action.param) };
						sendMessage(*msgPtr);
						delete msgPtr;
						break;
					}
					case ActionType::JOIN_ROOM: {
						if (messageCounter.attemptIncrementPriority() == false) {
							goto skipActionPop;
						}
						std::string* channelName = static_cast<std::string*>(action.param);
						sendMessage(TTV::Message("", "", TTV::msgID::JOIN, *channelName, ""));
						delete channelName;
						break;
					}
					case ActionType::LEAVE_ROOM: {
						if (messageCounter.attemptIncrementPriority() == false) {
							goto skipActionPop;
						}
						std::string* channelName = static_cast<std::string*>(action.param);
						sendMessage(TTV::Message("", "", TTV::msgID::PART, *channelName, ""));
						delete channelName;
						break;
					}
					case ActionType::DISCONNECT:
						//actionQueue.pop();
						actionQueueMutex.unlock();
						goto disconnectionLabel;
						break;
					default:
						break;
					}

					actionQueue.pop();
				skipActionPop:;
				}
				actionQueueMutex.unlock();

				//Send pending private message if the cooldown allows it.
				if (!privmsgSendQueue.empty() && (std::chrono::steady_clock::now() - lastSentPrivmsgTime) >= privmsgCooldown
					&& messageCounter.attemptIncrement()) {

					sendMessage(privmsgSendQueue.front());
					privmsgSendQueue.pop();
					if (privmsgSendQueue.size() > 3u) {//Safety measure: if the queue gets too backed up, toss out old elements until it is more resonable.
						while (privmsgSendQueue.size() > 3u)
							privmsgSendQueue.pop();
					}
					lastSentPrivmsgTime = std::chrono::steady_clock::now();

				}

				//Read incomming messages.
				messageQueueMutex.lock();
				if (messageQueue.empty() == false) {
					TTV::Message& msg = messageQueue.front();

					messageRecieveCallback(msg);

					switch (msg.ID) {
					case TTV::msgID::PRIVMSG: {
						size_t commandIndex = searchTree.searchForCommand(msg.msg);
						if (commandIndex != SIZE_MAX) {
							CommandState& cmdState = commands[commandIndex].second;
							if (cmdState.cooldown < (std::chrono::steady_clock::now() - cmdState.previousCallTime)) {
								bool success = commands[commandIndex].first.commandFunction(msg, cmdState);
								cmdState.previousCallTime = std::chrono::steady_clock::now();
							}
						}
						break;
					}
					case TTV::msgID::PING:
						sendMessage(TTV::Message("", "", TTV::msgID::PONG, "", msg.msg));
						break;

					case TTV::msgID::NOTICE:
						//TODO maybe add a notice queue or actionType.
						break;
					case TTV::msgID::RECONNECT:
						//TODO a disconnect is about to happen, maybe do something or just terminate the program.
						break;
					case TTV::msgID::USERNOTICE:
						//TODO an event such as subcription has happened.
						break;
					case TTV::msgID::USERSTATE:
						//Either the bot sent a message, or a user joined the chat.
						break;
					case TTV::msgID::JOIN:
						//This bot or another user joined the channel (only gets sent if there are less than 1000 viewers). https://dev.twitch.tv/docs/irc/membership/
						//TODO maybe when the bot joins, add the joined channel to a vector of active channels.
						break;
					case TTV::msgID::PART:
						//This bot either got banned from the channel, or another user left the channel.
						break;

					default:
						break;
					}
					messageQueue.pop();
				}
				messageQueueMutex.unlock();
			}
		}
	disconnectionLabel:
		//disconnect if connected, and return to kill the thread;
		performDisconnect();
		//std::cout << "Exiting actionLoop." << std::endl;
	}

	void BeetleBuzz::messageReadLoop(std::stop_token stopToken) {
		while (stopToken.stop_requested() == false) {
			try {
				socketStream.read(inBuffer); //Blocks until a full message has been read.
			}
			catch (std::exception const& e) {
				std::cout << "Error while reading incoming message: " << e.what() << std::endl;
				//isConnected = false;
				actionThread.request_stop();
				return;
			}
			if (inBuffer.size() == 0u)
				continue;

			std::string_view dataView{ static_cast<char*>(inBuffer.data().data()), inBuffer.size() };
			
			size_t start = 0u;
			size_t end = dataView.find_first_of('\n');
			size_t msgLength = DKUtil::getTrimmedLength(dataView, start, end - start);

			messageQueueMutex.lock();
			while (start < dataView.size()) {
				if ( msgLength > 0u) {
					messageQueue.emplace(dataView.substr(start, msgLength));
					//std::cout << "Read message: " << dataView << std::endl;
				}
				if (end == std::string_view::npos)
					break;
				start = end + 1;
				end = dataView.find_first_of('\n', start);
				//Strip trailing whitespace.
				msgLength = DKUtil::getTrimmedLength(dataView, start, end - start);
			}
			messageQueueMutex.unlock();

			inBuffer.clear();
		}
		//std::cout << "Exiting messageLoop." << std::endl;
	}

	void BeetleBuzz::sendMessage(const TTV::Message& msg) {
		std::stringstream ss;
		//Tags.
		if (msg.tags.size() > 0u) {
			ss << msg.tags << ' ';
		}
		//Message ID.
		ss << TTV::msgIDToStringView(msg.ID);
		//Channel.
		if (msg.channel.size() > 0u) {
			ss << " #" << msg.channel;
		}
		//Message.
		if (msg.msg.size() > 0u) {
			ss << " :" << msg.msg;
		}
		std::string str{ ss.str() };
		auto writeBuffer{ outBuffer.prepare(str.size()) };
		memcpy(writeBuffer.data(), str.data(), str.size());
		outBuffer.commit(str.size());
		
		if (socketStream.is_open()) {
			socketStream.write(outBuffer.data());
		}
		outBuffer.clear();
	}

	bool BeetleBuzz::connect(std::string NICK, std::string PASS, std::string capabilities) {

		if (isConnected) {
			throw std::exception("Cannot connect since a connetion is already active for this instance!");
		}

		//Reset all the stuff in case this is a reconnection.
		socketStream.~stream();
		new(&socketStream) websocket::stream<beast::ssl_stream<tcp::socket>>{ ioContext, sslContext };

		while (actionQueue.empty() == false) {
			actionQueue.pop();
		}
		while (messageQueue.empty() == false) {
			messageQueue.pop();
		}
		while (privmsgSendQueue.empty() == false) {
			privmsgSendQueue.pop();
		}
		actionQueueMutex.~mutex();
		new(&actionQueueMutex) std::mutex();

		messageQueueMutex.~mutex();
		new(&messageQueueMutex) std::mutex();

		inBuffer.clear();
		outBuffer.clear();

		//Do the connecting.
#if BEETLEBUZZ_USE_DUMMYSOCKET == false
		const std::string twitchWebsocketURL = "irc-ws.chat.twitch.tv"; //"wss://irc-ws.chat.twitch.tv:443"
		const std::string port = "443";

		sslContext.set_default_verify_paths();
		const auto resolverResults = tcpResolver.resolve(twitchWebsocketURL, port);
		try{
			//connect
			endPoint = boost::asio::connect(beast::get_lowest_layer(socketStream), resolverResults);
			if (!SSL_set_tlsext_host_name(socketStream.next_layer().native_handle(), twitchWebsocketURL.c_str())) {
				throw beast::system_error(
					beast::error_code(
						static_cast<int>(::ERR_get_error()),
						boost::asio::error::get_ssl_category()
					), "Failed to set SNI Hostname");
			}

			std::string host = twitchWebsocketURL + ':' + std::to_string(endPoint.port());

			socketStream.next_layer().handshake(ssl::stream_base::client);
			// Set a decorator to change the User-Agent of the handshake
			socketStream.set_option(websocket::stream_base::decorator(
				[](websocket::request_type& req)
				{
					req.set(http::field::user_agent,
						std::string(BOOST_BEAST_VERSION_STRING) +
						" websocket-client-coro");
				}));
			socketStream.handshake(host, "/");
#endif
			socketStream.write(boost::asio::buffer("PASS " + PASS));
			socketStream.write(boost::asio::buffer("NICK " + NICK));
			messageCounter.attemptIncrement();
			messageCounter.attemptIncrement();
			socketStream.write(boost::asio::buffer(capabilities));
			messageCounter.attemptIncrement();

			//wait for connection to complete.
			/*expected messages:
			:tmi.twitch.tv 001 BotName :Welcome, GLHF!
			:tmi.twitch.tv 002 BotName :Your host is tmi.twitch.tv
			:tmi.twitch.tv 003 BotName :This server is rather new
			:tmi.twitch.tv 004 BotName :-
			:tmi.twitch.tv 375 BotName :-
			:tmi.twitch.tv 372 BotName :You are in a maze of twisty passages, all alike.
			:tmi.twitch.tv 376 BotName :>
			:tmi.twitch.tv CAP * ACK :twitch.tv/commands twitch.tv/tags
			*/
			beast::flat_buffer inBuffer;
			for (int i = 0; i < 8; ++i) {
				socketStream.read(inBuffer);
				std::string readMsg((char*)(inBuffer.data().data()), inBuffer.size());
				inBuffer.consume(inBuffer.size());

				//std::cout << readMsg << std::endl;

				std::string_view dataView{ readMsg };

				size_t start = 0u;
				size_t end = dataView.find_first_of('\n');

				while (start < dataView.size()) {
					TTV::Message msg(dataView.substr(start, end - start));

					switch (msg.ID) {
					case TTV::msgID::_001:
					case TTV::msgID::_002:
					case TTV::msgID::_003:
					case TTV::msgID::_004:
					case TTV::msgID::_375:
					case TTV::msgID::_372:
					case TTV::msgID::_376:
						//++i;
						break;
					case TTV::msgID::CAP:
						goto finishedConnecting;
					default:
						socketStream.close(websocket::close_code::normal);
						return false;
					}
					if (end == std::string_view::npos)
						break;
					start = end + 1;
					end = dataView.find_first_of('\n', start);
				}
			}
			//Malformed connection since expected messages were not recieved.
			socketStream.close(websocket::close_code::normal);
			return false;

		finishedConnecting: //Sucessfully connected.
			//Start the message handling thread and return true;
			messageThread = std::jthread([this](std::stop_token stoken) {messageReadLoop(stoken); });
			actionThread = std::jthread([this](std::stop_token stoken) {actionLoop(stoken); });

			isConnected = true;
			return true;
		}
		catch (std::exception const& e) {
			isConnected = false;
			socketStream.close(websocket::close_code::normal);
			std::cout << "Error: " << e.what() << std::endl;
			return false;
		}
	}

	void BeetleBuzz::performDisconnect() {

		try {
			if (socketStream.is_open())
				socketStream.close(websocket::close_code::normal);
		}
		catch(std::exception const& e) {
			std::cout << "Error performing disconnect: " << e.what() << std::endl;
		}
		messageThread.join();
		isConnected = false;

	}

	void BeetleBuzz::joinRoom(std::string channelName) {
		actionQueueMutex.lock();

		actionQueue.emplace(ActionType::JOIN_ROOM, new std::string(std::move(channelName)));

		actionQueueMutex.unlock();
	}

	void BeetleBuzz::leaveRoom(std::string channelName) {
		actionQueueMutex.lock();

		actionQueue.emplace(ActionType::LEAVE_ROOM, new std::string(std::move(channelName)));

		actionQueueMutex.unlock();
	}

	void BeetleBuzz::disconnect() {
		actionQueueMutex.lock();

		actionQueue.emplace(ActionType::DISCONNECT, nullptr);

		actionQueueMutex.unlock();
	}
}
