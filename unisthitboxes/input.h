#pragma once
#include <Windows.h>
#include <Xinput.h>
#include "Constants.h"


class Controller 
{
public:
	Controller();
	Controller(int id);
	int initController();


	void RefreshState();

	XINPUT_GAMEPAD GetState();

	bool checkButton(WORD button);

	bool checkDiagonal(WORD diag);

	int GetDirection();
	int GetReverseDirection();
	
	int GetButton();
private:
	XINPUT_STATE state;
	int controllerId = 0;

};


class ControllerManager {
public:
	ControllerManager();
	void invertDirection();
	void initControllers();
	void updateInputs();
	void updateInputs(int, int);
	//Wanted controller ids
	int local = 0, remote = 1;
	Controller localController;
	bool invert = false;
};