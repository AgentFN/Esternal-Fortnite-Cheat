#include "DirectOverlay.h"
#include <D3dx9math.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <list>

#define M_PI 3.14159265358979323846264338327950288419716939937510


// LOGO
bool Logo = true;

// Aimbot
bool Menu1 = true;
bool Aimbot = false;
bool DrawFov = false;
bool FOVDOT = false;

bool Head = false;
bool Chest = false;

// Smooth
float Aim_Speed = 1.0f;
const int Max_Aim_Speed = 4;
const int Min_Aim_Speed = 0;

float aimfov = 15.0f;

// Aimbot KEYS
bool Menu3 = false;

//bool CapsLock = false;
bool LButton = false;
bool RButton = false;

// Visuals
bool Menu2 = false;
bool SkeletonESP = false;
bool BoxESP = false;
bool LineESP = false;
bool DistanceESP = false;
bool LockLine = false;
bool Reticle = false;
bool FilledBoxESP = false;
bool EnemyTAG = false;
bool HeadDotESP = false;
bool CornerESP = false;
bool CircleESP = false;
//bool CircleESPFilled = false;

// Int & Float Shit
int width;
int height;
int localplayerID;
float FovAngle;

DWORD processID;
HWND hwnd = NULL;

HANDLE DriverHandle;
uint64_t base_address;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;

Vector3 localactorpos;
Vector3 Localcam;

bool isaimbotting;
DWORD_PTR entityx;

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x4A8);

	if (bonearray == NULL)
	{
		bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x4A8 + 0x10);
	}

	return read<FTransform>(DriverHandle, processID, bonearray + (index * 0x30));
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(DriverHandle, processID, mesh + 0x1C0);

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

extern Vector3 CameraEXT(0, 0, 0);

Vector3 ProjectWorldToScreen(Vector3 WorldLocation, Vector3 camrot)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);
	Vector3 Camera;

	auto chain69 = read<uintptr_t>(DriverHandle, processID, Localplayer + 0xa8);
	uint64_t chain699 = read<uintptr_t>(DriverHandle, processID, chain69 + 8);

	Camera.x = read<float>(DriverHandle, processID, chain699 + 0x7F8);
	Camera.y = read<float>(DriverHandle, processID, Rootcomp + 0x12C);

	float test = asin(Camera.x);
	float degrees = test * (180.0 / M_PI);
	Camera.x = degrees;

	if (Camera.y < 0)
		Camera.y = 360 + Camera.y;

	D3DMATRIX tempMatrix = Matrix(Camera);
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	uint64_t chain = read<uint64_t>(DriverHandle, processID, Localplayer + 0x70);
	uint64_t chain1 = read<uint64_t>(DriverHandle, processID, chain + 0x98);
	uint64_t chain2 = read<uint64_t>(DriverHandle, processID, chain1 + 0x130);

	Vector3 vDelta = WorldLocation - read<Vector3>(DriverHandle, processID, chain2 + 0x10);
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float zoom = read<float>(DriverHandle, processID, chain699 + 0x590);

	FovAngle = 80.0f / (zoom / 1.19f);
	float ScreenCenterX = width / 2.0f;
	float ScreenCenterY = height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	CameraEXT = Camera;

	return Screenlocation;
}

int SelectedItem = 0;
int MaxItem = 5;

