#include "j1GameLayer.h"
#include "j1App.h"
#include "j1EntityManager.h"
#include "j1Gui.h"
#include "j1CollisionManager.h"
#include "p2Point.h"
#include "j1Render.h"
#include "Enemy.h"
#include "Player.h"
#include "Object.h"
#include "j1PerfTimer.h"
#include "p2Log.h"
#include "j1SceneManager.h"
#include "Scene.h"
#include "j1Map.h"
#include "HUD.h"
#include "j1QuestManager.h"
#include "j1Audio.h"
#include "j1CutSceneManager.h"


// just for temporal wall collider
#include "Entity.h"



j1GameLayer::j1GameLayer() : j1Module()
{
	name = ("game");
	em = new j1EntityManager();
	hud = new Hud();
}

j1GameLayer::~j1GameLayer()
{
	RELEASE(hud);
	RELEASE(em);
}

bool j1GameLayer::Awake(pugi::xml_node& conf)
{
	em->Awake(conf);
	hud->Awake(conf);
	return true;
}

// Called before the first frame
bool j1GameLayer::Start()
{
	bool ret = true;

	active = true;
	hud->Start();


	return ret;
}

//preUpdate
bool j1GameLayer::PreUpdate()
{
	if (!pause && App->sceneM->currentScene->inGame)
		em->PreUpdate();
	//hud->PreUpdate();

	return true;
}

//update
bool j1GameLayer::Update(float dt)
{
	bool ret = true;
	

	if (!App->sceneM->currentScene->inGame)
	{
		hud->Update(dt);
		return ret;
	}
	if(!pause)
		em->Update(dt);
	
	ret = hud->Update(dt);

	App->render->CameraFollow(em->player->currentPos);

	
	if (App->input->GetKey(SDL_SCANCODE_GRAVE) == KEY_DOWN)
		debug = !debug;

	if(debug)
		DebugFunc();
		

	return ret;
}

//postUpdate
bool j1GameLayer::PostUpdate()
{
	if (App->sceneM->currentScene->inGame)
	em->PostUpdate(); 
	//hud->PostUpdate();

	return true;
}

// Called before quitting
bool j1GameLayer::CleanUp()
{
	active = false;

	hud->CleanUp();
	em->CleanUp();

	return true;
}

void j1GameLayer::PickObject(Object * object)
{
	switch (object->objectType)
	{
	case GREEN_RUPEE:
	case BLUE_RUPEE:
	case RED_RUPEE:
		em->player->rupees += ((Rupee*)object)->rupeeValue;
		App->audio->PlayFx(22);
		break;

	case LIFEHEART:
		if (em->player->life < em->player->maxLife)
				em->player->life++;
		App->audio->PlayFx(2);
		
		break;
	case HEART_CONTAINER:
		em->player->maxLife++;
		em->player->life = em->player->maxLife;
		em->player->bonusLife++;
		App->audio->PlayFx(29);
		break;
	case BOMB_DROP:
		if (em->player->bombs < em->player->maxBombs)
		{
			if ((em->player->bombs + 10) > em->player->maxBombs)
			{
				em->player->bombs = em->player->maxBombs;
			}
			else
			{
				em->player->bombs += 10;
			}
			
		}
			
		break;

	case ARROW_BUY:
	case ARROW_DROP:
		if (em->player->arrows < em->player->maxArrows)
			if ((em->player->arrows + 5) > em->player->maxArrows)
			{
				em->player->arrows = em->player->maxArrows;
			}
			else
			{
				em->player->arrows += 5;
			}

		break;

	case BOOK:
		em->player->ableToSpin = true;
		App->audio->PlayFx(23);
		break;

	case BOSS_KEY:
		em->player->bossKeyEquipped = true;
		App->audio->PlayFx(37);
		break;

	case LIFE_POTION:
		if (object->objectType == LIFE_POTION)
		{
			if (em->player->lifePotions < em->player->maxLifePotions)
				em->player->lifePotions++;
		}

	case STAMINA_POTION:
		if (object->objectType == STAMINA_POTION)
		{
			if (em->player->staminaPotions < em->player->maxStaminaPotions)
				em->player->staminaPotions++;
		}
		
	case BOMB_SAC:
		if (object->objectType == BOMB_SAC)
		{
			if (object->objectType == BOMB_SAC)
				em->player->bombs += 5;
		}

	default:
		em->player->inventory.push_back(object->objectType);
		App->audio->PlayFx(23);
		break;

	}
	
		if (object->price > 0)
	{
		return;
	}

	
	if (em->player->pickedObject)
	{
		em->player->pickedObject->life = -1;
	}

	em->player->pickedObject = object;
	em->player->pickedObject->col->active = false;
	em->player->showObjectTimer.Start();
	
	
}

