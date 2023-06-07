#pragma once

#include <chrono>
#include <functional>

#include "Message.h"

namespace BeetleBuzz {
	enum class ActionType {
		SEND_MSG, //param is ptr to a TTV::Message.
		JOIN_ROOM, //param is ptr to std::string.
		LEAVE_ROOM, //param is ptr to std::string.
		DISCONNECT //param is nullptr.
	};
	struct Action {
		ActionType type;
		void* param;
	};

	typedef uint32_t UserTypeFlags;
	enum class UserType : UserTypeFlags {
		PLEB = 0u,
		CREATOR = 1u,
		FOLLOWER = 2u,
		MOD = 4u,
		VIP = 8u
	};
	/*UserTypeFlags operator| (UserType lhs, UserType rhs) {
		return UserTypeFlags(lhs) | UserTypeFlags(rhs);
	}
	UserTypeFlags operator| (UserTypeFlags lhs, UserType rhs) {
		return lhs | UserTypeFlags(rhs);
	}
	UserTypeFlags operator& (UserType lhs, UserType rhs) {
		return UserTypeFlags(lhs) & UserTypeFlags(rhs);
	}
	UserTypeFlags operator& (UserTypeFlags lhs, UserType rhs) {
		return lhs & UserTypeFlags(rhs);
	}*/

	struct CommandState {
		std::chrono::steady_clock::time_point previousCallTime{ std::chrono::seconds(0u) }; //Time point that the command was last called at.
		std::chrono::steady_clock::duration cooldown; //Minimum time between calls of the command.
		uint64_t stateFlags; //Command specific flags for persistence.

		CommandState(std::chrono::steady_clock::duration cmdCooldown, uint64_t initialFlags = 0u) : cooldown{ cmdCooldown }, stateFlags{ initialFlags }{}
	};

	struct ChatCommand {
		std::string command; //String that is matched against when checking for commands in messages. Short string optimization is supposedly up to 22 chars, so best to keep commands under this length.
		std::function<bool(TTV::Message&, CommandState&)> commandFunction; //Callback function.
		//std::function<bool(TTV::Message&)> commandFunction; //Callback function.

		ChatCommand(std::string commandString, std::function<bool(TTV::Message&, CommandState&)> commandCallbackFunction = nullptr)
		//ChatCommand(std::string commandString, std::function<bool(TTV::Message&)> commandCallbackFunction = nullptr)
			: command{ commandString }, commandFunction{ commandCallbackFunction } {};

		/*friend bool operator< (const ChatCommand& lhs, const ChatCommand& rhs) {
			return lhs.command < rhs.command;
		}*/
		friend bool operator< (const ChatCommand& lhs, const ChatCommand& rhs) {
			size_t shorterLength = std::min(lhs.command.length(), rhs.command.length());

			//first mismatching char determines whether it is less than.
			for (size_t i = 0u; i < shorterLength; ++i) {
				if (lhs.command[i] != rhs.command[i]) {
					return lhs.command[i] < rhs.command[i];
				}
			}

			//if the first <shorterLength> charaters are equal, then the longer string is "less than". This will place the longer string above the shorter one when sorting.
			return rhs.command.length() == shorterLength;
		};
	};
}