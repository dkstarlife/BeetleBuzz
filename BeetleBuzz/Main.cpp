
#define BEETLEBUZZ_USE_DUMMYSOCKET false
#include "BeetleBuzz.h"

void runBeetleBuzz() {
	BeetleBuzz::BeetleBuzz bb(
		[](TTV::Message& msg) {
			std::cout << TTV::msgIDToStringView(msg.ID) << " #" << msg.channel << ' ' << msg.getSenderName() << ": " << msg.msg << std::endl;
		});

	std::cout << "BeetleBuzz Booted\n~~~~~~~~~~~~~~~~\nPlease enter Twitch authentication info in the form of:\noauth:xxxxxxxxxxxxxxxxx username\n" << std::endl;
	while (false == GetAsyncKeyState(VK_ESCAPE)) {
		std::string name, pass;
		std::string temp;
		std::cin >> std::ws;
		std::getline(std::cin, temp);
		pass = temp.substr(0u, temp.find_first_of(' '));
		name = temp.substr(pass.size() + 1); //skip the space.
		//std::cout << temp << " NICK " << name << " PASS " << pass << std::endl;
		//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //empty the in stream.

		if (true == bb.connect(name, pass)) {
			std::cout << "\n~~~ Authentiication successful. ~~~\n- To join a chat room use: JOIN channelname\n- to leave a chat room use: PART channelname\n- To disconnect use: LEAVE.\n" << std::endl;
			break;
		}
		std::cout << "ERROR: Authentication failed.\nTry again:\n" << std::endl;
	}

	//input loop
	while (false == GetAsyncKeyState(VK_ESCAPE)) {
		std::string input;
		//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		std::cin >> std::ws;
		std::getline(std::cin, input);
		if (input.size() < 5)
			continue;

		std::string_view cmd{ input.data(), 5u };
		if ("JOIN " == cmd) {
			std::cout << "Joining room: " << input.substr(5u) << std::endl;
			bb.joinRoom(input.substr(5u));
		}
		else if ("PART " == cmd) {
			std::cout << "Leaving room: " << input.substr(5u) << std::endl;
			bb.leaveRoom(input.substr(5u));
		}
		else if ("LEAVE" == cmd) {
			std::cout << "Disconneting from Twitch..." << std::endl;
			bb.disconnect();
			break;
		}
	}
	//Wait for disconnection to complete.
	while (bb.isConnected) {
		std::cout << "..." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	std::cout << "Disconnetion completed." << std::endl;

}

int main() {
	runBeetleBuzz();
}
