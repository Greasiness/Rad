
#define _USE_MATH_DEFINES
#include <string>
#include <iostream>
#include <time.h>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include "Server.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "Window.h"
#include <time.h>
#include "Object.h"
#include "VAO.h"
#include "glslprogram.h"
#include "Cube.h"
#include "ShaderController.h"
#include "Ground.h"
#include <Qt/QtGui/QImage> 
#include <Qt/QtOpenGL/QGLWidget>
#include "SkyBox.h"
#include "Structures.h"
#include "Sphere.h"
#include "TextureScreen.h"
#include "Camera.h"
#include "Scene.h"
#include "Mesh.h"
#include "Texture.h"
#include "ConfigSettings.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "gameState.h"
using glm::vec3;
using glm::vec4;
using glm::mat4;
using glm::mat3;
using glm::quat;
using namespace std;
Ground* ground;
float cam_sp = (float)0.1;
string configBuf;
int counter = 0;
std::vector <pair<string, mat4>>* sendVec = new vector<pair<string, mat4>>;
std::vector <pair<string, mat4>>* recvVec = new vector<pair<string, mat4>>;
Scene* scene;
boost::asio::io_service io_service;
tcp_server* server;
int cam_rot = 0;
int playerID = -1;
int numOfVecs = 4;
int keyState = 0;
bool player1shoot, player2shoot, player3shoot, player4shoot;

std::string str;
gameState gs;

