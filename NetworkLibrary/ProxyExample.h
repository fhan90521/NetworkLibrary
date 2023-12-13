#pragma once
#include "NetLibrary.h"
namespace RPC
{
	class Proxy
	{
	private:
		//netlibrary pointer
	public:
		void ScCreateMyCharacter(Session* pSession, int& id, char& eye_dir, short& x, short& y, char& hp); 
		void ScCreateOtherCharacter(Session* pSession, int& id, char& eye_dir, short& x, short& y, char& hp);
		void ScDeleteCharacter(Session* pSession, int& id);
		void ScMoveStart(Session* pSession, int& id, char& dir, short& x, short& y);
		void ScMoveStop(Session* pSession, int& id, char& eye_dir, short& x, short& y);
		void ScAttack1(Session* pSession, int& id, char& eye_dir, short& x, short& y);
		void ScAttack2(Session* pSession, int& id, char& eye_dir, short& x, short& y);
		void ScAttack3(Session* pSession, int& id, char& eye_dir, short& x, short& y);
		void ScDamage(Session* pSession, int& attackID, int& damageID, char& damageHP);
	};
}