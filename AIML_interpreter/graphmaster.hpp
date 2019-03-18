#pragma once

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <list>

#include "category.hpp"
#include "bot.hpp"

namespace AIML
{
	namespace fs = std::experimental::filesystem;
	using std::unordered_map;
	using std::unordered_set;
	using std::string;
	using std::queue;
	using std::list;

	//	Node Types in priority order
	enum NodeType {
		PWord = 0,	// $Word
		PZero = 1,	// #0+ Wildcard
		POne = 2,	// _1+ Wildcard
		Word = 3,	// WORD
		Zero = 4,	// ^0+ Wildcard
		One = 5,	// *1+ Wildcard
		//	Not involved in matching
		ROOT = 6,	//	Root Node
	};

	struct Node {
		unordered_map<string, Node> Branches;
		bool EndPoint;
		Node* _Parent;
		NodeType _NodeType;

		Node(Node* Parent = nullptr, NodeType NType = NodeType::ROOT) : _Parent(Parent), EndPoint(false), _NodeType(NType) {}
		
		//	Process a token, removing it from the queue before passing deeper
		void ProcessTokens(queue<string> &Tokens) {
			if (Tokens.empty()) {
				EndPoint = true;
				//	End of tokens
				//	Store <template> in this node
				std::cout << "End Branch" << std::endl;
			}
			else {
				auto Token = Tokens.front();
				NodeType NType = NodeType::Word;
				switch (Token.at(0)) {
				case '$': NType = NodeType::PWord; break;
				case '#': NType = NodeType::PZero; break;
				case '_': NType = NodeType::POne; break;
				case '^': NType = NodeType::Zero; break;
				case '*': NType = NodeType::One; break;
				default: break;
				}
				std::cout << Token << "(" << NType << ")->";
				Tokens.pop();
				Branches.emplace(Token, Node(this, NType)).first->second.ProcessTokens(Tokens);
			}
		}