void V2Menu()
{
	if (Logo)
	{
		DrawString(_xor_("Teagy's Skill").c_str(), 22, width / 2, 2, 0.f, 0.f, 0.f, 0.8f);
	}

	if (Menu1)
	{
		DrawBox(5, 10, 449, 288, 0.0f, 0.f, 0.f, 0.f, 1.0f, true);
		DrawBox(5, 10, 450, 288, 2, 255.f, 0.f, 0.f, 0.5f, false);
		DrawBox(5, 10, 450, 23, 0, 255.f, 0.f, 0.f, 0.5f, true);
		DrawString(_xor_(" Teagy#9378                          External Skill V2                             (INSERT)").c_str(), 14, 10, 8, 1, 1, 1, 1);
		DrawBox(5, (20 * SelectedItem) + 32, 449, 25, 0, 0.34f, 0.34f, 0.34f, 0.5f, true);
		//DrawLine(10, 29, 150, 28, 1, 1, 1, 1);
		DrawBox(5, 32, 449, 25, 0, 0.f, 255.f, 0.f, 0.2f, true);
		DrawString(_xor_("              | Visuals |                                               | Assist |").c_str(), 15, 10, 35, 1, 1, 1, 1);

		if (BoxESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 60, 0, 1, 0, 255.f);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 60, 1, 0, 0, 255.f);

		DrawString(_xor_(" F1      Box").c_str(), 13, 10, 10 + 60, 1, 1, 1, 1);

		if (FilledBoxESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 80, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 80, 1, 0, 0, 1);

		DrawString(_xor_(" F2      Box Fill").c_str(), 13, 10, 10 + 80, 1, 1, 1, 1);

		if (CornerESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 100, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 100, 1, 0, 0, 1);

		DrawString(_xor_(" F3      Corner Box").c_str(), 13, 10, 10 + 100, 1, 1, 1, 1);


		if (SkeletonESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 120, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 120, 1, 0, 0, 1);

		DrawString(_xor_(" F4      Skeleton").c_str(), 13, 10, 10 + 120, 1, 1, 1, 1);


		if (LineESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 140, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 140, 1, 0, 0, 1);

		DrawString(_xor_(" F5      Snapline").c_str(), 13, 10, 10 + 140, 1, 1, 1, 1);

		if (HeadDotESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 160, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 160, 1, 0, 0, 1);

		DrawString(_xor_(" F6      Head Dot").c_str(), 13, 10, 10 + 160, 1, 1, 1, 1);

		if (CircleESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 180, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 180, 1, 0, 0, 1);

		DrawString(_xor_(" F7      Circle").c_str(), 13, 10, 10 + 180, 1, 1, 1, 1);

		if (DistanceESP)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 200, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 200, 1, 0, 0, 1);

		DrawString(_xor_(" F8      Distance").c_str(), 13, 10, 10 + 200, 1, 1, 1, 1);


		if (FOVDOT)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 220, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 220, 1, 0, 0, 1);

		DrawString(_xor_(" F9      Reticle").c_str(), 13, 10, 10 + 220, 1, 1, 1, 1);

		if (LockLine)
			DrawString(_xor_("        O").c_str(), 15, 10 + 120, 10 + 240, 0, 1, 0, 1);
		else
			DrawString(_xor_("        X").c_str(), 15, 10 + 120, 10 + 240, 1, 0, 0, 1);

		DrawString(_xor_(" F10    VisLock").c_str(), 13, 10, 10 + 240, 1, 1, 1, 1);






		if (Aimbot)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 60, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 60, 1, 0, 0, 1);

		DrawString(_xor_("F11          Aimbot").c_str(), 13, 220, 10 + 60, 1, 1, 1, 1);

		if (DrawFov)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 80, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 80, 1, 0, 0, 1);

		DrawString(_xor_("F12          Aim Fov").c_str(), 13, 220, 10 + 80, 1, 1, 1, 1);

		if (Head)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 100, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 100, 1, 0, 0, 1);

		DrawString(_xor_("HOME     Head").c_str(), 13, 220, 10 + 100, 1, 1, 1, 1);

		if (Chest)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 120, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 120, 1, 0, 0, 1);

		DrawString(_xor_("END        Chest").c_str(), 13, 220, 10 + 120, 1, 1, 1, 1);

		if (LButton)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 140, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 140, 1, 0, 0, 1);

		DrawString(_xor_("PAUSE     LButton").c_str(), 13, 220, 10 + 140, 1, 1, 1, 1);

		if (RButton)
			DrawString(_xor_("O").c_str(), 15, 10 + 420, 10 + 160, 0, 1, 0, 1);
		else
			DrawString(_xor_("X").c_str(), 15, 10 + 420, 10 + 160, 1, 0, 0, 1);

		DrawString(_xor_("DELETE    RButton").c_str(), 13, 220, 10 + 160, 1, 1, 1, 1);

		DrawString(_xor_("UP/DOWN        AimFov").c_str(), 13, 220, 10 + 200, 1, 1, 1, 1);

		DrawString(_xor_("LEFT/RIGHT      Smooth").c_str(), 13, 220, 10 + 220, 1, 1, 1, 1);

		DrawLine(10, 273, 450, 273, 1, 1, 1, 1);
		DrawString(_xor_(" update: 4/18/21").c_str(), 12, 10, 278, 1, 1, 1, 1);
	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		HWND test = FindWindowA(0, _xor_("Fortnite  ").c_str());

		if (test == NULL)
		{
			ExitProcess(0);
		}

		if (Logo)
		{
			if (GetAsyncKeyState(VK_INSERT) & 1) {
				Menu1 = !Menu1;
			}

			if (Menu1)
			{
			}

			if (GetAsyncKeyState(VK_F11) & 1) {
				Aimbot = !Aimbot;
			}
			
			if (GetAsyncKeyState(VK_F12) & 1) {
				DrawFov = !DrawFov;
			}
			if (aimfov) {
				if (GetAsyncKeyState(VK_UP) & 1)
					aimfov += 1;
				if (GetAsyncKeyState(VK_DOWN) & 1)
					aimfov -= 1;
				if (aimfov < 1)
					aimfov = 1;
				if (aimfov > 360)
					aimfov = 360;
			}

			if (Aim_Speed < Max_Aim_Speed) {
				if (GetAsyncKeyState(VK_RIGHT) & 1)
					Aim_Speed += 1;
				if (GetAsyncKeyState(VK_LEFT) & 1)
					Aim_Speed -= 1;
				if (Aim_Speed < 1)
					Aim_Speed = 1;
				if (Aim_Speed > 3)
					Aim_Speed = 3;
			}

			if (GetAsyncKeyState(VK_HOME) & 1) {
				Head = !Head;
			}

			if (GetAsyncKeyState(VK_END) & 1) {
				Chest = !Chest;
			}

			if (GetAsyncKeyState(VK_F1) & 1) {
				BoxESP = !BoxESP;
			}
			if (GetAsyncKeyState(VK_F2) & 1) {
				FilledBoxESP = !FilledBoxESP;
			}
			if (GetAsyncKeyState(VK_F6) & 1) {
				HeadDotESP = !HeadDotESP;
			}
			if (GetAsyncKeyState(VK_F5) & 1) {
				LineESP = !LineESP;
			}
			if (GetAsyncKeyState(VK_F4) & 1) {
				SkeletonESP = !SkeletonESP;
			}
			if (GetAsyncKeyState(VK_F8) & 1) {
				DistanceESP = !DistanceESP;
			}
			if (GetAsyncKeyState(VK_F9) & 1) {
				FOVDOT = !FOVDOT; // Reticle
			}
			if (GetAsyncKeyState(VK_F10) & 1) {
				LockLine = !LockLine;
			}
			if (GetAsyncKeyState(VK_F7) & 1) {
				CircleESP = !CircleESP;
			}

			if (GetAsyncKeyState(VK_F3) & 1) {
				CornerESP = !CornerESP;
			}

			if (GetAsyncKeyState(VK_PAUSE) & 1) {
				LButton = !LButton;
			}

			if (GetAsyncKeyState(VK_DELETE) & 1) {
				RButton = !RButton;
			}

			//if (GetAsyncKeyState(VK_END) & 1) {
			//	CapsLock = !CapsLock;
			//}

		}
	}
}

