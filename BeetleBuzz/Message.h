#pragma once

#include <string>
#include <vector>

namespace TTV {

	enum class msgID {
		UNKNOWN,
		CAP,
		CLEARCHAT,
		CLEARMSG,
		GLOBALUSERSTATE,
		HOSTTARGET,
		JOIN,
		NICK,
		NOTICE,
		PART,
		PASS,
		PING,
		PONG,
		PRIVMSG,
		RECONNECT,
		ROOMSTATE,
		USERNOTICE,
		USERSTATE,
		WHISPER,
		_001, //Logged in successfully.
		_002, //part of log in msg.
		_003, //part of log in msg.
		_004, //part of log in msg.
		_353, //List of users
		_366, //End of list of users.
		_372, //part of log in msg.
		_375, //part of log in msg.
		_376, //part of log in msg.
		_421 //Unsupported IRC command.
	};

	static constexpr std::string_view msgIDToStringView(msgID id) {
		using enum msgID;
		switch (id) {
		case CAP: return "CAP";
		case CLEARCHAT: return "CLEARCHAT";
		case CLEARMSG: return "CLEARMSG";
		case GLOBALUSERSTATE: return "GLOBALUSERSTATE";
		case HOSTTARGET: return "HOSTTARGET";
		case JOIN: return "JOIN";
		case NICK: return "NICK";
		case NOTICE: return "NOTICE";
		case PART: return "PART";
		case PING: return "PING";
		case PONG: return "PONG";
		case PRIVMSG: return "PRIVMSG";
		case RECONNECT: return "RECONNECT";
		case ROOMSTATE: return "ROOMSTATE";
		case USERNOTICE: return "USERNOTICE";
		case USERSTATE: return "USERSTATE";
		case WHISPER: return "WHISPER";
		case UNKNOWN: return "UNKNOWN";
		default: return "NUMERIC_ID";
		}
	}

	static constexpr msgID stringViewToMsgID(std::string_view str) {
		using enum msgID;
		switch (str[0]) {
		case 'P': 
			switch (str[1]) {
			case 'R': return PRIVMSG;
			case 'I': return PING;
			case 'O': return PONG;
			case 'A': return PART;
			default: return UNKNOWN;
			}
		case 'N':
			switch (str[1]) {
			case 'O':return NOTICE;
			case 'I': return NICK;
			default: return UNKNOWN;
			}
		case 'U':
			if (str.length() < 5) return UNKNOWN;
			switch (str[4]) {
			case 'N': return USERNOTICE;
			case 'S': return USERSTATE;
			default: return UNKNOWN;
			}
		case 'J': 
			return JOIN;
		case 'R':
			switch (str[1]) {
			case 'O': return ROOMSTATE;
			case 'E': return RECONNECT;
			default: return UNKNOWN;
			}
		case 'G': return GLOBALUSERSTATE;
		case 'W': return WHISPER;
		case 'H': return HOSTTARGET;
		case 'C':
			if (str.length() < 6) return CAP;
			switch (str[5]) {
			case 'M': return CLEARMSG;
			case 'C': return CLEARCHAT;
			default: return UNKNOWN;
			}
		case '0':
			switch (str[2]) {
			case '1': return _001;
			case '2': return _002;
			case '3': return _003;
			case '4': return _004;
			default: return UNKNOWN;
			}
		case '3':
			switch (str[1]) {
			case '5': return _353;
			case '6': return _366;
			case '7':
				switch (str[2]) {
				case '2': return _372;
				case '5': return _375;
				case '6': return _376;
				}
			default: return UNKNOWN;
			}
		case '4': return _421;
		default: return UNKNOWN;
		}
	}

	class Message {
		char* rawData;
	public:
		msgID ID = msgID::UNKNOWN;
		std::string_view source;
		std::string_view channel;
		std::string_view tags;
		std::string_view msg;

		Message(std::string_view _tags, std::string_view _source, msgID _ID, std::string_view _channel, std::string_view _msg){
			rawData = new char[_tags.size() + _source.size() + _channel.size() + _msg.size()];
			ID = _ID;
			
			memcpy(rawData, _tags.data(), _tags.size());
			tags = std::string_view(rawData, _tags.size());
			size_t offset = _tags.size();

			memcpy(rawData + offset, _source.data(), _source.size());
			source = std::string_view(rawData+offset, _source.size());
			offset += _source.size();

			memcpy(rawData + offset, _channel.data(), _channel.size());
			channel = std::string_view(rawData + offset, _channel.size());
			offset += _channel.size();

			memcpy(rawData + offset, _msg.data(), _msg.size());
			msg = std::string_view(rawData + offset, _msg.size());
		}

		Message(const std::string_view _rawMsg) : rawData{ new char[_rawMsg.length()] }{

			memcpy((void*)rawData, _rawMsg.data(), _rawMsg.length());
			const std::string_view rawMsg{ rawData, _rawMsg.length() };

			size_t i = 0U;
			size_t start, end;
			//Get the tags.
			if (rawMsg.length() > 1 && rawMsg[i] == '@') {
				++i;
				start = i;
				end = rawMsg.find_first_of(' ', start);
				if (end != std::string_view::npos) {
					tags = rawMsg.substr(start, end-start);
					i = end + 1;
				}
			}
			//Get source.
			if (rawMsg.length() > i && rawMsg[i] == ':') {
				++i;
				start = i;
				end = rawMsg.find_first_of(' ', start);
				if (end != std::string_view::npos) {
					source = rawMsg.substr(start, end-start);
					i = end + 1;
				}
			}
			//Get msgID/command.
			start = i;
			while (i < rawMsg.length()) {
				if (rawMsg[i] == ' ') {
					break;
				}
				++i;
			}
			if(i > start)
				ID = stringViewToMsgID(rawMsg.substr(start, i - start));
			++i;
			//Get channel.
			start = ++i; //++i to skip the '#' at the start of the channel name.
			while (i < rawMsg.length()) {
				if (rawMsg[i] == ':') {
					--i;
					break;
				}
				++i;
			}
			if(i > start)
				channel = rawMsg.substr(start, i - start);
			++i;
			++i;
			//Get message/parameters.
			if (i < rawMsg.length())
				msg = rawMsg.substr(i);//copy rest of msg.
		}

		[[nodiscard]]
		std::string_view getSenderName() const {
			return source.substr(0u, source.find_first_of('!'));
		}

		~Message() {
			delete[] rawData;
		}

	};

}