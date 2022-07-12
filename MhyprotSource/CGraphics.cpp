#include "CGraphics.h"
#include "Offsets.h"
#include "../MhyprotSource/Mhyprot/baseadress.h"
#include <d3dx9.h>

//-----------------------------------W2S for Arma 3 & DayZ--------------------------------------------------

Vector3 get_inverted_view_up()
{
	return Vector3(read<Vector3>(camera + 0x14));
}

Vector3 get_inverted_view_right()
{
	return Vector3(read<Vector3>(camera + 0x8));
}

Vector3 get_inverted_view_translation()
{
	return Vector3(read<Vector3>(camera + 0x2C));
}

Vector3 get_inverted_view_forward()
{
	return Vector3(read<Vector3>(camera + 0x20));
}

Vector3 get_viewport_size()
{
	return Vector3(read<Vector3>(camera + 0x58));
}

Vector3 get_projection_d1()
{
	return Vector3(read<Vector3>(camera + 0xD0));
}
Vector3 get_projection_d2()
{
	return Vector3(read<Vector3>(camera + 0xDC));
}

Vector3 invertedviewup;
Vector3 invertedviewright;
Vector3 invertedviewtranslation;
Vector3 invertedviewforward;
Vector3 viewportsize;
Vector3 projectiondl;
Vector3 projectiond1;
Vector3 projectiond2;

Vector3 c_graphics_instance::world_to_screen(const Vector3 position)
{

	const Vector3 temp = position - invertedviewtranslation;

	const float x = temp.Dot(invertedviewright);
	const float y = temp.Dot(invertedviewup);

	if (const float z = temp.Dot(invertedviewforward); z > 0)
	{
		return { viewportsize.x * (1 + (x / projectiond1.x / z)), get_viewport_size().y * (1 - (y / projectiond2.y / z)), z
		};
	}

	return {};


	return { 0, 0, 0 };
}

bool c_graphics_instance::initializeit()
{
	invertedviewtranslation = get_inverted_view_translation();
	invertedviewright = get_inverted_view_right();
	invertedviewup = get_inverted_view_up();
	invertedviewforward = get_inverted_view_forward();
	viewportsize = get_viewport_size();
	projectiond1 = get_projection_d1();
	projectiond2 = get_projection_d2();
	return true;
}


//--------------------------------- W2S for many other Games --------------------------------

//constexpr auto m_pGraphics = 0x25B8;
//constexpr auto cGameOffset = 0x1440B5458;
//constexpr auto m_Width = 0x20;
//constexpr auto m_Height = 0x24;
//constexpr auto m_camera = 0x68;
//constexpr auto m_cameraMatrix = 0x30;
//constexpr auto m_viewMatrix = 0x1B0;
//
//D3DXMATRIX CGraphicsInstance::GetGameMatrix()
//{
//	D3DXMATRIX gameMatrix;
//	D3DXMATRIX cameraMatrix = CGraphicsInstance::GetCameraMatrix();
//
//	gameMatrix.m[0][0] = cameraMatrix.m[0][0];
//	gameMatrix.m[0][1] = cameraMatrix.m[1][0];
//	gameMatrix.m[0][2] = cameraMatrix.m[2][0];
//	gameMatrix.m[0][3] = cameraMatrix.m[3][0];
//
//	gameMatrix.m[1][0] = -cameraMatrix.m[0][1];
//	gameMatrix.m[1][1] = -cameraMatrix.m[1][1];
//	gameMatrix.m[1][2] = -cameraMatrix.m[2][1];
//	gameMatrix.m[1][3] = -cameraMatrix.m[3][1];
//
//	gameMatrix.m[2][0] = cameraMatrix.m[0][2];
//	gameMatrix.m[2][1] = cameraMatrix.m[1][2];
//	gameMatrix.m[2][2] = cameraMatrix.m[2][2];
//	gameMatrix.m[2][3] = cameraMatrix.m[3][2];
//
//	gameMatrix.m[3][0] = cameraMatrix.m[0][3];
//	gameMatrix.m[3][1] = cameraMatrix.m[1][3];
//	gameMatrix.m[3][2] = cameraMatrix.m[2][3];
//	gameMatrix.m[3][3] = cameraMatrix.m[3][3];
//
//	return gameMatrix;
//}
//
//D3DXMATRIX CGraphicsInstance::GetCameraMatrix()
//{
//	uint64_t game = read<uint64_t>(cGameOffset);
//	uint64_t graphics = read<uint64_t>(game + m_pGraphics);
//	uint64_t camera = read<uint64_t>(graphics + m_camera);
//	uint64_t cameraMatrix = read<uint64_t>(camera + m_cameraMatrix);
//	D3DMATRIX viewMatrix = read<D3DMATRIX>(cameraMatrix + m_viewMatrix);
//
//	return viewMatrix;
//}
//
//Vector3 c_graphics_instance::world_to_screen(Vector3* ScreenPos, Vector3 WorldPosition)
//{
//
//	D3DXMATRIX gameMatrix = GetGameMatrix();
//	float w = 0.0f;
//
//	ScreenPos->x = gameMatrix.m[0][0] * WorldPosition.x + gameMatrix.m[0][1] * WorldPosition.y + gameMatrix.m[0][2] * WorldPosition.z + gameMatrix.m[0][3];
//	ScreenPos->y = gameMatrix.m[1][0] * WorldPosition.x + gameMatrix.m[1][1] * WorldPosition.y + gameMatrix.m[1][2] * WorldPosition.z + gameMatrix.m[1][3];
//	w = (float)(gameMatrix.m[3][0] * WorldPosition.x + gameMatrix.m[3][1] * WorldPosition.y + gameMatrix.m[3][2] * WorldPosition.z + gameMatrix.m[3][3]);
//
//	if (w < 0.01f)
//		return false;
//
//	float invw = 1.0f / w;
//
//	ScreenPos->x *= invw;
//	ScreenPos->y *= invw;
//
//	uint64_t game = read<uint64_t>(cGameOffset);
//	uint64_t graphics = read<uint64_t>(game + m_pGraphics);
//	float width = read<uint32_t>(graphics + m_Width);
//	float height = read<uint32_t>(graphics + m_Height);
//
//	float x = width / 2.f;
//	float y = height / 2.f;
//
//	x += 0.5f * (float)ScreenPos->x * width + 0.5f;
//	y -= 0.5f * (float)ScreenPos->y * height + 0.5f;
//
//	ScreenPos->x = x;
//	ScreenPos->y = y;
//	ScreenPos->z = 0;
//	return true;
//}