bool GetAimKey()
{

	//if (CapsLock)
	//	return (GetAsyncKeyState(VK_CAPITAL) & 0x8000);

	if (RButton)
		return (GetAsyncKeyState(VK_RBUTTON) & 0x8000);

	if (LButton)
		return (GetAsyncKeyState(VK_LBUTTON) & 0x8000);

}

void aimbot(float x, float y)
{
	float ScreenCenterX = (width / 2);
	float ScreenCenterY = (height / 2);
	int AimSpeed = Aim_Speed;
	float TargetX = 0;
	float TargetY = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);

	return;
}

double GetCrossDistance(double x1, double y1, double x2, double y2)
{
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

bool GetClosestPlayerToCrossHair(Vector3 Pos, float& max, float aimfov, DWORD_PTR entity)
{
	if (!GetAimKey() || !isaimbotting)
	{
		if (entity)
		{
			float Dist = GetCrossDistance(Pos.x, Pos.y, width / 2, height / 2);

			if (Dist < max)
			{
				max = Dist;
				entityx = entity;
				aimfov = aimfov;
			}
		}
	}
	return false;
}

void AimAt3(DWORD_PTR entity, Vector3 Localcam)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280);

	auto rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (rootHeadOut.x != 0 || rootHeadOut.y != 0)
	{
		if ((GetCrossDistance(rootHeadOut.x, rootHeadOut.y, width / 2, height / 2) <= aimfov * 8))
		{
			// Lockline
			if (LockLine)
				DrawLine(width / 2, height / 2, rootHeadOut.x, rootHeadOut.y, 1.f, 255.f, 0.f, 0.f, 1.0f);;;

			// Headdot
			if (LockLine)
				DrawEllipse(rootHeadOut.x, rootHeadOut.y, 3.0, 3.0, 1.3f, 255.f, 0.f, 0.f, 1.0f, false);

		}
	}
}

