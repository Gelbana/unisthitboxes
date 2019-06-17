#pragma once
#include "input.h"
#include <stdio.h>
#include <Windows.h>
#include "util.h"
#pragma comment(lib, "XInput.lib") 


Controller::Controller() {}

Controller::Controller(int id)
{
	controllerId = id;
}

int Controller::initController()
{
	DWORD dwResult;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	dwResult = XInputGetState(controllerId, &state);

	if (dwResult == ERROR_SUCCESS)
	{
		printf("suc");
		return controllerId;
	}
	else
	{
		printf("fail");
		controllerId = -1;
		return controllerId;

	}

}

bool Controller::checkButton(WORD button)
{

	return (state.Gamepad.wButtons & button) != 0;

}

bool Controller::checkDiagonal(WORD diag)
{

	return ((state.Gamepad.wButtons == diag));

}

void Controller::RefreshState()
{
	if (controllerId != -1)
	{
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		if (XInputGetState(controllerId, &state) != ERROR_SUCCESS)
		{
			controllerId = -1;
		}
	}
	
}

XINPUT_GAMEPAD Controller::GetState()
{
	RefreshState();
	return state.Gamepad;
}


int Controller::GetButton()
{
	int total = 0;
	if (checkButton(XINPUT_GAMEPAD_A)) {
		//printf("Pressing X\n");
		total += 8;
	}

	if (checkButton(XINPUT_GAMEPAD_B)) {
		//printf("Pressing O\n");
		total += 4;
	}

	if (checkButton(XINPUT_GAMEPAD_Y)) {
		//printf("Pressing Triangle\n");
		total += 2;
	}

	if (checkButton(XINPUT_GAMEPAD_X)) {
		//printf("Pressing Square\n");
		total += 1;
	}

	if (checkButton(XINPUT_GAMEPAD_LEFT_SHOULDER)) {
		//printf("Pressing L1\n");
		total += 64;
	}

	if (checkButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
		//printf("Pressing R1\n");
		total += 128;
	}

	if (checkButton(XINPUT_GAMEPAD_START)) {
		printf("Pressing Start\n");
		total += 1;
	}

	if (checkButton(XINPUT_GAMEPAD_BACK)) {
		printf("Pressing Select\n");
		total += 2;
	}
	return total;
}

//Based on numpad notation
int Controller::GetDirection()
{
	//CHANGE TO SWITCH
	//Check diagonals first
	if ((checkButton(XINPUT_GAMEPAD_DPAD_DOWN)) & (checkButton(XINPUT_GAMEPAD_DPAD_LEFT))) {
		//printf("Pressing DownLeft\n");
		return 1;
	}

	if ((checkButton(XINPUT_GAMEPAD_DPAD_DOWN)) & (checkButton(XINPUT_GAMEPAD_DPAD_RIGHT))) {
		//printf("Pressing DownRight\n");
		return 3;
	}

	if ((checkButton(XINPUT_GAMEPAD_DPAD_UP)) & (checkButton(XINPUT_GAMEPAD_DPAD_RIGHT))) {
		//printf("Pressing UpRight\n");
		return 9;
	}

	if ((checkButton(XINPUT_GAMEPAD_DPAD_UP)) & (checkButton(XINPUT_GAMEPAD_DPAD_LEFT))) {
		//printf("Pressing UpLeft\n");
		return 7;
	}

	if (checkButton(XINPUT_GAMEPAD_DPAD_DOWN)) {
		//printf("Pressing Down\n");
		return 2;
	}

	if (checkButton(XINPUT_GAMEPAD_DPAD_RIGHT)) {
		//printf("Pressing Right\n");
		return 6;
	}

	if (checkButton(XINPUT_GAMEPAD_DPAD_UP)) {
		//printf("Pressing Up\n");
		return 8;
	}

	if (checkButton(XINPUT_GAMEPAD_DPAD_LEFT)) {
		//printf("Pressing Left\n");
		return 4;
	}

	return 0;
}

ControllerManager::ControllerManager()
{
	localController = Controller();
}


void ControllerManager::initControllers()
{
	localController.initController();
}

void ControllerManager::updateInputs() {


	localController.RefreshState();

	*(uint8_t*)(util::get_base_address() + 0x61BC34) = localController.GetButton();
	*(uint8_t*)(util::get_base_address() + 0x61BC37) = localController.GetDirection();


}

void ControllerManager::updateInputs(int button, int direction) {


	localController.RefreshState();
	//printf("%d\n", localController.checkButton(XINPUT_GAMEPAD_A));
	*(int*)(util::get_base_address() + 0x323E04) = button;
	*(int*)(util::get_base_address() + 0x323DF0) = direction;


}

