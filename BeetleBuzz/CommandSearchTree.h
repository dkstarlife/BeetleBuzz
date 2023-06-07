#pragma once

#include <string>
#include <memory_resource>
#include <functional>
#include <climits> //for numeric limits.

#include "Message.h"
#include "DKUtil.h"

namespace BeetleBuzz {

	class CommandSearchTree {
		std::pmr::monotonic_buffer_resource memBuffer;
		std::pmr::unsynchronized_pool_resource memPool{ &memBuffer };

		template<class objectType>
		[[nodiscard]] void* allocMem() {
			return memPool.allocate(sizeof(objectType), alignof(objectType));
		}

		struct Node {
			std::string_view substring;
			std::pmr::vector<Node*> children;
			Node* parent{ nullptr };
			size_t commandIndex{ SIZE_MAX };

			Node(std::string_view str, std::pmr::vector<Node*> childNodes, Node* parentNode, size_t commandIndex, std::pmr::unsynchronized_pool_resource& poolResource)
				: substring{ str }, children{ childNodes, &poolResource }, parent{ parentNode }, commandIndex{ commandIndex } {}

			//Creates a new Node that is made up of the chars from charIndex to end, and moves all the children to that Node. Also sets this Node's substring to the chars before the charIndex.
			void splitNode(size_t charIndex, std::pmr::unsynchronized_pool_resource& poolResource);

			//Returns the index of the first non-matching character.
			size_t match(std::string_view str);

		};

		std::vector<std::string> commands;

		Node rootNode{ "", {}, nullptr, SIZE_MAX, memPool };

		void addCommandNode(size_t index);
		void buildTree();

		void addNodeToStringStream(const Node& node, std::stringstream& ss, uint32_t tabDepth);

	public:

		CommandSearchTree() {};

		CommandSearchTree(std::vector<std::string> listOfCommands) : commands{ listOfCommands } {
			buildTree();
		}

		//Looks through the recorded commands to see if any of them are prefixes to the text string, and returns the index of longest match. Otherwise returns SIZE_MAX if no command is a prefix to the text.
		size_t searchForCommand(const std::string_view& text);

		std::string toString() {
			std::stringstream ss;
			addNodeToStringStream(rootNode, ss, 0u);
			return ss.str();
		};

	};
}