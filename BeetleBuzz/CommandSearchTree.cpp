#include "CommandSearchTree.h"


namespace BeetleBuzz {

	//Creates a new Node that is made up of the chars from charIndex to end, and moves all the children to that Node. Also sets this Node's substring to the chars before the charIndex.
	void CommandSearchTree::Node::splitNode(size_t charIndex, std::pmr::unsynchronized_pool_resource& poolResource) {
		//std::cout << "splitting node: " << substring << " at " << charIndex;

		void* mem = poolResource.allocate(sizeof(Node), alignof(Node));
		Node* newNode = new(mem) Node{ substring.substr(charIndex), std::move(children), this, commandIndex, poolResource };

		commandIndex = SIZE_MAX;
		//children.clear();
		children = std::pmr::vector<Node*>(&poolResource);
		children.push_back(newNode);
		for (Node* ptr : newNode->children) {
			ptr->parent = newNode;
		}

		substring = substring.substr(0u, charIndex);

		//std::cout << " into: " << substring << " | " << newNode->substring << std::endl;
	}

	//Returns the index of the first non-matching character.
	size_t CommandSearchTree::Node::match(std::string_view str) {
		//std::cout << "Matching str: " << str << " with node: " << substring;
		size_t shorterLength = std::min(str.length(), substring.length());
		size_t i = 0u;
		for (; i < shorterLength; ++i) {
			if (DKUtil::toLower(str[i]) != substring[i]) {
				//std::cout << " -> charIndex: " << i << " char: " << str[i] << std::endl;
				return i;
			}
		}
		if (i == substring.length()) {
			//std::cout << " -> charIndex: " << substring.length() << " char: END_OF_NODE" << std::endl;
			return substring.length();
		}
		else if (i == str.length()) {
			//std::cout << " -> charIndex: " << substring.length() << " char: " << str[i] << std::endl;
			return i;
		}
		//std::cout << " -> charIndex: " << 0u << " char: NO_MATCH" << std::endl;
		return 0u;
	}

	void CommandSearchTree::addNodeToStringStream(const Node& node, std::stringstream& ss, uint32_t tabDepth) {
		for (uint32_t i = tabDepth; i != 0; --i)
			ss << '\t';
		ss << " {" << node.substring << "->";
		if (node.commandIndex != SIZE_MAX) {
			ss << commands[node.commandIndex];
		}
		ss << '\n';

		for (Node* ptr : node.children) {
			addNodeToStringStream(*ptr, ss, tabDepth + 1);
		}
		for (uint32_t i = tabDepth; i != 0; --i)
			ss << '\t';
		ss << "}\n";
	}

	void CommandSearchTree::addCommandNode(size_t index) {
		Node* currentNode = &rootNode;
		std::string_view cmdStr = commands[index];

		for (size_t i = 0u; i < cmdStr.size(); ++i) { //Convert the command string to lower case.
			commands[index][i] = DKUtil::toLower(commands[index][i]);
		}

		for (size_t nodeIndex = 0u; nodeIndex < currentNode->children.size(); ) {
			Node* nextNode = currentNode->children[nodeIndex];
			size_t charIndex = nextNode->match(cmdStr);

			if (charIndex == 0) {//Nothing matched, go next.
				//std::cout << "No match, next node.\n";
				++nodeIndex;
				continue;
			}
			else if (charIndex == nextNode->substring.length()) {//Matched with entire node substring.
				if (charIndex == cmdStr.length()) {//Full match for the command has been found.
					//std::cout << "Full match, setting command.\n";
					//nextNode->command = &command;
					nextNode->commandIndex = index;
					return;
				}
				//Still more command to go through, go deeper.
				//std::cout << "Matched substring, go deeper.\n";
				currentNode = nextNode;
				nodeIndex = 0u;
				cmdStr = cmdStr.substr(charIndex);
				continue;
			}
			else {//Partial match, split and add a new child node.
				//std::cout << "Partial match, splitting.\n";
				nextNode->splitNode(charIndex, memPool);
				if (charIndex == cmdStr.length()) { //After split, if the node is now a perfect match.
					//nextNode->command = &command;
					nextNode->commandIndex = index;
				}
				else { //If there is more command left to process.
					nextNode->children.push_back(new(allocMem<Node>()) Node{ cmdStr.substr(charIndex), {}, nextNode, index , memPool });
				}
				return;
			}
		}
		//No matching leaf node, add one.
		//std::cout << "No matches, add child node: " << cmdStr << " to: " << currentNode->substring << '\n';
		currentNode->children.push_back(new(allocMem<Node>()) Node{ cmdStr, {}, currentNode, index, memPool });

	}

	void CommandSearchTree::buildTree() {
		for(size_t i = 0u; i < commands.size(); ++i){
			addCommandNode(i);
		}
	}

	//Looks through the recorded commands to see if any of them are prefixes to the text string, and returns the index of longest match.
	size_t CommandSearchTree::searchForCommand(const std::string_view& text) {
		Node* currentNode = &rootNode;
		size_t returnCommand{ SIZE_MAX };
		std::string_view workingStr{text};

		for (size_t nodeIndex = 0u; nodeIndex < currentNode->children.size(); ) {
			Node* nextNode = currentNode->children[nodeIndex];
			size_t charIndex = nextNode->match(workingStr);
			//std::cout << "matching: " << workingStr << " and " << nextNode->substring << '\n';

			if (charIndex == 0) {//Nothing matched.
				//std::cout << "no match\n";
				++nodeIndex;
				continue;
			}
			else if (charIndex == nextNode->substring.length()) {//The entire node's substring matches.
				//std::cout << "Full match\n";
				if (nextNode->commandIndex != SIZE_MAX) {
					returnCommand = nextNode->commandIndex;
				}
				currentNode = nextNode;
				nodeIndex = 0u;
				workingStr = workingStr.substr(charIndex);
				continue;
			}
			else {//partial matches mean that we have gone too deep in the tree.
				//std::cout << "Partial match\n";
				break;
			}
		}

		return returnCommand;
	}

}