void j1GameLayer::BuyObject(Object * object)
{	
	if (buy_timer.Read() > 700 && em->player->rupees >= object->price)
	{
		if ((object->objectType == LIFE_POTION && em->player->lifePotions < em->player->maxLifePotions) ||
			(object->objectType == ARROW_BUY && em->player->arrows < em->player->maxArrows) ||
			(object->objectType == BOMB_DROP && em->player->bombs < em->player->maxBombs) ||
			(object->objectType == STAMINA_POTION && em->player->staminaPotions < em->player->maxStaminaPotions))
		{
			PickObject(object);
			em->player->rupees -= object->price;
			buy_timer.Start();
			App->audio->PlayFx(36);
		}
	}
}


void j1GameLayer::StopLoading()
{
	hud->loaded_game->Set_Active_state(false);
}


//Save game
bool j1GameLayer::Save(pugi::xml_node &data) const
{
	//Save player data
	pugi::xml_node player = data.append_child("player");
	
	pugi::xml_node pos = player.append_child("position");
	pos.append_attribute("x") = em->player->currentPos.x;
	pos.append_attribute("y") = em->player->currentPos.y;
	player.append_attribute("life") = em->player->life;
	player.append_attribute("maxLife") = em->player->maxLife;
	player.append_attribute("direction") = em->player->currentDir;
	player.append_attribute("age") = em->player->age;
	player.append_attribute("rupees") = em->player->rupees;
	player.append_attribute("arrows") = em->player->arrows;
	player.append_attribute("bombs") = em->player->bombs;
	player.append_attribute("spin") = em->player->ableToSpin;
	player.append_attribute("bonus") = em->player->bonusLife;
	player.append_attribute("key") = em->player->bossKeyEquipped;

	pugi::xml_node inventory = player.append_child("inventory");
	for (list<OBJECT_TYPE>::iterator it = em->player->inventory.begin(); it!= em->player->inventory.end(); it++)
	{
		inventory.append_child("item").append_attribute("type") = (*it);
	}

	//Save current map
	data.append_child("current_map").append_attribute("name") = App->sceneM->currentScene->name.c_str();
	
	pugi::xml_node chests = data.append_child("open_chests").append_child(App->sceneM->currentScene->name.c_str());

	
	for (int i = 0; i < openChests.size(); i++)
		chests.append_child("chest").append_attribute("id") = openChests[i];

	
	
	return true;
}

