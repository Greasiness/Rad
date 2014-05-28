#ifndef _GAMESTATE_H_
#define _GAMESTATE_H_
#include "rapidjson\stringbuffer.h"
#include "rapidjson\filestream.h";
#include "rapidjson\prettywriter.h";
#include "rapidjson\rapidjson.h";
#include "rapidjson\reader.h";
#include "rapidjson\writer.h";
#include "Object.h"
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "rapidjson\document.h"
class gameState
{
public:
	gameState();
	~gameState();
	void addObject(Object*);
	void removeObject(int);
	std::vector<Object*> getObjectByType(std::string);
	std::string getJSONStringFull();
	void parseJSONString(std::string);
	void posChanged(int);
	void camChanged(int);
	void shootChanged(int);
	void healthChanged(int);
	Object* getObject(int);
	//for testing
	std::string getPosString(std::vector<std::pair<string, mat4>>*);
	std::vector<std::pair<string, mat4> >* parsePosString(std::string);
	int getRecentlyAdded();

private:
	std::vector<Object*>* objects;
	std::vector<std::pair<int, std::string> >* commands;
	std::vector<int> openIndices;
	int recentlyAdded;
};
gameState::gameState()
{
	objects = new std::vector<Object*>;
	commands = new std::vector<std::pair<int, string>>;
}
gameState::~gameState()
{
}

Object* gameState::getObject(int i){
	return objects->at(i);
}