void handle_mouse_state(int pid, int mouseState){
	if (mouseState & 1){
		scene->basicAttack(pid);
	}
	else if (mouseState & 1 << 1){
		//std::cout << "projectile attack from client" << std::endl;
		if (pid == 0)
			player1shoot = true;
		else if (pid == 1)
			player2shoot = true;
		else if (pid == 2)
			player3shoot = true;
		else if (pid == 3)
			player4shoot = true;
		//std::cout << player1shoot << player2shoot << player3shoot << player4shoot << std::endl;
		scene->projectileAttack(pid, &(*recvVec)[playerID * 4 + 2].second);
	}
}
void handle_key_state(int pid, int keyState){
	if (scene->getPlayerObj(pid) == NULL)
		return;
	if (keyState & 1){ //'a'
		//cout << "move left " << pid << " for " << -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed() << endl;
		scene->setHMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelHMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 1){ //'d'
		//cout << "move right " << pid << " for " << -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed() << endl;
		scene->setHMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelHMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 2){ //'w'
		//cout << "move up " << pid << " for " << -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed() << endl;
		scene->setVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		scene->cancelVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 3){ //'s'
		//cout << "move down " << pid << " for " << -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed() << endl;
		scene->setVMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	else{
		//scene->cancelVMove(pid, -((scene->getPlayer(playerID))->getBoots())->getMoveSpeed());
	}
	if (keyState & 1 << 4){ //' '
		//cout << "jump " << pid  << endl;
		scene->jump(pid);
	}
	else
	{

	}
	if (keyState & 1 << 5){ //'W' for sprint
		scene->setVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getSprintSpeed());
	}
	else{
		scene->cancelVMove(pid, ((scene->getPlayer(playerID))->getBoots())->getSprintSpeed());
	}
}
void handle_cam_rot(int pid, float cam_rot){
	if (scene->getPlayerObj(pid) == NULL)
		return;
	scene->pushRot(pid, -cam_sp*cam_rot);
	cam_rot = 0; // possibly a problem
}
void handle_cam_mat(int pid, mat4 camM)
{
	scene->setCamM(pid, camM);
}
int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // open an OpenGL context with double buffering, RGB colors, and depth buffering
	glutInitWindowSize(10, 10); // set initial window size
	glutCreateWindow("CSE 125 - Group 4 (RadioactiveChipmunks)");
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error initializing GLEW: %s\n",
			glewGetErrorString(err));
		system("pause");
	}
	scene = new Scene();
	scene->setGravity(vec3(0, -9.8, 0));

	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));
	sendVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));
	recvVec->push_back(std::make_pair("", mat4(0.0f)));

	srand(time(NULL));

	try
	{
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(tcp::v4(), "localhost", "13"); // does nothing for the moment
		tcp::resolver::iterator itr = resolver.resolve(query);
		tcp::endpoint endpoint = *itr;
		server = new tcp_server(io_service, endpoint);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	LARGE_INTEGER freq, last, current, loop_end;
	double diff;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&last);
	string p0, p1, p2, p3;

	bool newData[4] = { false };
	while (true){
		QueryPerformanceCounter(&current);
		diff = (double)(current.QuadPart - last.QuadPart) / (double)freq.QuadPart;
		last = current;
		recvVec = server->getState();
		io_service.poll();
		server->getDataState(newData);
		server->age();//tell server the data is deprecated
		player1shoot = false;
		player2shoot = false;
		player3shoot = false;
		player4shoot = false;
		if (strcmp((*recvVec)[0].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[0].first.c_str());
			playerID = 0;
			handle_key_state(playerID, (int)(*recvVec)[playerID * 4].second[0][0]);
			if (newData[0]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * 4 + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * 4 + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * 4 + 3].second[0][0]);
			}
		}
		// VECTOR INDICES NEED UPDATE FOR MOUSE
		if (strcmp((*recvVec)[numOfVecs].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[numOfVecs].first.c_str());
			playerID = 1;
			handle_key_state(playerID, (int)(*recvVec)[playerID * 4].second[0][0]);
			if (newData[1]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * 4 + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * 4 + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * 4 + 3].second[0][0]);
			}

		}
		if (strcmp((*recvVec)[numOfVecs * 2].first.c_str(), ""))
		{
			playerID = atoi((*recvVec)[numOfVecs * 2].first.c_str());
			playerID = 2;
			handle_key_state(playerID, (int)(*recvVec)[playerID * 4].second[0][0]);
			if (newData[2]){
				handle_mouse_state(playerID, (int)(*recvVec)[playerID * 4 + 1].second[0][0]);
				handle_cam_mat(playerID, (*recvVec)[playerID * 4 + 2].second);
				handle_cam_rot(playerID, (int)(*recvVec)[playerID * 4 + 3].second[0][0]);
			}
		}
		/*if (strcmp((*recvVec)[numOfVecs * 3].first.c_str(), ""))
		{
		playerID = atoi((*recvVec)[numOfVecs * 3].first.c_str());
		handle_key_state(playerID, (int)(*recvVec)[playerID * 4].second[0][0]);
		if (newData[3]){
		handle_mouse_state(playerID, (int)(*recvVec)[playerID * 4 + 1].second[0][0]);
		handle_cam_mat(playerID, (*recvVec)[playerID * 4 + 2].second);
		handle_cam_rot(playerID, (int)(*recvVec)[playerID * 4 + 3].second[0][0]);
		}
		}*/

		scene->simulate(diff, (float)(1.0 / 100));
		boost::array<mat4, 4> m;
		for (int i = 0; i < scene->numPlayers(); i++)
		{
			m[i] = scene->getPlayerMats()[i];
		}
		// Print out matrix contents
		/*
		cout << (m[0])[0][0] << (m[0])[0][1] << (m[0])[0][2] << (m[0])[0][3] << endl;
		cout << (m[0])[1][0] << (m[0])[1][1] << (m[0])[1][2] << (m[0])[1][3] << endl;
		cout << (m[0])[2][0] << (m[0])[2][1] << (m[0])[2][2] << (m[0])[2][3] << endl;
		cout << (m[0])[3][0] << (m[0])[3][1] << (m[0])[3][2] << (m[0])[3][3] << endl;
		*/
		if (player1shoot == true)
			p0 = "0s";
		else
			p0 = "0S";
		if (player2shoot == true)
			p1 = "1s";
		else
			p1 = "1S";
		if (player3shoot == true)
			p2 = "2s";
		else
			p2 = "2S";
		if (player4shoot == true)
			p3 = "3s";
		else
			p3 = "3S";

		(*sendVec)[0] = std::make_pair("0", m[0]);
		(*sendVec)[1] = std::make_pair("1", m[1]);
		(*sendVec)[2] = std::make_pair("2", m[2]);
		(*sendVec)[3] = std::make_pair("3", m[3]);

		//std::cout << gs.getPosString(sendVec) << std::endl;
		//std::cout << "pair 0: " << ((*sendVec)[0].first.c_str()) << std::endl;
		//std::cout << "pair 1: " << ((*sendVec)[1].first.c_str()) << std::endl;
		//std::cout << "pair 2: " << ((*sendVec)[2].first.c_str()) << std::endl;
		//std::cout << "pair 3: " << ((*sendVec)[3].first.c_str()) << std::endl;
		//server->send(*sendVec);

		/*int num = rand() % 1000 + 1;

		for (int i = 0; i < num; i++)
		{
		str = str + "a";
		}

		num = rand() % 1000 + 1;

		for (int i = 0; i < num; i++)
		{
		str = str + "b";
		}

		str = "{" + str + "}";*/

		str = (scene->getGameState())->getJSONStringFull();

		server->send(str + '`');
		io_service.poll();

		//str = "";

		//limit the speed of server
		QueryPerformanceCounter(&loop_end);
		diff = (double)(loop_end.QuadPart - last.QuadPart) / (double)freq.QuadPart * 1000;
		if (diff < 15){
			//cout << diff << endl;
			Sleep(15 - diff);
		}
		else{
			//cout << diff << endl;
		}
	}
	return 0;
}