//Load game
bool j1GameLayer::Load(pugi::xml_node& data)
{

	if (em->player == nullptr)
		em->player = em->CreatePlayer(App->game->playerX, App->game->playerY, YOUNG);
	

	em->player->currentPos.x = em->player->lastPos.x = data.child("player").child("position").attribute("x").as_int();
	em->player->currentPos.y = em->player->lastPos.y = data.child("player").child("position").attribute("y").as_int();
	em->player->life = data.child("player").attribute("life").as_int();
	em->player->rupees = data.child("player").attribute("rupees").as_int();
	em->player->arrows = data.child("player").attribute("arrows").as_int();
	em->player->bombs = data.child("player").attribute("bombs").as_int();
	em->player->maxLife = data.child("player").attribute("maxLife").as_int();

	em->player->changeAge = data.child("player").attribute("age").as_int();
	em->player->ableToSpin = data.child("player").attribute("spin").as_bool();
	em->player->bonusLife = data.child("player").attribute("bonus").as_int();
	em->player->bossKeyEquipped = data.child("player").attribute("key").as_bool();

	em->player->inventory.clear();

	for (pugi::xml_node it = data.child("player").child("inventory").first_child(); it; it = it.next_sibling("item"))
	{
		em->player->inventory.push_back((OBJECT_TYPE)it.attribute("type").as_int());
	}

	string dest = data.child("current_map").attribute("name").as_string();

	openChests.clear();
	for (pugi::xml_node it = data.child("open_chests").child(dest.c_str()).first_child(); it; it = it.next_sibling("chest"))
	{
		openChests.push_back(it.attribute("id").as_int(-1));
	}

	if (em->player->dead)
		em->player->life = em->player->maxLife;

	App->sceneM->RequestSceneChange(em->player->currentPos, dest.c_str() , (DIRECTION)data.child("player").attribute("direction").as_int());

	em->player->col->active = true;

	em->player->UpdateCollider();

	em->player->linearMovement = { 0, 0 };
	em->player->sprite->tint = { 255, 255, 255, 255 };
	em->player->actionState = IDLE;

	return true;
}

bool j1GameLayer::DebugFunc()
{
	iPoint mousePos;
	App->input->GetMousePosition(mousePos.x, mousePos.y);
	

	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		em->ActiveObject(mousePos.x, mousePos.y, BOMB);
	}

	mousePos = App->map->WorldToMap(mousePos.x, mousePos.y);

	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN)
	{
		iPoint mousePos;
		App->input->GetMousePosition(mousePos.x, mousePos.y);
		
		em->player->currentPos = mousePos;
	}

	if (App->input->GetKey(SDL_SCANCODE_F2))
	{
		App->sceneM->RequestSceneChange({ 47 * 8,220 * 8 }, "bossRoom", D_DOWN);
		em->player->ChangeAge(ADULT);
		App->SaveGame("saves.xml");
		em->player->life = em->player->maxLife;
	}

	if (App->input->GetKey(SDL_SCANCODE_F))
	{
		App->sceneM->RequestSceneChange({ 100,100 }, "kakarikoForest", D_DOWN);
		App->SaveGame("saves.xml");
		em->player->life = em->player->maxLife;
	}

	//active Cutscenes
	if (App->input->GetKey(SDL_SCANCODE_LCTRL))
	{
		if (App->input->GetKey(SDL_SCANCODE_1))
			App->cutsceneM->StartCutscene(1);

		if(App->input->GetKey(SDL_SCANCODE_2))
			App->cutsceneM->StartCutscene(2);

		if (App->input->GetKey(SDL_SCANCODE_3))
			App->cutsceneM->StartCutscene(3);

		if (App->input->GetKey(SDL_SCANCODE_4))
			App->cutsceneM->StartCutscene(4);

		return true;
	}

	if (App->input->GetKey(SDL_SCANCODE_L))
		em->player->life = em->player->maxLife;

	if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)
	em->player->changeAge = 0;
	else if (App->input->GetKey(SDL_SCANCODE_3) == KEY_DOWN)
	em->player->changeAge = 2;
	else if (App->input->GetKey(SDL_SCANCODE_4) == KEY_DOWN)
	em->CreateEnemy(1, GREEN_SOLDIER, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_5) == KEY_DOWN)
	em->CreateEnemy(1, RED_SOLDIER, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_6) == KEY_DOWN)
	em->CreateEnemy(1, OCTOROK, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_7) == KEY_DOWN)
	em->CreateEnemy(1, WIZDROVE, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_8) == KEY_DOWN)
	em->CreateEnemy(1, EYEGORE, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_9) == KEY_DOWN)
	em->CreateEnemy(1, TEKTITE, mousePos.x, mousePos.y, vector<iPoint>());
	else if (App->input->GetKey(SDL_SCANCODE_N) == KEY_DOWN)
	em->ActiveObject(em->player->currentPos.x, em->player->currentPos.y, STAMINA_POTION);
	else if (App->input->GetKey(SDL_SCANCODE_M) == KEY_DOWN)
	em->player->rupees += 5;

	return true;
}


