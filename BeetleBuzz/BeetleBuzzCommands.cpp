#include "BeetleBuzz.h"

namespace BeetleBuzz {

	//Calculate a hash based on the username of the caller.
	inline uint32_t usernameToSeed(const std::string_view& name) {

		const size_t remainder{ name.size() % 4u };
		const size_t endOfWholeChunks{ name.size() - remainder };

		uint32_t seed{ 0u };
		uint32_t copyTarget{ 0u };
		for (size_t i = 0u; i < endOfWholeChunks; i += 4) {
			memcpy(&copyTarget, name.data() + i, 4u); //Copy 4 char bytes into the uint32.
			seed ^= copyTarget * 6542989;
		}
		memcpy(&copyTarget, name.data() + endOfWholeChunks, remainder);
		seed ^= copyTarget;
		return seed;
	}

	bool BeetleBuzz::diceCmd(TTV::Message& input, CommandState& cmdState) {
		if (privmsgSendQueue.size() > 2u)
			return false;

		std::stringstream ss;
		ss << '@' << input.getSenderName(); //@name of caller.

		std::string_view paramString{ input.msg.substr(5u) }; //Skip "!dice".

		std::vector<uint32_t> params; //Parameters are one or more numbers separated by whitespace. Each param is an n-sided die to be rolled.

		size_t index = 0u;
		while ( index < paramString.size()) {
			uint32_t param = 0u;
			for (; index < paramString.size(); ++index) {//Skip whitespace.
				if (DKUtil::isSpace(paramString[index]) == false)
					break;
			}
			for (uint32_t digit = 0u; index < paramString.size(); ++index, ++digit) {
				if (DKUtil::isSpace(paramString[index]))
					break;
				else if (DKUtil::isDigit(paramString[index]))
					param = param * 10u + static_cast<uint32_t>(paramString[index] - '0');
				else {
					params.push_back(param == 0u ? 6u : param);
					goto endOfParams;
				}
			}
			if (param > 0u)
				params.push_back(param);
		}
	endOfParams:
		if (params.size() == 0u)
			params.push_back(6u); //If no parameters are given, default to a single d6.
		//Roll the dice.
		//uint32_t seed = static_cast<uint32_t>(time(nullptr));
		auto sysTime = std::chrono::system_clock::now();
		uint32_t seed = static_cast<uint32_t>(sysTime.time_since_epoch().count());

		for (size_t i = 0u; i < params.size(); ++i) {
			uint32_t roll = DKUtil::wellonsHash((uint32_t)i, seed);
			ss << ' ' << (roll % params[i]) + 1;
		}
		//send the response.
		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

	bool BeetleBuzz::personalElementCmd(TTV::Message& input, CommandState& cmdState) {
		if (privmsgSendQueue.size() > 2u)
			return false;

		static const std::vector<std::string_view> elements{
			"💨 Air 💨", "🌫 Mist 🌫", "🌪 Dust 🌪",
			"⛰ Earth ⛰", "⏳ Sand ⏳", "🔩 Metal 🔩",
			"🔥 Fire 🔥", "🌋 Lava 🌋", "⚡ Lightning ⚡",
			"🌊 Water 🌊", "☁ Steam ☁", "❄ Ice ❄",
			"🌌 Aether 🌌" };

		const std::string_view name{ input.getSenderName() };
		uint32_t seed{ usernameToSeed(name) };

		std::stringstream ss;
		ss << '@' << name << " Your element is: " << elements[DKUtil::wellonsHash(seed) % elements.size()];

		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

	bool BeetleBuzz::personalColorCmd(TTV::Message& input, CommandState& cmdState) {
		if (privmsgSendQueue.size() > 2u)
			return false;

		const std::string_view name{ input.getSenderName() };
		uint32_t seed{ usernameToSeed(name) * 198491317u }; //get the seed and shuffle it a bit further.

		std::stringstream ss;
		ss << '@' << name << " Your color is: " << colorList[DKUtil::wellonsHash(seed) % colorList.size()];

		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

	bool BeetleBuzz::dailyFortuneCmd(TTV::Message& input, CommandState& cmdState) {
		if (privmsgSendQueue.size() > 2u)
			return false;

		//pick a fortune based on the date and the username.
		const std::string_view name{ input.getSenderName() };
		uint32_t seed{ usernameToSeed(name) * 1337u }; //get the seed and shuffle it a bit further.

		//Current time rounded down to the day, giving a single unique number for the entire day.
		uint32_t day = std::chrono::floor<std::chrono::days>(std::chrono::steady_clock::now()).time_since_epoch().count(); 

		std::stringstream ss;
		ss << '@' << name << " Your daily fortune is: " << fortuneList[DKUtil::wellonsHash(seed, day) % fortuneList.size()];

		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

	bool BeetleBuzz::pickOneCmd(TTV::Message& input, CommandState& cmdState) {
		if (privmsgSendQueue.size() > 2u)
			return false;

		//Pick one of the options in the input. Options are separated by '|' characters.
		size_t start{ 8u }; //skip "!pickone"
		size_t end{ input.msg.find_first_of('|', start) };
		if (end == input.msg.npos)
			return false; //If there are no '|' in the string, the call fails.

		std::vector<std::string_view> options;
		options.push_back(input.msg.substr(start, end - start));

		for (start = end + 1; start < input.msg.size(); start = end + 1) {
			end = input.msg.find_first_of('|', start);
			options.push_back(input.msg.substr(start, end - start));

			if (end == input.msg.npos)
				break;
		}

		if (options.size() < 2)
			return false;

		uint32_t seed = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());

		std::stringstream ss;
		ss << '@' << input.getSenderName(); //@username
		ss << " Option picked: " << options[DKUtil::wellonsHash(seed) % options.size()];

		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

	bool BeetleBuzz::personalAnimalCommand(TTV::Message& input, CommandState& cmdState) {

		//🐶🐺🐱🦁🐯🦒🦊🦝🐗🐭🐹🐻🐨🐼🐸🐴🦄🐔
		//🐒🦍🦧🐆🐎🦌🦏🦛🐂🐃🐄🐖🐏🐑🐐🐪🐫🦙🦘🦥🦨🦡🐘🐁🐀🦔🐇🐿🦎🐊🐢🐍🐉🦕🦖🦦🦈🐬🐳🐟🐠🐡🦐🦑🐙🦞🦀🦆🐓🦃🦅🕊🦢🦜🦩🦚🦉🐦🐧🦇🦋🐌🐛🦟🦗🐜🐝🐞🦂🕷👽
		//Pick an animal emoji based on username. 
		std::vector<std::string> animals{ "🐶","🐺","🐱","🦁","🐯","🦒","🦊","🦝","🐗","🐭","🐹","🐻","🐨","🐼","🐸","🐴","🦄","🐔",
			"🐒", "🦍", "🦧", "🐆", "🐎", "🦌", "🦏", "🦛", "🐂", "🐃", "🐄", "🐖", "🐏", "🐑", "🐐", "🐪", "🐫", "🦙", "🦘", "🦥", "🦨", "🦡", "🐘", "🐁", "🐀", "🦔", "🐇", "🐿",
			"🦎", "🐊", "🐢", "🐍", "🐉", "🦕", "🦖", "🦦", "🦈", "🐬", "🐳", "🐟", "🐠", "🐡", "🦐", "🦑", "🐙", "🦞", "🦀", "🦆", "🐓", "🦃", "🦅", "🕊", "🦢", "🦜", "🦩", "🦚", "🦉", "🐦", "🐧",
			"🦇", "🦋", "🐌", "🐛", "🦟", "🦗", "🐜", "🐝", "🐞", "🦂", "🕷", "👽" 
		};

		const std::string_view name{ input.getSenderName() };
		uint32_t seed{ usernameToSeed(name) * 4141u };

		std::stringstream ss;
		ss << '@' << name << " Your animal is: " << animals[DKUtil::wellonsHash(seed) % animals.size()];

		//actionQueue.emplace(ActionType::SEND_MSG, new TTV::Message("", "", TTV::msgID::PRIVMSG, input.channel, ss.str()));
		privmsgSendQueue.emplace("", "", TTV::msgID::PRIVMSG, input.channel, ss.str());

		return true;
	}

}