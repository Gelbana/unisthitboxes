#define _CRT_SECURE_NO_WARNINGS

#include "util.h"
#include "game.h"
#include "graphics.h"
#include "rollback.h"
#include "Constants.h"
#include <Windows.h>
#include <d3d9.h>
#include <detours.h>
#include "input.h"


ControllerManager controllers = ControllerManager();

void draw_pushbox(IDirect3DDevice9 *device, const game::CHARA_DATA *object)
{
	const auto *cmd_info = object->state_cmd_info;
	const auto flip = object->flipped ? -1 : 1;

	if (cmd_info == nullptr || cmd_info->hurtbox_count == 0)
		return;

	const auto pushbox = cmd_info->hurtboxes[0];

	if (pushbox == nullptr)
		return;

	graphics::draw_box(
		device,
		object->pos_x + object->parent_pos_x + pushbox->x1 * game::COORD_TO_PIXEL * flip,
		object->pos_y + object->parent_pos_y + pushbox->y1 * game::COORD_TO_PIXEL,
		object->pos_x + object->parent_pos_x + pushbox->x2 * game::COORD_TO_PIXEL * flip,
		object->pos_y + object->parent_pos_y + pushbox->y2 * game::COORD_TO_PIXEL,
		D3DCOLOR_ARGB(1, 255, 255, 0),
		D3DCOLOR_ARGB(255, 255, 255, 0));
}

void draw_hurtboxes(IDirect3DDevice9 *device, const game::CHARA_DATA *object)
{
	const auto *cmd_info = object->state_cmd_info;
	const auto flip = object->flipped ? -1 : 1;

	if (cmd_info == nullptr)
		return;

	// Hollow when invuln
	// Cyan when counter
	// Blue when high counter
	const auto alpha = object->invuln_frames != 0 ? 0 : 1;
	const auto red = 0;
	const auto green = (object->ch_flags & game::CH_HIGH_COUNTER) ? 0 : 255;
	const auto blue = object->ch_flags != 0 ? 255 : 0;

	for (auto i = 1; i < cmd_info->hurtbox_count; i++)
	{
		const auto hurtbox = cmd_info->hurtboxes[i];

		if (hurtbox == nullptr)
			continue;

		graphics::draw_box(
			device,
			object->pos_x + object->parent_pos_x + hurtbox->x1 * game::COORD_TO_PIXEL * flip,
			object->pos_y + object->parent_pos_y + hurtbox->y1 * game::COORD_TO_PIXEL,
			object->pos_x + object->parent_pos_x + hurtbox->x2 * game::COORD_TO_PIXEL * flip,
			object->pos_y + object->parent_pos_y + hurtbox->y2 * game::COORD_TO_PIXEL,
			D3DCOLOR_ARGB(alpha, red, green, blue),
			D3DCOLOR_ARGB(255, red, green, blue));
	}
}

void draw_hitboxes(IDirect3DDevice9 *device, const game::CHARA_DATA *object)
{
	const auto *cmd_info = object->state_cmd_info;
	const auto flip = object->flipped ? -1 : 1;

	if (cmd_info == nullptr)
		return;

	for (auto i = 0; i < cmd_info->hitbox_count; i++)
	{
		const auto hitbox = cmd_info->hitboxes[i];

		if (hitbox == nullptr)
			continue;

		graphics::draw_box(
			device,
			object->pos_x + object->parent_pos_x + hitbox->x1 * game::COORD_TO_PIXEL * flip,
			object->pos_y + object->parent_pos_y + hitbox->y1 * game::COORD_TO_PIXEL,
			object->pos_x + object->parent_pos_x + hitbox->x2 * game::COORD_TO_PIXEL * flip,
			object->pos_y + object->parent_pos_y + hitbox->y2 * game::COORD_TO_PIXEL,
			D3DCOLOR_ARGB(1, 255, 0, 0),
			D3DCOLOR_ARGB(255, 255, 0, 0));
	}
}

// IDirect3DDevice9::EndScene
using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
EndScene_t orig_EndScene;
constexpr auto EndScene_idx = 42;

