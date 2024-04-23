#include "RoomManager.h"
void RoomManager::RegisterRoom(Room* pRoom)
{
	EXCLUSIVE_LOCK;
	_pRoomList.push_back(pRoom);
}

void RoomManager::DeregisterRoom(Room* pRoom)
{
	EXCLUSIVE_LOCK;
	_pRoomList.remove(pRoom);
}

void RoomManager::UpdateRooms()
{
	while (bShutDown==false)
	{
		{
			LONG64 currentTime = GetTickCount64();
			EXCLUSIVE_LOCK;
			for (Room* pRoom : _pRoomList)
			{
				if (pRoom->_bUpdating == false)
				{
					LONG64 timeDiff = currentTime - pRoom->_prevUpdateTime;
					if (timeDiff >= _updatePeriod)
					{
						pRoom->DoAsync(&Room::UpdateJob);
					}
				}
			}
		}
		Sleep(_updatePeriod / 4);
	}
}

RoomManager::RoomManager():_roomThread(&RoomManager::UpdateRooms,this)
{
}

RoomManager::~RoomManager()
{
	bShutDown = true;
	_roomThread.join();
}