int gameState::getRecentlyAdded(){
	return recentlyAdded;
}
void gameState::posChanged(int in){
	int exists = -1;
	for (int i = 0; i < commands->size(); ++i){
		if (commands->at(i).first == in){
			exists = i;
			break;
		}
	}
	if (exists == -1){
		commands->push_back(std::make_pair(in, "p"));
	}
	else {
		if (commands->at(exists).second.find("p") == std::string::npos)
			commands->at(exists).second.append("p");
	}
}
void gameState::camChanged(int in){
	int exists = -1;
	for (int i = 0; i < commands->size(); ++i){
		if (commands->at(i).first == in){
			exists = i;
			break;
		}
	}
	if (exists == -1){
		commands->push_back(std::make_pair(in, "c"));
	}
	else {
		if (commands->at(exists).second.find("c") == std::string::npos)
			commands->at(exists).second.append("c");
	}
}
void gameState::shootChanged(int in){
	int exists = -1;
	for (int i = 0; i < commands->size(); ++i){
		if (commands->at(i).first == in){
			exists = i;
			break;
		}
	}
	if (exists == -1){
		commands->push_back(std::make_pair(in, "s"));
	}
	else {
		if (commands->at(exists).second.find("s") == std::string::npos)
			commands->at(exists).second.append("s");
	}
}
void gameState::healthChanged(int in){
	int exists = -1;
	for (int i = 0; i < commands->size(); ++i){
		if (commands->at(i).first == in){
			exists = i;
			break;
		}
	}
	if (exists == -1){
		commands->push_back(std::make_pair(in, "h"));
	}
	else {
		if (commands->at(exists).second.find("h") == std::string::npos)
			commands->at(exists).second.append("h");
	}
}
std::string gameState::getJSONStringFull(){
	rapidjson::Document fromScratch;
	fromScratch.SetObject();
	rapidjson::Document::AllocatorType& allocator = fromScratch.GetAllocator();
	string temp[500];
	for (int i = 0; i < commands->size(); ++i){
		rapidjson::Value object(rapidjson::kObjectType);
		std::string mod = commands->at(i).second;
		for (int j = 0; j < mod.size(); ++j){
			rapidjson::Value array(rapidjson::kArrayType);
			switch (mod[j]){
			case 'p':
				//std::cout << "p";
				for (int k = 0; k < 4; ++k)
				for (int l = 0; l < 4; ++l)
					array.PushBack(objects->at(commands->at(i).first)->getModelM()[k][l], allocator);
				object.AddMember("p", array, allocator);
				break;
			case 's':
				object.AddMember("s", objects->at(commands->at(i).first)->getShoot(), allocator);
				break;
			case 'h':
				object.AddMember("h", objects->at(commands->at(i).first)->getHealth(), allocator);
				break;
			case 'c':
				for (int k = 0; k < 4; ++k)
				for (int l = 0; l < 4; ++l)
					array.PushBack(objects->at(commands->at(i).first)->getCam()[k][l], allocator);
				object.AddMember("c", array, allocator);
				break;
			}
		}
		temp[i] = to_string(commands->at(i).first);
		fromScratch.AddMember(temp[i].c_str(), object, allocator);
	}
	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	fromScratch.Accept(writer);
	commands->clear();
	return strbuf.GetString();
}
void gameState::parseJSONString(std::string str){
	string name;
	rapidjson::Document parsedFromString;
	parsedFromString.SetObject();
	parsedFromString.Parse<0>(str.c_str());
	for (rapidjson::Value::ConstMemberIterator it = parsedFromString.MemberBegin(); it != parsedFromString.MemberEnd(); it++)
	for (rapidjson::Value::ConstMemberIterator it2 = it->value.MemberBegin(); it2 != it->value.MemberEnd(); it2++){
		name = it2->name.GetString();
		if (name == "p"){
			mat4 m;
			const rapidjson::Value& membersArray = (parsedFromString[it->name.GetString()])["p"];
			for (rapidjson::SizeType i = 0; i < membersArray.Size(); i++)
			{
				float temp = (float)membersArray[i].GetDouble();
				m[i/4][i%4] = temp;
			}
			
			objects->at(std::atoi(it->name.GetString()))->setModelM(m);
		}
		else if (name == "s"){
			objects->at(std::atoi(it->name.GetString()))->setShoot((float)it2->value.GetBool());
		}
		else if (name == "h"){
			objects->at(std::atoi(it->name.GetString()))->setHealth((float)it2->value.GetInt());
		}
		else if (name == "c"){
			mat4 m;
			const rapidjson::Value& membersArray = (parsedFromString[it->name.GetString()])["c"];
			for (rapidjson::SizeType i = 0; i < membersArray.Size(); i++)
			{
				float temp = (float)membersArray[i].GetDouble();
				m[i / 4][i % 4] = temp;
			}

			objects->at(std::atoi(it->name.GetString()))->setCam(m);
		}
	}
	std::vector<Object* > poop;
}
std::string gameState::getPosString(std::vector<std::pair<string, mat4>>* v){
	rapidjson::Document fromScratch;
	fromScratch.SetObject();
	rapidjson::Document::AllocatorType& allocator = fromScratch.GetAllocator();
	rapidjson::Value array(rapidjson::kArrayType);
	std::string temp;
	for (int h = 0; h < v->size(); ++h){
		rapidjson::Value array(rapidjson::kArrayType);
		for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j){
			temp = "";
			temp += std::to_string(v->at(h).second[i][j]);
			array.PushBack(v->at(h).second[i][j], allocator);
		}
		fromScratch.AddMember(v->at(h).first.c_str(), array, allocator);
	}
	rapidjson::StringBuffer strbuf;
	rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
	fromScratch.Accept(writer);
	return strbuf.GetString();
}
std::vector<std::pair<string, mat4> >* gameState::parsePosString(std::string str){
	rapidjson::Document parsedFromString;
	parsedFromString.SetObject();
	parsedFromString.Parse<0>(str.c_str());
	std::vector<std::pair<string, mat4> >* pos = new std::vector<std::pair<string, mat4> >;
	if (!parsedFromString["player1"].IsNull()){
		float nums[16] = { 0.0 };
		int k = 0;
		for (int i = 0; i < 4; ++i){
			for (int j = 0; j < 4; ++j){
				nums[i * 4 + j] = (float)parsedFromString["player1"][k].GetDouble();
				k++;
			}
		}
		glm::mat4 bbb = (glm::make_mat4(nums));
		pos->push_back(std::make_pair("player1", bbb));
	}
	if (!parsedFromString["player2"].IsNull()){
		float nums[16] = { 0.0 };
		int k = 0;
		for (int i = 0; i < 4; ++i){
			for (int j = 0; j < 4; ++j){
				nums[i * 4 + j] = (float)parsedFromString["player2"][k].GetDouble();
				k++;
			}
		}
		glm::mat4 bbb = (glm::make_mat4(nums));
		pos->push_back(std::make_pair("player2", bbb));
	}
	if (!parsedFromString["player3"].IsNull()){
		float nums[16] = { 0.0 };
		int k = 0;
		for (int i = 0; i < 4; ++i){
			for (int j = 0; j < 4; ++j){
				nums[i * 4 + j] = (float)parsedFromString["player3"][k].GetDouble();
				k++;
			}
		}
		glm::mat4 bbb = (glm::make_mat4(nums));
		pos->push_back(std::make_pair("player3", bbb));
	}
	if (!parsedFromString["player4"].IsNull()){
		float nums[16] = { 0.0 };
		int k = 0;
		for (int i = 0; i < 4; ++i){
			for (int j = 0; j < 4; ++j){
				nums[i * 4 + j] = (float)parsedFromString["player4"][k].GetDouble();
				k++;
			}
		}
		glm::mat4 bbb = (glm::make_mat4(nums));
		pos->push_back(std::make_pair("player4", bbb));
	}
	return pos;
}
void gameState::addObject(Object* obj){
	if (openIndices.size() == 0){
		objects->push_back(obj);
		obj->setGameStateIndex(objects->size() - 1);
		recentlyAdded = objects->size() - 1;
		commands->push_back(std::make_pair(objects->size() - 1, "pshc"));
	}
	else {
		objects->at(openIndices.back()) = obj;
		obj->setGameStateIndex(openIndices.back());
		recentlyAdded = openIndices.back();
		openIndices.pop_back();
		commands->push_back(std::make_pair(openIndices.back(), "pshc"));
	}
}
void gameState::removeObject(int i){
	//cout << "remove object\n";
	delete objects->at(i);
	objects->at(i) = NULL;
	openIndices.push_back(i);
}
std::vector<Object*> gameState::getObjectByType(string type){
	std::vector<Object*> res;
	for (int i = 0; i < objects->size(); ++i)
	if (objects->at(i)->getType() == type)
		res.push_back(objects->at(i));
	return res;
}

#endif