void AimAt2(DWORD_PTR entity, Vector3 Localcam)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280);

	auto rootHead = GetBoneWithRotation(currentactormesh, 2);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (rootHeadOut.x != 0 || rootHeadOut.y != 0)
	{
		if ((GetCrossDistance(rootHeadOut.x, rootHeadOut.y, width / 2, height / 2) <= aimfov * 8) || isaimbotting)
		{
			aimbot(rootHeadOut.x, rootHeadOut.y);
		}
	}
}

void AimAt(DWORD_PTR entity, Vector3 Localcam)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280);

	auto rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (rootHeadOut.x != 0 || rootHeadOut.y != 0)
	{
		if ((GetCrossDistance(rootHeadOut.x, rootHeadOut.y, width / 2, height / 2) <= aimfov * 8) || isaimbotting)
		{
			aimbot(rootHeadOut.x, rootHeadOut.y);;;

			DrawString(_xor_("TARGET LOCKED").c_str(), 13, rootHeadOut.x, rootHeadOut.y - 0, 255, 255, 1);
		}
	}
}

void aimbot2(Vector3 Localcam)
{
	if (entityx != 0)
	{
		if (GetAimKey())
		{
			AimAt2(entityx, Localcam);;;
		}
		else
		{
			isaimbotting = false;
		}
	}
}

void aimbot(Vector3 Localcam)
{
	if (entityx != 0)
	{
		if (GetAimKey())
		{
			AimAt(entityx, Localcam);;;
		}
		else
		{
			isaimbotting = false;
			AimAt3(entityx, Localcam);
		}
	}
}

void AIms(DWORD_PTR entity, Vector3 Localcam)
{
	float max = 100.f;

	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280);

	Vector3 rootHead = GetBoneWithRotation(currentactormesh, 66);;;
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (GetClosestPlayerToCrossHair(rootHeadOut, max, aimfov, entity))
		entityx = entity;
}