bool j1GameLayer::On_Collision_Callback(Collider * c1, Collider * c2 , float dt)
{
	if (c1->type == COLLIDER_PLAYER && (c2->type == COLLIDER_NPC || c2->type == COLLIDER_BUSH || c2->type == COLLIDER_CHEST))
	{
		if (!c1->parent)
			return false;
		iPoint Movement;
		if (abs(c1->rect.x - c2->rect.x) < abs(c1->rect.y - c2->rect.y))
		{
			if (c1->parent->currentPos.y > c2->parent->currentPos.y)
				Movement = {0, 1};
			else Movement = { 0, -1 };
		}
		else
		{
			if (c1->parent->currentPos.x > c2->parent->currentPos.x)
				Movement = { 1, 0 };
			else Movement = { -1, 0 };
		}

		if (c1->parent->actionState == DODGING)
			em->player->dodgeDir.SetToZero();
		
		c1->parent->Move(SDL_ceil(c1->parent->speed*dt)*Movement.x, SDL_ceil(c1->parent->speed*dt)*Movement.y);
			

		if (c2->type == COLLIDER_NPC && App->cutsceneM->CutsceneReproducing() == false)
		{
			Npc* tmp = (Npc*)c2->parent;
			if (tmp->npcType == NPC_ZELDA)
				App->cutsceneM->StartCutscene(2);
		}

		return true;
	}
	if (c1->type == COLLIDER_PLAYER && c2->type == COLLIDER_WOOD_DOOR)
	{
		if (em->player->bossKeyEquipped)
		{
			App->audio->PlayFx(35);
			c2->parent->life = -1;
		}
		else c1->parent->currentPos = c1->parent->lastPos;
		return true;
	}
	if (c1->type == COLLIDER_PLAYER && c2->type == COLLIDER_WALL)
	{
		c1->parent->currentPos = c1->parent->lastPos;
		return true;
	}
 	if (c1->type == COLLIDER_PLAYER && (c2->type == COLLIDER_ENEMY || c2->type == COLLIDER_OCTOSTONE || c2->type == COLLIDER_MAGIC_SLASH || c2->type == COLLIDER_BOLT|| c2->type == COLLIDER_BOMB_EXPLOSION || c2->type == COLLIDER_ZELDA_ARROW) )
	{
		//When link is adult, empuja enemigos
		if (em->player->actionState == DODGING && em->player->age == ADULT )
		{
			//TODO: Arrange this
			/*if (((Enemy*)(c2->parent))->enemyState != STEP_BACK)
			{ 
				((Enemy*)(c2->parent))->enemyState = STEP_BACK;
				c2->parent->pushedBackTimer.Start();
			}*/
		}
		else if (em->player->invulnerable == false && em->player->dodging == false && em->player->life > 0)
		{
  			if (em->player != nullptr && c2->parent != nullptr)
			  {
				em->player->GetHit(c2->parent);
			  }
		}

		return true;
	}

	if (c1->type == COLLIDER_OCTOSTONE && c2->type == COLLIDER_LINK_SWORD)
	{
		if (c1->parent == nullptr)
			return false;
		else c1->parent->life = -1;
	}

	if (c1->type == COLLIDER_ENEMY)
	{

		if (c1->parent == nullptr)
			return false;
		
		if (c2->type == COLLIDER_LINK_SWORD)
		{
			
			if (((Enemy*)(c1->parent))->enemyState != STEP_BACK && ((Enemy*)(c1->parent))->enemyState != DODGING_LINK)
			{
				c1->parent->GetHit(em->player);
			}
			return true;
		}

		if (c2->type == COLLIDER_ENEMY)
		{
			if (c2->parent == nullptr)
				return false;

			switch (((Enemy*)(c2->parent))->enemyState)
			{
			case KEEP_DISTANCE:
			case CHASING:
				iPoint Movement;
				if (c1->parent->currentPos.DistanceTo(c2->parent->currentPos) > 10)
					break;

				if (c1->rect.x < c2->rect.x)
					Movement.x = 1;
				else Movement.x = -1;;

				if (c1->rect.y < c2->rect.y)
					Movement.y = 1;
				else Movement.y = -1;

				c2->parent->Move(SDL_ceil(c2->parent->speed*dt)*Movement.x, SDL_ceil(c2->parent->speed*dt)*Movement.y);
				break;
		}
			return true;
	}

		if (c2->type == COLLIDER_LINK_ARROW)
		{
			c2->parent->life = -1;

			if (((Enemy*)(c1->parent))->enemyState != STEP_BACK)
			{
				c1->parent->GetHit(c2->parent);
			}
			return true;
		}

		if (c2->type == COLLIDER_WALL || c2->type == COLLIDER_WOOD_DOOR)
		{
			c1->parent->currentPos = c1->parent->lastPos;
			return true;
		}
	}

	if (c1->type == COLLIDER_NPC)
	{
		if (c2->type == COLLIDER_LINK_SWORD && em->player->actionState == ATTACKING)
		{
			Npc* tmp = (Npc*)c1->parent;
			if (tmp->npcType != NPC_ZELDA)
			  em->player->toTalk = (Npc*)c1->parent;
		}
		return true;
	}

	
	if(c2->type == COLLIDER_PICKABLE_ITEM)
	{
		PickObject((Object*)c2->parent);
		return true;
	}

	if (c1->type == COLLIDER_BUYABLE_ITEM && c2->type == COLLIDER_LINK_SWORD) //	Shop
	{
		//	Shop
		BuyObject((Object*)c1->parent);
		return true;
	}

	if (c1->type == COLLIDER_BUSH && c2->type == COLLIDER_LINK_SWORD)
	{
		c1->parent->life = -1;
		return true;
	}

	if (c1->type == COLLIDER_CHEST && c2->type == COLLIDER_LINK_SWORD && c1->parent->actionState == CLOSE)
	{
		openChests.push_back(c1->parent->id);
		c1->parent->actionState = OPEN;
		App->audio->PlayFx(26);
		iPoint objPos = App->map->WorldToMap(c1->parent->currentPos.x, c1->parent->currentPos.y);
		Object* obj = em->CreateObject(1, objPos.x, objPos.y, ((Chest*)c1->parent)->objectInside);
		PickObject(obj);
		return true;
	}


	if (c1->type == COLLIDER_PLAYER && c2->type == COLLIDER_EVENT)
	{
		if (App->quest->TriggerCollisionCallback(c2) == false)
			App->quest->StepCollisionCallback(c2);
		return true;
	}

	if (c1->type == COLLIDER_BOMB_EXPLOSION && c2->type == COLLIDER_ENEMY )
	{
		if(c2->parent && c1->parent)
			c2->parent->GetHit(c1->parent);
		return true;
	}

	if (c1->type == COLLIDER_BOMB_EXPLOSION && (c2->type == COLLIDER_WALL||c2->type == COLLIDER_BUSH))
	{
		c2->parent->life = -1;
		return true;
	}

	if (c1->type == COLLIDER_BOMB && (c2->type == COLLIDER_ENEMY || c2->type == COLLIDER_BOMB_EXPLOSION))
	{
		((Bomb*)c1->parent)->ExplodeBomb();
		return true;
	}

	if (c1->type == COLLIDER_ZELDA_ARROW && c2->type == COLLIDER_WALL)
	{
		c1->parent->life = -1;
		return true;
	}

	if(c1->type== COLLIDER_INTERRUPTOR && c2->type == COLLIDER_LINK_ARROW)
	{
		((Interruptor*)c1->parent)->on = true;
		c1->parent->actionState = ON;
		c2->parent->life = -1;

		if (!App->quest->TriggerInterruptorCallback((Object*)c1->parent))
			App->quest->StepInterruptorCallback((Object*)c1->parent);
	}
	return true;
}