		//	Traverse the graph searching for a pattern
		const bool FindTokens(list<string>::iterator Tokens, list<string>::iterator End) {
			if (Tokens == End) {
				//	#	0 or more
				auto Branch = Branches.find("#");
				if (Branch != Branches.end()) {
					std::cout << "TRAILING--> #" << std::endl;
					//	Don't increment 'Tokens' on a 0+ wildcard
					if (Branch->second.FindTokens(Tokens, End)) { return true; }
				}
				//	^ 0 or more
				Branch = Branches.find("^");
				if (Branch != Branches.end()) {
					std::cout << "TRAILING--> ^" << std::endl;
					//	Don't increment 'Tokens' on a 0+ wildcard
					if (Branch->second.FindTokens(Tokens, End)) { return true; }
				}
				if (EndPoint) {
					//	Return <template> from this node
					std::cout << "PATTERN MATCH" << std::endl;
					return true;
				}
				else {
					//	Incomplete pattern
					std::cout << "PATTERN INCOMPLETE" << std::endl;
					return false;
				}
			} else {
				//	$	Priority
				auto Branch = Branches.find("$" + *Tokens);
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> $" << std::endl;
					if (Branch->second.FindTokens(++Tokens, End)) {	return true; }
					else { --Tokens; }
				}
				//	#	0 or more
				Branch = Branches.find("#");
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> #" << std::endl;
					//	Treat 0+ wildcards as if they match 0 words by not incrementing 'Tokens'
					if (Branch->second.FindTokens(Tokens, End)) { return true; }
				}
				//	_ 	1 or more
				Branch = Branches.find("_");
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> _" << std::endl;
					if (Branch->second.FindTokens(++Tokens, End)) {	return true; }
					else { --Tokens; }
				}
				//	word
				Branch = Branches.find(*Tokens);
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> WORD" << std::endl;
					if (Branch->second.FindTokens(++Tokens, End)) {	return true; }
					else { --Tokens; }
				}
				//	^ 0 or more
				Branch = Branches.find("^");
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> ^" << std::endl;
					//	Treat 0+ wildcards as if they match 0 words by not incrementing 'Tokens'
					if (Branch->second.FindTokens(Tokens, End)) { return true; }
				}
				//	* 1 or more
				Branch = Branches.find("*");
				if (Branch != Branches.end()) {
					std::cout << "[" << *Tokens << "]--> *" << std::endl;
					if (Branch->second.FindTokens(++Tokens, End)) {	return true; }
					else { --Tokens; }
				}
				//	No direct node match; Rematch current node if wildcard
				if (_NodeType == NodeType::PZero) {
					//	Increment 'Tokens' and rematch this node
					std::cout << "(" << *Tokens << ") # +" << std::endl;
					if (FindTokens(++Tokens, End)) { return true; }
					else { --Tokens; }
				}
				else if (_NodeType == NodeType::POne) {
					//	Increment 'Tokens' and rematch this node
					std::cout << "(" << *Tokens << ") _ +" << std::endl;
					if (FindTokens(++Tokens, End)) { return true; }
					else { --Tokens; }
				}
				else if (_NodeType == NodeType::Zero) {
					//	Increment 'Tokens' and rematch this node
					std::cout << "(" << *Tokens << ") ^ +" << std::endl;
					if (FindTokens(++Tokens, End)) { return true; }
					else { --Tokens; }
				}
				else if (_NodeType == NodeType::One) {
					//	Increment 'Tokens' and rematch this node
					std::cout << "(" << *Tokens << ") * +" << std::endl;
					if (FindTokens(++Tokens, End)) { return true; }
					else { --Tokens; }
				}
				if (_Parent) {
					std::cout << "<--[" << *Tokens << "] NODE DEPLEATED" << std::endl;
				}
				else {
					std::cout << "[" << *Tokens << "] NO MATCH AVAILABLE" << std::endl;
				}
				return false;
			}
		}

		unsigned int BranchCount() {
			unsigned int iBranches = (!_Parent ? 0:1);
			for (auto Branch : Branches) {
				iBranches += Branch.second.BranchCount();
			}
			return iBranches;
		}

		unsigned int PatternCount() {
			unsigned int iPatterns = EndPoint;
			for (auto Branch : Branches) {
				iPatterns += Branch.second.PatternCount();
			}
			return iPatterns;
		}
	};

	class GraphMaster {
		std::queue<rapidxml::file<>> XML;
		std::queue<rapidxml::xml_document<>> Documents;

		Node RootNode;						//	Root node for all pattern branches
		unordered_set<string> Words;		//	Unique set of words across all branches
		std::vector<Category> Categories;	//	All graphed categories (templates)

		std::unordered_map<std::string, std::string*> DVariables;	// Current stored variables
		std::array<std::string*, 8> DStars; // Current <star/> inputs

		//	Tokenize an input string
		//	Tokens are converted to UPPER CASE
		//	ToDo: Punctuation and contractions are stripped
		queue<string> TokenQueue(string Input)
		{
			queue<string> Tokens;
			for (auto& x : Input) {
				x = toupper(x);
			}
			string token;
			std::istringstream tokenStream(Input);
			while (getline(tokenStream, token, ' '))
			{
				auto tmp = (*Words.insert(token).first);
				Tokens.push(tmp);
			}
			return Tokens;
		}
		list<string> TokenList(string Input)
		{
			list<string> Tokens;
			for (auto& x : Input) {
				x = toupper(x);
			}
			string token;
			std::istringstream tokenStream(Input);
			while (getline(tokenStream, token, ' '))
			{
				auto tmp = (*Words.insert(token).first);
				Tokens.push_back(tmp);
			}
			return Tokens;
		}

		void ParseCategories(rapidxml::xml_node<>* Root_Node, std::string UseTopic = "") {
			for (auto Category_Node = Root_Node->first_node("category"); Category_Node; Category_Node = Category_Node->next_sibling())
			{
				//	Dissect PATTERN
				auto Pattern_Node = Category_Node->first_node("pattern");
				if (Pattern_Node == NULL) {
					std::cout << "Skipping Malformed Category: No PATTERN" << std::endl;
					continue;
				}
				//	Dissect TEMPLATE
				auto Template_Node = Category_Node->first_node("template");
				if (Template_Node == NULL) {
					std::cout << "Skipping Malformed Category: No TEMPLATE" << std::endl;
					continue;
				}
				//	Start constructing a new CATEGORY
				Categories.emplace_back(Category(Template_Node, &DVariables, &DStars));

				//	Dissect THAT
				/*auto That_Node = Category_Node->first_node("that");
				if (That_Node != NULL) {
					std::string That = That_Node->value();
					Category_List.back().SetThat(That);
				}*/

				//	Create a pattern and add this category to it
				//	Tokenize our input pattern
				auto Tokens = TokenQueue(Pattern_Node->value());
				//	Pass our tokens into the RootNode
				RootNode.ProcessTokens(Tokens);
			}
		}

	public:

		GraphMaster() : RootNode(), DStars({ new string("*1*"),new string("*2*"),new string("*3*"),new string("*4*"),new string("*5*"),new string("*6*"),new string("*7*"),new string("*8*") }) {
			fs::path AIML_Folder = fs::current_path().append("AIML");
			std::cout << AIML_Folder << std::endl;
			if (fs::exists(AIML_Folder) && fs::is_directory(AIML_Folder)) {
				for (const auto& entry : fs::directory_iterator(AIML_Folder))
				{
					auto filename = entry.path().filename();
					if (fs::is_directory(entry.status()))
					{
						std::cout << "Unable to load nested folders (yet)" << std::endl;
					}
					else if (fs::is_regular_file(entry.status())) {
						std::cout << "Loading (" << filename << ").." << std::endl;
						XML.emplace(entry.path().string().c_str());
						Documents.emplace();
						try {
							//	Parse the raw XML data into an XML Document object
							Documents.back().parse<rapidxml::parse_default | rapidxml::parse_normalize_whitespace>(XML.back().data());
							//	Iterate through the XML extracting C++ objects from the AIML
							rapidxml::xml_node<>* Root_Node = Documents.back().first_node("aiml");
							for (rapidxml::xml_node<>* Topic_Node = Root_Node->first_node("topic"); Topic_Node; Topic_Node = Topic_Node->next_sibling())
							{
								if (strcmp(Topic_Node->name(), "topic") == 0) {
									auto Topic_Name = Topic_Node->first_attribute("name");
									if (Topic_Name == NULL) {
										std::cout << "Skipping Malformed Topic: No NAME" << std::endl;
										continue;
									}
									ParseCategories(Topic_Node, Topic_Name->value());
								}
							}
							ParseCategories(Root_Node);
						}
						catch (const rapidxml::parse_error & e) {
							std::cout << "\tXML Error: " << e.what() << " - " << e.where<char>() << std::endl;
						}
					}
				}
			}
			PrintDebug();
		}

		~GraphMaster() {
			while (!Documents.empty()) {
				Documents.pop();
			}
			while (!XML.empty()) {
				XML.pop();
			}
			for (auto Obj : DStars) {
				delete Obj;
			}
		}

		//	Try to match a pattern from the graph
		//	Uses a bots 'Variable' and 'Star' list
		void MatchPattern(string Pattern, std::unordered_map<std::string, std::string*>* _Variables, std::array<std::string*, 8>* _Stars)
		{
			//	Tokenize our input pattern
			auto Tokens = TokenList(Pattern);
			//	Pass our tokens into the RootNode
			RootNode.FindTokens(Tokens.begin(), Tokens.end());
		}
		
		void DebugCategories(std::unordered_map<std::string, std::string*>* _Variables, std::array<std::string*, 8>* _Stars) {
			for (auto Category : Categories) {
				Category.PrintData(_Variables, _Stars);
			}
		}

		void PrintDebug() {
			std::cout << "Unique Words:\t" << Words.size() << std::endl;
			std::cout << "Total Branches:\t" << RootNode.BranchCount() << std::endl;
			std::cout << "Total Patterns:\t" << RootNode.PatternCount() << std::endl;
		}

		//	Creates a new AIML::Bot
		//	Must be destroyed BEFORE the AIML::GraphMaster
		Bot CreateBot() {
			return Bot(this);
		}

	};
}