void DrawSkeleton(DWORD_PTR mesh)
{
	Vector3 vHeadBone = GetBoneWithRotation(mesh, 96);;;
	Vector3 vHip = GetBoneWithRotation(mesh, 2);
	Vector3 vNeck = GetBoneWithRotation(mesh, 65);
	Vector3 vUpperArmLeft = GetBoneWithRotation(mesh, 34);
	Vector3 vUpperArmRight = GetBoneWithRotation(mesh, 91);
	Vector3 vLeftHand = GetBoneWithRotation(mesh, 35);
	Vector3 vRightHand = GetBoneWithRotation(mesh, 63);
	Vector3 vLeftHand1 = GetBoneWithRotation(mesh, 33);;;
	Vector3 vRightHand1 = GetBoneWithRotation(mesh, 60);
	Vector3 vRightThigh = GetBoneWithRotation(mesh, 74);;;
	Vector3 vLeftThigh = GetBoneWithRotation(mesh, 67);
	Vector3 vRightCalf = GetBoneWithRotation(mesh, 75);
	Vector3 vLeftCalf = GetBoneWithRotation(mesh, 68);
	Vector3 vLeftFoot = GetBoneWithRotation(mesh, 69);
	Vector3 vRightFoot = GetBoneWithRotation(mesh, 76);;;

	Vector3 vHeadBoneOut = ProjectWorldToScreen(vHeadBone, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vHipOut = ProjectWorldToScreen(vHip, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vNeckOut = ProjectWorldToScreen(vNeck, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vUpperArmLeftOut = ProjectWorldToScreen(vUpperArmLeft, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vUpperArmRightOut = ProjectWorldToScreen(vUpperArmRight, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftHandOut = ProjectWorldToScreen(vLeftHand, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightHandOut = ProjectWorldToScreen(vRightHand, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftHandOut1 = ProjectWorldToScreen(vLeftHand1, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightHandOut1 = ProjectWorldToScreen(vRightHand1, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightThighOut = ProjectWorldToScreen(vRightThigh, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftThighOut = ProjectWorldToScreen(vLeftThigh, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightCalfOut = ProjectWorldToScreen(vRightCalf, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftCalfOut = ProjectWorldToScreen(vLeftCalf, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftFootOut = ProjectWorldToScreen(vLeftFoot, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightFootOut = ProjectWorldToScreen(vRightFoot, Vector3(Localcam.y, Localcam.x, Localcam.z));

	DrawLine(vHipOut.x, vHipOut.y, vNeckOut.x, vNeckOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vUpperArmLeftOut.x, vUpperArmLeftOut.y, vNeckOut.x, vNeckOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vUpperArmRightOut.x, vUpperArmRightOut.y, vNeckOut.x, vNeckOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vUpperArmLeftOut.x, vUpperArmLeftOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vUpperArmRightOut.x, vUpperArmRightOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vLeftHandOut1.x, vLeftHandOut1.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vRightHandOut1.x, vRightHandOut1.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftThighOut.x, vLeftThighOut.y, vHipOut.x, vHipOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vRightThighOut.x, vRightThighOut.y, vHipOut.x, vHipOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftCalfOut.x, vLeftCalfOut.y, vLeftThighOut.x, vLeftThighOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vRightCalfOut.x, vRightCalfOut.y, vRightThighOut.x, vRightThighOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftFootOut.x, vLeftFootOut.y, vLeftCalfOut.x, vLeftCalfOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
	DrawLine(vRightFootOut.x, vRightFootOut.y, vRightCalfOut.x, vRightCalfOut.y, 1.f, 0.f, 0.f, 0.f, 200.f);
}

void Loop(int width, int height) {

	if (FOVDOT) // Reticle
		DrawCircle(width / 2, height / 2, 3.f, 1.f, 0.f, 10.f, 255.f, 1.0f, true);

	// DrawLine(width / 2, 526, width / 2, 557, 2.4f, 0, 0, 0, 10);

	float radiusx = aimfov * (width / 2 / 100);
	float radiusy = aimfov * (height / 2 / 100);

	float calcradius = (radiusx + radiusy) / 2;

	if (DrawFov)
	DrawCircle(width / 2, height / 2, calcradius, 1.f, 0.f, 0.f, 0.f, 50.f, false);

	Uworld = read<DWORD_PTR>(DriverHandle, processID, base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x180);

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(DriverHandle, processID, Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(DriverHandle, processID, LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(DriverHandle, processID, Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(DriverHandle, processID, PlayerController + 0x2A0);

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(DriverHandle, processID, LocalPawn + 0x130);  

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(DriverHandle, processID, LocalPawn + 0x18);
	}

	Ulevel = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD64 PlayerState = read<DWORD64>(DriverHandle, processID, LocalPawn + 0x240);

	if (PlayerState == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(DriverHandle, processID, Ulevel + 0xA0);

	DWORD_PTR AActors = read<DWORD_PTR>(DriverHandle, processID, Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = read<uint64_t>(DriverHandle, processID, AActors + i * 0x8);

		int curactorid = read<int>(DriverHandle, processID, CurrentActor + 0x18);

		if (curactorid == localplayerID || curactorid == 20328438 || curactorid == 20328753 || curactorid == 9343426) 

		{
			if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
				continue;

			uint64_t CurrentActorRootComponent = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x130);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x280);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			int MyTeamId = read<int>(DriverHandle, processID, PlayerState + 0xED0);

			DWORD64 otherPlayerState = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x240);

			if (otherPlayerState == (uint64_t)nullptr || otherPlayerState == -1 || otherPlayerState == NULL)
				continue;

			int ActorTeamId = read<int>(DriverHandle, processID, otherPlayerState + 0xED0);

			Vector3 CirclePOS = GetBoneWithRotation(currentactormesh, 2);
			Vector3 Headpos = GetBoneWithRotation(currentactormesh, 66);
			Vector3 distancepelvis = GetBoneWithRotation(currentactormesh, 0);

			Localcam = CameraEXT;
			localactorpos = read<Vector3>(DriverHandle, processID, Rootcomp + 0x11C);

			float distance = localactorpos.Distance(distancepelvis) / 100.f;

			if (distance < 1.5f)
				continue;

			auto rootHeadDOT = GetBoneWithRotation(currentactormesh, 66);
			Vector3 rootHeadOutDOT2 = ProjectWorldToScreen(rootHeadDOT, Vector3(Localcam.y, Localcam.x, Localcam.z));

			//W2S
			Vector3 BottomposW2s = ProjectWorldToScreen(distancepelvis, Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 HeadposW2s = ProjectWorldToScreen(Headpos, Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);;;
			Vector3 bottom = ProjectWorldToScreen(bone0, Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15), Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 Aimpos = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 10), Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 HeadDot = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 8), Vector3(Localcam.y, Localcam.x, Localcam.z));;;
			Vector3 CircleESPPosition = ProjectWorldToScreen(Vector3(CirclePOS.x, CirclePOS.y, CirclePOS.z + 15), Vector3(Localcam.y, Localcam.x, Localcam.z));;;

			float Height1 = abs(Headbox.y - bottom.y);
			float Width1 = Height1 * 1.0;

			if (MyTeamId != ActorTeamId) 
			{
				if (SkeletonESP)
					DrawSkeleton(currentactormesh);

				if (BoxESP)
					DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 1.0f, 0.f, 0.f, 255.f, 100.f, false);

				// Box Outline
				//if (BoxESP)
				//	DrawBox(Headbox.x - (Width1 / 3), Headbox.y, Width1, Height1, 1.5f, 0.f, 0.f, 0.f, 100.f, false);
				//if (BoxESP)
				//   DrawBox(Headbox.x - (Width1 / 3), Headbox.y, Width1, Height1, 0.5f, 255.f, 255.f, 255.f, 50.f, false);

				if (FilledBoxESP)
					DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 0.f, 255.f, 0.f, 0.f, 0.2f, true);

				if (HeadDotESP)
					DrawEllipse(rootHeadOutDOT2.x, rootHeadOutDOT2.y, 3.0, 3.0, 1.f, 0.f, 255.f, 255.f, 1.0f, false);

				if (CornerESP) {
					DrawLine(Headbox.x - (Width1 / 2), Headbox.y, Headbox.x - (Width1 / 2) + (Width1 / 4), Headbox.y, 1.f, 0.f, 0.f, 255.f, 100);//x top corner left
					DrawLine(Headbox.x - (Width1 / 2), Headbox.y, Headbox.x - (Width1 / 2), Headbox.y + (Width1 / 4), 1.f, 0.f, 0.f, 255.f, 100);//y
					DrawLine(Headbox.x - (Width1 / 2) + Width1, Headbox.y, Headbox.x + (Width1 / 2) - (Width1 / 4), Headbox.y, 1.f, 0.f, 0.f, 255.f, 100);//x  top corner right
					DrawLine(Headbox.x - (Width1 / 2) + Width1, Headbox.y, Headbox.x - (Width1 / 2) + Width1, Headbox.y + (Width1 / 4), 1.f, 0.f, 0.f, 255.f, 100);//y
					DrawLine(Headbox.x - (Width1 / 2), Headbox.y + Height1, Headbox.x - (Width1 / 2) + (Width1 / 4), Headbox.y + Height1, 1.f, 0.f, 0.f, 255.f, 100);//x bottom left
					DrawLine(Headbox.x - (Width1 / 2), Headbox.y - (Width1 / 4) + Height1, Headbox.x - (Width1 / 2), Headbox.y + Height1, 1.f, 0.f, 0.f, 255.f, 100);//y bottom left
					DrawLine(Headbox.x - (Width1 / 2) + Width1, Headbox.y + Height1, Headbox.x + (Width1 / 2) - (Width1 / 4), Headbox.y + Height1, 1.f, 0.f, 0.f, 255.f, 100);//x bottom right
					DrawLine(Headbox.x - (Width1 / 2) + Width1, Headbox.y - (Width1 / 4) + Height1, Headbox.x - (Width1 / 2) + Width1, Headbox.y + Height1, 1.f, 0.f, 0.f, 255.f, 100);//y bottom right
				}

				//if (Box2)
				//{
				//	float BoxHeight = HeadposW2s.y - bottom.y;
				//	float BoxWidth = BoxHeight / 1.0f;
				//
				//	Vector3 topRight = { HeadposW2s.x - (BoxWidth / 2), HeadposW2s.y, 0 };
				//	Vector3 topLeft = { HeadposW2s.x + (BoxWidth / 2), HeadposW2s.y, 0 };
				//	Vector3 bottomRight = { bottom.x - (BoxWidth / 2), bottom.y, 0 };
				//	Vector3 bottomLeft = { bottom.x + (BoxWidth / 2), bottom.y, 0 };
				//
				//	DrawLine(topLeft.x, topLeft.y, topRight.x, topRight.y, 1, 0, 0, 255, 1.f);
				//	DrawLine(topLeft.x, topLeft.y, bottomLeft.x, bottomLeft.y, 1, 0, 0, 255, 1.f);
				//	DrawLine(topRight.x, topRight.y, bottomRight.x, bottomRight.y, 1, 0, 0, 255, 1.f);
				//	DrawLine(bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y, 1, 0, 0, 255, 1.f);
				//}

				if (CircleESP)
					DrawEllipse(CircleESPPosition.x, CircleESPPosition.y, 30.5, 30.5, 1.f, 0.f, 0.f, 255.f, 100.0f, false);

				//if (CircleESPFilled)
				//	DrawEllipse(CircleESPPosition.x, CircleESPPosition.y, 25.5, 25.5, 2.f, 0.f, 255.f, 255.f, 100.0f, false);

				if (DistanceESP)
				{
					CHAR dist[50];
					sprintf_s(dist, _xor_("%.fm Away").c_str(), distance);

					DrawString(dist, 12, bottom.x + 30, bottom.y - 0, 255, 255, 1);
				}

				if (LineESP)
					DrawLine(width / 2, height, bottom.x, bottom.y, 0.7f, 255.f, 255.f, 255.f, 1);

				if (Aimbot)
				{
					AIms(CurrentActor, Localcam);
				}
			}
		}
	}

	if (Chest)
	{
		aimbot2(Localcam);
	}

	if (Aimbot)
	{
		aimbot(Localcam);
	}
}

void main()
{

	if (DriverHandle == INVALID_HANDLE_VALUE)
	{
		printf(_xor_(" \n").c_str());
		printf(_xor_(" Driver wasnt loaded\n").c_str());
		printf(_xor_(" Reason 1: You didnt load it\n").c_str());
		printf(_xor_(" Reason 2: It didnt work when you 'loaded it' \n").c_str());
		printf(_xor_(" \n").c_str());
		printf(_xor_(" Solution: Try again and try different fixes. (Didnt work then? make a ticket)\n").c_str());
		Sleep(2000);
		exit(0);
	}

	info_t Input_Output_Data1;
	unsigned long int Readed_Bytes_Amount1;
	DeviceIoControl(DriverHandle, ctl_clear, &Input_Output_Data1, sizeof Input_Output_Data1, &Input_Output_Data1, sizeof Input_Output_Data1, &Readed_Bytes_Amount1, nullptr);

	while (hwnd == NULL)
	{


		printf(_xor_(" \n").c_str());
		printf(_xor_(" Looking for discord & fortnite...\n").c_str());
		Sleep(1000);
	}
	GetWindowThreadProcessId(hwnd, &processID);

	RECT rect;
	if (GetWindowRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	info_t Input_Output_Data;
	Input_Output_Data.pid = processID;
	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(DriverHandle, ctl_base, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
	base_address = (unsigned long long int)Input_Output_Data.data;
	system("CLS");
	printf(_xor_(" \n").c_str());
	std::printf(_xor_(" Injected\n").c_str(), (void*)base_address);

	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);

	DirectOverlaySetOption(D2DOV_FONT_COURIER);
	getchar();
}