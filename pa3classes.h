#include "geom.h"

// The bulk of the hack leverages this Player class
// It was reversed using ReClass.net and Ghidra

class Player;
class ILocalPlayer;
class IItem;

// These are the functions have have been reversed from the ClientWorld
// class. None of them are used, but could be.
class ClientWorld
{
public:
	char pad_0004[48]; //0x0004

	virtual void Tick(float);
    virtual bool HasLocalPlayer();
    virtual void* GetLocalPlayer();
    virtual bool IsAuthority();
	virtual void AddLocalPlayer(Player*, ILocalPlayer*);
	virtual void AddRemotePlayer(Player*);
	virtual void AddRemotePlayerWithId(uint32_t, Player*);
	virtual void RemovePlayer(Player*);
	virtual void Use(Player*, void*);
	virtual void Activate(Player*, void*);
	virtual void Reload(Player*);
	virtual void Jump(bool);
	virtual void Sprint(bool);
	virtual void FireRequest(bool);
	virtual void TransitionToNPCState(Player*, const std::string&);
	virtual void BuyItem(Player*, void*, void*, uint32_t);
	virtual void SellItem(Player*, void*, void*, uint32_t);
	virtual void Respawn(Player*);
	virtual void Teleport(Player*, const std::string&);
	virtual void Chat(void* player, std::string& msg);
	virtual void Function20();
	virtual void Function21();
	virtual void Function22();
	virtual void Function23();
	virtual void Function24();
	virtual void Function25();
	virtual void Function26();
	virtual void Function27();
}; //Size: 0x0034


class Player
{
public:
	char pad_0004[4]; //0x0004
	// One of the most important things to note here when doing this on Windows with MSVC.
	// Need to make sure _ITERATOR_DEBUG_LEVEL=0 when doing dll injection as the application/dlls are generally in release mode
	// and when the iterator level is > 0, MSVC adds additional things to std classes, which causes a mismatch.
	// When the iterator_debug_level is > 0, the size of "std::string" is actually 28bytes, not the expected
	// 24 bytes. This size difference causes all sorts of problems
	std::string m_playerName; //0x0008
	std::string m_teamName; //0x0020
	int8_t m_avatarIndex; //0x0038
	char pad_0039[55]; //0x0039
	uint8_t N000001BD; //0x0070
	uint8_t m_pvpEnabled; //0x0071
	uint8_t m_togglePVP; //0x0072
	uint8_t N0000023C; //0x0073
	char pad_0074[172]; //0x0074
	float m_walkSpeed; //0x0120
	float m_jumpSpeed; //0x0124
	char pad_0124[34]; //0x0128
	class LocalPlayer* ptrToLocalPlayer; //0x0148
	char pad_014C[96]; //0x014C

	virtual void AddRef();
	virtual void Function1();
	virtual ILocalPlayer* GetLocalPlayer() const;
	virtual const char* GetPlayerName();
	virtual const char* GetTeamName();
	virtual uint8_t GetAvatarIndex();
	// Could never figure out what this function did the assembly
	// was effectely ret [ecx + 0x3c], which doesn't make sense as that's part
	// of the std::string
	virtual void Unknown_ecx_3c();
	virtual bool IsPvPDesired();
	virtual void SetPvPDesired(bool);
	virtual void GetInventory();
	virtual void Function10();
	virtual void Function11();
	virtual void Function12();
	virtual void Function13();
	virtual void Function14();
	virtual void Function15();
	virtual void Function16();
	virtual void Function17();
	virtual void Function18();
	virtual void Function19();
	virtual void Function20();
	virtual void Function21();
	virtual void Function22();
	virtual void Function23();
	virtual void Function24();
	virtual void Function25();
	virtual void Function26();
	virtual void Function27();
	virtual void Function28();
	virtual void Function29();
	virtual void Function30();
	virtual void Function31();
	virtual void Function32();
	virtual void Function33();
	virtual void Function34();
	virtual void Function35();
	virtual void Function36();
	virtual void Function37();
	virtual void Function38();
	virtual void Function39();
	virtual void Function40();
	virtual void Function41();
	virtual void Function42();
	virtual float GetWalkingSpeed();
	virtual float GetSprintMultiplier();
	virtual float GetJumpSpeed();
	virtual float GetJumpHoldTime();
	virtual bool CanJump();
	virtual void SetJumpState(bool);
	virtual void SetSprintState(bool);
	virtual void SetFireRequestState(bool);
	virtual void TransitionToNPCState(const char*);
	virtual void BuyItem(void* ptr, IItem* item, uint32_t val);
	virtual void SellItem(void* ptr, IItem* item, uint32_t val);
	virtual void EnterRegion(const char*);
	virtual void Respawn();
	virtual void Teleport(const char*);
	virtual void Chat(const char*);
}; //Size: 0x01AC

class ILocalPlayer
{
public:
	char pad_0000[4]; //0x0000
	class IActor* ptrToIActor; //0x0004
	char pad_0008[4]; //0x0008
}; //Size: 0x000C

class IActor
{
public:
	char pad_0000[276]; //0x0000
	class IUE4Actor* ptrToUE4Actor; //0x0114
	char pad_0118[244]; //0x0118
	class allGameObjects* ptrToGameObjects; //0x020C
	char pad_0210[20]; //0x0210

}; //Size: 0x0224

class IUE4Actor
{
public:
	char pad_0000[24]; //0x0000
	class IActor* ptrToIActor; //0x0018
	char pad_001C[116]; //0x001C
	Vector3 m_position; //0x0090
	char pad_009C[100]; //0x009C
}; //Size: 0x0100

class allGameObjects
{
public:
	char pad_0000[12]; //0x0000
	class UE4ActorCamera* ptrToCameraUE4ActorCamera; //0x000C
}; //Size: 0x0010

class UE4ActorCamera
{
public:
	char pad_0000[24]; //0x0000
	class IActor* N00000AA6; //0x0018
	char pad_001C[100]; //0x001C
	Vector4 m_quaternion; //0x0080
	Vector3 m_cameraPosition; //0x0090
	char pad_009C[20]; //0x009C

}; //Size: 0x00B0