HRESULT __stdcall hook_EndScene(IDirect3DDevice9 *device)
{
	graphics::render_start(device);

	// Draw hitboxes of each type for all entities.
	const auto iterate_objects = [&](auto func)
	{
		for (const auto &entry : *game::object_list)
		{
			if (entry.item == nullptr || (entry.flags & 4) || !entry.item->IsValid())
				continue;

			if (!(entry.item->flags3 & game::OF3_DORMANT))
				func(device, entry.item);
		}
	};

	iterate_objects(draw_pushbox);
	iterate_objects(draw_hurtboxes);
	iterate_objects(draw_hitboxes);

	graphics::render_end(device);

	return orig_EndScene(device);
}

using RunFrame_t = decltype(game::RunFrame);
RunFrame_t orig_RunFrame;

int __fastcall hook_RunFrame(void *thisptr)
{
	static auto pressed = false;

	if (GetAsyncKeyState(VK_F1) & 0x8000)
	{
		if (!pressed)
			rollback.save_game_state();

		pressed = true;
	}
	else if (GetAsyncKeyState(VK_F2) & 0x8000)
	{
		if (!pressed)
			rollback.load_game_state();

		pressed = true;
	}
	else if (GetAsyncKeyState(VK_F3) & 0x8000)
	{

		if (!pressed)
		{
			for (auto i = 0; i < 10; i++)
				orig_RunFrame(thisptr);
		}

		pressed = true;
	}
	else
	{
		pressed = false;
	}
	rollback.save_game_state();
	
	return orig_RunFrame(thisptr);
}


using RenderChoice_t = decltype(game::RenderChoice);
RenderChoice_t orig_RenderChoice;
int count = 0;

int __fastcall hook_RenderChoice(DWORD t, int a) 
{
	int* f = (int*)(util::get_base_address() + SKIP_FRAME_FLAG);
	static auto once = false;

	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		if (!once)
		{
			

			rollback.rollback_n(4);
		
			count = 4;
		
		}
			
		
		once = true;
	}
	else 
	{
		once = false;
	}


	if (count > 0) 
	{
		count--;
		*f = 0x2;
	}
	else
	{
		*f = 0x0;
	}


	
	
	return orig_RenderChoice(t, a);
}

using MainLoop_t = decltype(game::MainLoop);
MainLoop_t orig_MainLoop;

int __fastcall hook_MainLoop(void* t, void* nothing)
{
	int* game_state = (int*)(util::get_base_address() + GAME_STATE);
	
	controllers.updateInputs();
	
	if (*game_state == 2 || *game_state == 10 || *game_state == 3 || *game_state == 5)
	{
		int* world_timer = (int*)(util::get_base_address() + 0x2E6F40);
		if (*world_timer % 3 == 0) 
		{
			*(int*)(util::get_base_address() + 0x323E04) = 4;
		}
		else
		{
			*(int*)(util::get_base_address() + 0x323E04) = 0;
		}
			
		if (*game_state == 5) 
		{
			int* menuCursor = (int*)(util::get_base_address() + MAIN_MENU_STATE);
			*menuCursor = 0x2;
		}
		
		
		
		
		int* f = (int*)(util::get_base_address() + SKIP_FRAME_FLAG);
		*f = 0x2;
	}

	//TODO Set player character based on selection
	if (*game_state == 22) {
		if (GetAsyncKeyState(VK_RETURN) & 0x8000) {

		}
		*(int*)(util::get_base_address() + 0x323F4C) = 5;
		
	}
	return orig_MainLoop();
}


using VSScreen_t = decltype(game::MainLoop);
VSScreen_t orig_VSScreen;

signed int __fastcall hook_VSScreen()
{
	printf("Accessed");
	return orig_VSScreen();
}



using Controller_t = decltype(game::Controller);
Controller_t orig_Controller;

//DIsable everywhere but retry and char select
int __fastcall hook_Controller(int thing)
{
	int* game_state = (int*)(util::get_base_address() + GAME_STATE);
	if (*game_state == 2 || *game_state == 22 || *game_state == 12) {
		return orig_Controller(thing);
	}
	return 0;
}

