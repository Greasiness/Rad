#pragma once
#include <vector>
#include <map>
#include <math.h> 
#include <iostream>
#include <stdlib.h>
#include "Object.h"
#include "Octree.h"
#include "Projectile.h"
#include "Window.h"
#include "MD5Model.h"
#include "Camera.h"
#include "Ground.h"
#include "gamestate.h"
using namespace std;


#define RESPAWN_COUNTER 100
#define MAX_SPEED 100
#define MAX_DISTANCE 100

#define MAX_DAMAGE -50


class Scene
{
public:
	Scene(){
		state = new gameState();
		initialize();
	}
	~Scene(){
		for (uint i = 0; i<stationary.size(); i++){
			delete stationary[i];
		}
		stationary.clear();
		for (uint i = 0; i<player.size(); i++){
			delete player[i];
		}
		player.clear();
		delete state;
	}

	Object * getPlayerObj(int playerID)
	{
		vector<Object*> player1 = state->getObjectByType("player");
		for (int i = 0; i < player1.size(); i++)
		{
			if (player1[i]->getPlayerID() == playerID)
				return player1[i];
		}
		return NULL;
	}


	void simulate(float t, float sub){
		 player = state->getObjectByType("player");
		 stationary = state->getObjectByType("stationary");
		resolvePlayerRotation();
		while (t > sub){
			t -= sub;
			resolvePlayerTransition(sub);
			//	//octree here
			collisionDetection();
			collisionDetectionPlayer();
			collisionDetectionProjectile();
			despawnProjectile();
			rechargeJump();
			respawnPlayer();
		}
		resolvePlayerTransition(sub);
		//	//octree here
		collisionDetection();
		collisionDetectionPlayer();
		collisionDetectionProjectile();
		despawnProjectile();
		rechargeJump();
		respawnPlayer();
	}
	void collisionDetection(Octree* octree);
	void collisionDetection(){
		//player-stationary detection
		for (uint i = 0; i < player.size(); i++){
			bool touchGround1 = false;
			bool touchGround2 = false;
			for (uint j = 0; j < stationary.size(); j++){
				if (strcmp(stationary[j]->getName().c_str(), "ground") == 0){
					AABB pBox = player[i]->getAABB();
					vec3 mid = (pBox.max + pBox.min) / 2.0f;
					float disp = ((Ground*)stationary[j])->getDispY(mid[0], mid[2]);
					if (disp != -1){
						if (pBox.min[1] < disp){
							touchGround1 = true;
							player[i]->postTrans(glm::translate(vec3(0.0f,disp-pBox.min[1],0.0f)));
							player[i]->clearYVelocity();
						}
						if (pBox.min[1] - disp < 0.1)
							touchGround1 = true;
					}
				}
				else{
					AABB pBox = player[i]->getAABB();
					AABB sBox = stationary[j]->getAABB();
					bool collide = true;
					for (int v = 0; v < 3; v++){
						if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
							collide = false;
							break;
						}
					}
					if (collide){
						fixCollision(player[i], stationary[j], pBox, sBox, touchGround1, touchGround2);
					}
					stationary[j]->touchGround(touchGround2);
				
				}
			}
			player[i]->touchGround(touchGround1);
		
		}
	}

	void collisionDetectionPlayer(){
		//player-stationary detection
		for (uint i = 0; i < player.size(); i++){
			bool touchGround1 = player[i]->getTouchGround();
			for (uint j = i + 1; j < player.size(); j++){
				bool touchGround2 = player[j]->getTouchGround();
				AABB pBox = player[i]->getAABB();
				AABB sBox = player[j]->getAABB();
				bool collide = true;
				for (int v = 0; v < 3; v++){
					if (pBox.max[v] <= sBox.min[v] || sBox.max[v] <= pBox.min[v]){
						collide = false;
						break;
					}
				}
				if (collide){
					fixCollision(player[i], player[j], pBox, sBox, touchGround1, touchGround2);
				}
				player[j]->touchGround(touchGround2);

			}
			player[i]->touchGround(touchGround1);
		}
	}


	void fixCollision(Object* obj1, Object* obj2, AABB& box1, AABB& box2, bool& onGround1, bool& onGround2){
		float Rewind[3];
		float minRewind = 999;
		int minID = 0;
		vec3 v1 = vec3(obj1->getModelM()*vec4(obj1->getVelocity(), 0.0));
		vec3 v2 = vec3(obj2->getModelM()*vec4(obj2->getVelocity(), 0.0));
		vec3 vDiff = v1 - v2;

		for (int v = 0; v<3; v++){
			if (vDiff[v] > 0){
				Rewind[v] = (box1.max[v] - box2.min[v]) / vDiff[v];
			}
			else if (vDiff[v] < 0){
				Rewind[v] = (box1.min[v] - box2.max[v]) / vDiff[v];
			}
			else{//avoid division by 0
				Rewind[v] = 999;
			}
			if (Rewind[v] < minRewind){
				minRewind = Rewind[v];
				minID = v;
			}
		}
		vec3 rwVelocity1 = vec3(0.0);
		rwVelocity1[minID] = -v1[minID];
		obj1->preTrans(glm::translate(rwVelocity1*minRewind));
		vec3 rwVelocity2 = vec3(0.0);
		rwVelocity2[minID] = -v2[minID];
		obj2->preTrans(glm::translate(rwVelocity2*minRewind));
		if (minID == 1){
			obj1->clearYVelocity();
			onGround1 = true;
			obj2->clearYVelocity();
			onGround2 = true;
		}
		else{//stair effect
			AABB b1 = obj1->getAABB();
			AABB b2 = obj2->getAABB();
			if (b2.max[1] > b1.min[1] && b2.max[1] - b1.min[1] <= 0.11){
				obj1->preTrans(glm::translate(glm::vec3(0.0f, b2.max[1]-b1.min[1]+0.01f, 0.0f)));
			}
		}
	}
	void addPlayer(Object* p){ p->setType("player");  state->addObject(p); cout << "player added \n"; state->camChanged(state->getRecentlyAdded()); }
	void addStationary(Object* s){ s->setType("stationary");  state->addObject(s); cout << "stationary added\n";  }
	void addProjectile(Projectile* p){ projectile.push_back(p); }
	void setGravity(vec3& g){ gravity = g; }

	void resolvePlayerRotation(){
		for (uint i = 0; i < player.size(); i++){
			player[i]->postRotate(glm::rotate(mat4(1.0), player[i]->getPendingRote(), vec3(0, 1, 0)));
			player[i]->setPendingRot(0);
		}
	}
	void setCamM(int playerID, mat4 cam){
		if (playerID >= 4){
			cout << playerID << endl;
			system("pause");
		}
		state->getObject(playerID)->setCam(cam);
		state->camChanged(playerID);
	}

	void resolvePlayerTransition(float t){
		for (uint i = 0; i < player.size(); i++){
			player[i]->addVelocity(gravity*t);
			player[i]->postTrans(glm::translate(player[i]->getVelocity()*t));
			state->posChanged(player[i]->getGameStateIndex());
		}
		for (uint i = 0; i < projectile.size(); i++){
			projectile[i]->addVelocity(gravity*t);
			projectile[i]->postTrans(glm::translate(projectile[i]->getVelocity()*t));
		}
	}


	void setHMove(int playerID, int m){ state->getObject(playerID)->setHMove(m);  }
	void cancelHMove(int playerID, int m){ state->getObject(playerID)->cancelHMove(m); }
	void setVMove(int playerID, int m){ state->getObject(playerID)->setVMove(m); }
	void cancelVMove(int playerID, int m){ state->getObject(playerID)->cancelVMove(m); }
	
	
	void setPendingRot(int playerID, float f){ state->getObject(playerID)->setPendingRot(f); }
		void pushRot(int playerID, float f){ state->getObject(playerID)->pushRot(f); }
		void jump(int playerID){ state->getObject(playerID)->jump(); }


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////// START OF PLAYER ACTIONS /////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void respawnPlayer()
	{
		Object * holder;
		//int id;
		for (uint i = 0; i < player.size(); i++)
		{
			if (player[i]->getHealth() < 1)
			{

				holder = player[i];
				holder->setRespawn(holder->getRespawn() - 1);//no need to update, client shouldn't care
				
				if (holder->getRespawn() < 1)
				{
					//Window::addDrawList(holder);
					//Window::addPlayerList(holder);
					//Window::respawnPlayer(holder->getName());

					holder->putHealth(7);
					state->healthChanged(holder->getGameStateIndex());
					//state->setHealth(holder->getGameStateIndex(), -(holder->getHealth()) + 7);

					//holder->setModelM(holder->getAliveModelM());
					holder->setModelM(holder->getAliveModelM());
					state->posChanged(holder->getGameStateIndex());

					//cout << holder->getName() << " " << holder->getRespawn() << " " << holder->getHealth() << " " << player[i]->getHealth() << endl;

					//id = holder->getPlayerID();
					////insert the player at position at playerID
					////removes all players after playerID and appends them to the list.
					//for (int i = id; i < player.size() - 1; i++)
					//{
					//	holder = player[id];
					//	player.erase(player.begin() + id);
					//	player.push_back(holder);
					//}

					//cout << "" << endl;
				}
			}
		}
	}


	void collisionDetectionProjectile(){
		//player-stationary detection
		for (uint i = 0; i < projectile.size(); i++)
		{
			bool touchGround1 = projectile[i]->getTouchGround();
			for (uint j = 0; j < player.size(); j++)
			{
				if (projectile[i]->getPlayerID() == player[j]->getPlayerID())
					continue;
				AABB pBox = projectile[i]->getAABB();
				AABB sBox = player[j]->getAABB();
				bool touchGround2 = player[j]->getTouchGround();
				bool collide = false;
				int deltaX = pBox.max[0] - sBox.max[0];
				int deltaY = pBox.max[2] - sBox.max[2];
				int deltaZ = pBox.max[1] - sBox.max[1];
				//cout << deltaZ << endl;
				//[p][s]
				//if ((pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2] - 1) && (pBox.max[0] <= sBox.max[0]) && (pBox.max[0] >= sBox.max[0] - 1) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				if ((abs(deltaX) < 5) && (abs(deltaY) < 5) && (abs(deltaZ) < 5) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				{
					player[j]->postTrans(glm::translate(vec3(1, 0.5, 0)));
					(*projectile[i]).setHit(player[j]->getPlayerID());
					damagePlayer(player, player[j]->getPlayerID(), projectile[i]->getPlayerID());
				}
				////[s][p]
				//else if ((pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2] - 1) && (pBox.max[0] <= sBox.max[0]) && (pBox.max[0] >= sBox.max[0] - 1) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				//{
				//	player[j]->postTrans(glm::translate(vec3(-1, 0.5, 0)));
				//	(*projectile[i]).setHit(player[j]->getPlayerID());
				//	damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				//}
				////[p]
				////[s]
				//else if ((pBox.max[0] <= sBox.max[0] + 1) && (pBox.max[0] >= sBox.max[0]) && (pBox.max[2] <= sBox.max[2]) && (pBox.max[2] >= sBox.max[2]) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				//{
				//	player[j]->postTrans(glm::translate(vec3(0, 0.5, -1)));
				//	(*projectile[i]).setHit(player[j]->getPlayerID());
				//	damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				//}
				////[s]
				////[p]
				//else if ((pBox.max[0] <= sBox.max[0] + 1) && (pBox.max[0] >= sBox.max[0]) && (pBox.max[2] <= sBox.max[2] + 1) && (pBox.max[2] >= sBox.max[2]) && !(*projectile[i]).checkHit(player[j]->getPlayerID()))
				//{
				//	player[j]->postTrans(glm::translate(vec3(0, 0.5, 1)));
				//	(*projectile[i]).setHit(player[j]->getPlayerID());
				//	damagePlayer(player[j]->getPlayerID(), projectile[i]->getPlayerID());
				//}
			}
		}
	}

	void damagePlayer(vector<Object*> player, int targetId, int playerId)
	{
		Object * playerHolder = player[playerId];
		Object * targetHolder = player[targetId];
		targetHolder->setHealth(((RangeWeapon *)playerHolder->getWeapon())->getDamage());
		state->healthChanged(targetHolder->getGameStateIndex());
		//state->setHealth(targetHolder->getGameStateIndex(), targetHolder->getHealth() + ((RangeWeapon *)playerHolder->getWeapon())->getDamage());
		if (targetHolder->getHealth() < 1)
		{
			int dist, spd, dmg;
			dist = ((RangeWeapon *)playerHolder->getWeapon())->getDistance() * 2;
			spd = ((RangeWeapon *)playerHolder->getWeapon())->getSpeed() * 2;
			dmg = ((RangeWeapon *)playerHolder->getWeapon())->getDamage() * 2;
			//restricting speed and distance
			if (dist > MAX_DISTANCE)
				dist = MAX_DISTANCE;
			if (spd > MAX_SPEED)
				spd = MAX_SPEED;
			if (dmg < MAX_DAMAGE)
				dmg = MAX_DAMAGE;
			 
			targetHolder->setRespawn(RESPAWN_COUNTER);
			//Window::removeDrawList((*targetHolder).getName());
			//Window::removePlayerList((*targetHolder).getName());
			//respawn.push_back(targetHolder);
			//for (int i = 0; i < player.size(); i++)
			//{
			//	if (player[i]->getPlayerID() == targetId)
			//	{
			//		player[i]->setAliveModelM(player[i]->getModelM());
			//		player[i]->setModelM(player[i]->getModelM()*glm::translate(vec3(0, 50, 0)));
			//	}
			//}
			targetHolder->setAliveModelM(targetHolder->getModelM());
			state->posChanged(targetHolder->getGameStateIndex());
			targetHolder->setModelM(targetHolder->getModelM()*glm::translate(vec3(0, 50, 0)));
			//state->setModelM(targetHolder->getGameStateIndex(), targetHolder->getModelM()*glm::translate(vec3(100, 100, 100)));
			cout << playerId << " " << dmg << endl;

			//TODO: add weapons to gamestate
			RangeWeapon * newItem = new RangeWeapon(dist,
													spd,
													dmg);
			playerHolder->setWeapon(newItem);
			playerHolder->setKills(1);
			//RangeWeapon * newItem = new RangeWeapon(((RangeWeapon *)player[0]->getItem())->getDistance() * 3
			//										, ((RangeWeapon *)player[0]->getItem())->getSpeed() * 3);
		}
	}

	void basicAttack(int playerID)
	{
		vector<Object*> player = state->getObjectByType("player");
		for (int j = 0; j < player.size(); j++)
	    {
	    if (j == playerID)
	      continue;
	    AABB pBox = player[playerID]->getAABB();
	    AABB sBox = player[j]->getAABB();
	    vector<int> holder;	
	    //[p][s]
	    if ((pBox.max[2] <= sBox.max[2] + 5) && (pBox.max[2] >= sBox.max[2] - 5)  && pBox.max[0] == sBox.min[0])
	      {
			player[j]->postTrans(glm::translate(vec3(1, 0.5, 0)));
			state->posChanged(player[j]->getGameStateIndex());
			//state->postTrans(player[j]->getGameStateIndex(), glm::translate(vec3(1, 0.5, 0)));
			damagePlayer(player, j, playerID);
	      }
	    //[s][p]
	    else if ((pBox.max[2] <= sBox.max[2] + 5) && (pBox.max[2] >= sBox.max[2] - 5) && pBox.min[0] == sBox.max[0])
		{
			player[j]->postTrans(glm::translate(vec3(-1, 0.5, 0)));
			state->posChanged(player[j]->getGameStateIndex());
			//state->postTrans(player[j]->getGameStateIndex(), glm::translate(vec3(-1, 0.5, 0)));
			damagePlayer(player, j, playerID);
	      }
	    //[p]
	    //[s]
	    else if ((pBox.max[0] <= sBox.max[0] + 5) && (pBox.max[0] >= sBox.max[0] - 5) && pBox.min[2] == sBox.max[2])
		{
			player[j]->postTrans(glm::translate(vec3(0, 0.5, -1)));
			state->posChanged(player[j]->getGameStateIndex());
			//state->postTrans(player[j]->getGameStateIndex(), glm::translate(vec3(0, 0.5, -1)));
			damagePlayer(player, j, playerID);
	      }
	    //[s]
	    //[p]
	    else if ((pBox.max[0] <= sBox.max[0] + 5) && (pBox.max[0] >= sBox.max[0] - 5) && pBox.max[2] == sBox.min[2])
		{
			player[j]->postTrans(glm::translate(vec3(0, 0.5, 1)));
			state->posChanged(player[j]->getGameStateIndex());
			//state->postTrans(player[j]->getGameStateIndex(), glm::translate(vec3(0, 0.5, 1)));
			damagePlayer(player, j, playerID);
	      }
	    }
	  
	}

	void projectileAttack(int playerID, mat4 * test)
	{
		state->getObject(playerID)->setShoot(true); 
		state->shootChanged(playerID);
		vector<Object*> player = state->getObjectByType("player");
		//mat4 test = *cam; //cam->getCamM();
		vec4 holder = (*test) * vec4(0, 0, -1, 0); //orientation of camera in object space
		mat4 player1 = getPlayerObj(playerID)->getModelM();
		vec4 playerHolder = player1*vec4(0, 0, 0, 1);
		Cube * playerObject = ((Cube *)getPlayerObj(playerID));
		Object * playerHold = getPlayerObj(playerID);

		Projectile* cubeT = new Projectile(player.size() + respawn.size());
		cubeT->setKd(vec3(0.8, 0.0, 0.0));
		cubeT->setKa(vec3(0.3, 0.0, 0.0));
		cubeT->setKs(vec3(0.4, 0.0, 0.0));
		cubeT->setShininess(100);
		cubeT->setReflectFactor(vec2(0.2, 0.5));
		cubeT->setEta(0.5);
		cubeT->setCubeMapUnit(3);
		cubeT->setSpeed(5);
		//cubeT->postTrans(glm::translate(vec3(playerHolder[0] -2 + ((holder[0]) / 4), playerHolder[1], playerHolder[2] - (holder[2] / 4))));
		cubeT->setModelM(player1*glm::translate(vec3(0, 1, 0)));//get the new cube matrix by translating the player0 matrix forward in player0 object space. This way the new matrix will inherit player0 oriantation 
		cubeT->setAABB(AABB(vec3(-0.5, -0.5, -0.5), vec3(0.5, 0.5, 0.5)));
		AABB hold = cubeT->getAABB();
		cubeT->setStartX(hold.max[0]);
		cubeT->setStartY(hold.max[2]);
		cubeT->setPlayerID(playerID);

		//Name and type
		cubeT->setType("Cube");
		cubeT->setName("Test Cube" + std::to_string(projectile_counter));
		projectile_counter++;
		cubeT->setDistance(((RangeWeapon *)(playerHold)->getWeapon())->getDistance());
		//cout << cubeT->getPlayerID() << " " << cubeT->getDistance() << " " << cubeT->getSpeed() << " " << ((RangeWeapon *)playerHold->getWeapon())->getDamage() << " " << endl;
		//Add Cube to the draw list
		////////////////////////////////////////////////////////Window::addDrawList(cubeT);
		projectile.push_back(cubeT);
		cubeT->setSpeed(50);
		//cubeT->setHMove((holder[0] / 4));
		cubeT->setVelocity(vec3(holder)*((RangeWeapon *)playerHold->getWeapon())->getSpeed());// set object space velocity to camera oriantation in object space. Since camera always have the same xz oriantation as the object, xz oriantation wouldnt change when camera rotate.
		//cubeT->setVMove(1);  //do this if you want the cube to not have vertical velocity. uncomment the above setVelocity.
		//cout << holder[0] << ' ' << holder[1] << ' ' << holder[2] << ' ' << playerHolder[0] << ' ' << playerHolder[2] << endl;
	}

	void despawnProjectile()
	{
		for (uint i = 0; i < projectile.size(); i++)
		{
			float startX = projectile[i]->getStartX();
			float startY = projectile[i]->getStartY();
			AABB curr = projectile[i]->getAABB();
			int distance = sqrt(pow(curr.max[0] - startX, 2) + pow(curr.max[2] - startY, 2));//Pythagorean Theorem

			//cout << startX << " " << curr.max[0] << " " << curr.max[0] - startX << " " << distance << endl;
			if (distance >= (*projectile[i]).getDistance())
			{
				////////////////////////////////////////////////Window::removeDrawList((*projectile[i]).getName());
				projectile.erase(projectile.begin() + i);
			}
		}
	}

	void recover(int playerId)
	{
		getPlayerObj(playerId)->jump(20);
	}

	void rechargeJump()
	{
		for (uint i = 0; i < player.size(); i++)
		{
			if (player[i]->getTouchGround() && player[i]->getNumJumps() < player[i]->getTotalJumps())
				player[i]->incNumJumps();
		}
	}

	void takeItem()
	{

	}

	void resetVelocity(int playerId)
	{
		player[playerId]->resetVelocity();
	}

	int numPlayers()
	{
		return player.size();
	}

	gameState * getGameState()
	{
		return state;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////// END OF PLAYER ACTIONS ///////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	vector<mat4> getPlayerMats(){
		//vector<Object*> player = state->getObjectByType("player");
		vector<mat4> m;
		for (uint i = 0; i < player.size(); i++){
			m.push_back(player[i]->getModelM());
		}
		return m;
	}

	Object * getPlayerElement(int i)
	{
		vector<Object*> player = state->getObjectByType("player");
		return player[i];
	}

	Object * getPlayer(int i)
	{
		return getPlayerObj(i);
	}
	void initialize(){

		
		MD5Model* md50 = new MD5Model();
		md50->setSpeed(5);
		md50->postTrans(glm::translate(vec3(-20, 0.5, -20)));
		md50->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md50->setType("player");
		md50->setName("Player Model0");
		md50->setPlayerID(0);
		md50->setCam(mat4(1.0));
		addPlayer(md50);

		MD5Model* md51 = new MD5Model();
		md51->setSpeed(5);
		md50->postTrans(glm::translate(vec3(5, 0.5, 7)));
		md51->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md51->setType("player");
		md51->setName("Player Model1");
		md51->setCam(mat4(1.0));
		md51->setPlayerID(1);
		addPlayer(md51);

		MD5Model* md52 = new MD5Model();
		md52->setSpeed(5);
		md50->postTrans(glm::translate(vec3(10, 0.5, 7)));
		md52->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md52->setType("player");
		md52->setCam(mat4(1.0));
		md52->setName("Player Model2");
		md52->setPlayerID(2);
		addPlayer(md52);

		MD5Model* md53 = new MD5Model();
		md53->setSpeed(5);
		md50->postTrans(glm::translate(vec3(15, 0.5, 7)));
		md53->setAABB(AABB(vec3(-0.25, 0.0, -0.25), vec3(0.25, 1.5, 0.25)));
		md53->setType("player");
		md53->setName("Player Model3");
		md53->setCam(mat4(1.0));
		md53->setPlayerID(3);
		addPlayer(md53);

		Ground* ground = new Ground();
		ground->loadColorTex("img/moon_tex/moon_COLOR.png", "PNG");
		ground->loadDispTex("img/moon_tex/moon_DISP.png", "PNG");
		ground->loadNormalTex("img/moon_tex/moon_NRM.png", "PNG");
		ground->loadOccTex("img/moon_tex/moon_OCC.png", "PNG");
		ground->loadSpecTex("img/moon_tex/moon_SPEC.png", "PNG");
		ground->setDimensionS(40);
		ground->setDimensionT(40);
		ground->setRow(501);
		ground->setColumn(501);
		ground->setHeight(1 / 1.0);
		ground->generate();
		ground->setType("stationary");
		ground->setName("ground");
		addStationary(ground);




		counter = 0;
		projectile_counter = 0;
	}

protected:
	vector<Object*> stationary;
	vector<Object*> player;
	vector<Object*> respawn;
	vector<Object*> tower;
	vector<Object*> skillShot;
	vector<Object*> virtualTower;
	vector<Projectile*> projectile;
	vector<Item *> items;
	vector<mat4> camM;
	vec3 gravity;
	vector<vector<int>> prevAttacked;//first element is playerID, second is axis
	int counter;
	int projectile_counter;
	gameState* state;
};