using ControllerM_t = decltype(game::ControllerM);
ControllerM_t orig_ControllerM;
unsigned int __fastcall hook_ControllerM(void* th, void* nothing)
{
	return 0;
}

using DisableGameControls1_t = decltype(game::DisableGameControls1);
DisableGameControls1_t orig_DisableGameControls1;
int __fastcall hook_DisableGameControls1(void* th, void* nothing, int o)
{

	return 0;
}

using DisableGameControls2_t = decltype(game::DisableGameControls2);
DisableGameControls2_t orig_DisableGameControls2;
int __fastcall hook_DisableGameControls2(void* th, void* nothing, int o)
{

	return 0;
}

using GameTypeSelection_t = decltype(game::GameTypeSelection);
GameTypeSelection_t orig_GameTypeSelection;
int __fastcall hook_GameTypeSelection(void* th, void* nothing, int o)
{
	
	//Always makes it 2 player controls
	int i = orig_GameTypeSelection(o);
	*(int*)(util::get_base_address() + 0x35FCD0) = 1;
	return i;
}

using SideCheck_t = decltype(game::SideCheck);
SideCheck_t orig_SideCheck;
int limit = 1;
unsigned int __stdcall hook_SideCheck(int o)
{
	
	__asm pushad

	printf("%d\n", o);
	if (limit) {
		
		if (o == 0) {
			controllers.invert = false;
		}
		else {
			controllers.invertDirection();
		}
	}
	limit ^= 1;
	__asm popad
	return  orig_SideCheck(o);
}


void disableGameControls()
{
	BYTE nop2[] = {0x90, 0x90};

	memcpy((void*)(util::get_base_address() + 0x67BBF), &nop2, 2);
	memcpy((void*)(util::get_base_address() + 0x67C4F), &nop2, 2);
}




BOOL WINAPI DllMain(
	_In_ HINSTANCE hinstDLL,
	_In_ DWORD     fdwReason,
	_In_ LPVOID    lpvReserved
	)
{
	if (fdwReason != DLL_PROCESS_ATTACH)
		return FALSE;

	// Hook IDirect3DDevice9::EndScene.
	const auto **dev_vtable = *(const void***)(util::sigscan(
		"d3d9.dll",
		"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86",
		"xx????xx????xx") + 0x2);

	
	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	controllers.initControllers();
	//disableGameControls();

	orig_EndScene = (EndScene_t)(DetourFunction((BYTE*)(dev_vtable[EndScene_idx]), (BYTE*)(hook_EndScene)));

	orig_RunFrame = (RunFrame_t)(DetourFunction((BYTE*)(game::RunFrame), (BYTE*)(hook_RunFrame)));


	orig_MainLoop = (MainLoop_t)(DetourFunction((BYTE*)(game::MainLoop), (BYTE*)(hook_MainLoop)));

	orig_RenderChoice = (RenderChoice_t)(DetourFunction((BYTE*)(game::RenderChoice), (BYTE*)(hook_RenderChoice)));

	orig_VSScreen = (VSScreen_t)(DetourFunction((BYTE*)(game::VSScreen), (BYTE*)(hook_VSScreen)));

	orig_Controller = (Controller_t)(DetourFunction((BYTE*)(game::Controller), (BYTE*)(hook_Controller)));
	
	orig_GameTypeSelection = (GameTypeSelection_t)(DetourFunction((BYTE*)(game::GameTypeSelection), (BYTE*)(hook_GameTypeSelection)));
	//orig_ControllerM = (ControllerM_t)(DetourFunction((BYTE*)(game::ControllerM), (BYTE*)(hook_ControllerM)));
	orig_SideCheck = (SideCheck_t)(DetourFunction((BYTE*)(game::SideCheck), (BYTE*)(hook_SideCheck)));

	orig_DisableGameControls1 = (DisableGameControls1_t)(DetourFunction((BYTE*)(game::DisableGameControls1), (BYTE*)(hook_DisableGameControls1)));
	orig_DisableGameControls2 = (DisableGameControls2_t)(DetourFunction((BYTE*)(game::DisableGameControls2), (BYTE*)(hook_DisableGameControls2)));
	return TRUE;
}