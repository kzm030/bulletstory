#include "DxLib.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetGraphMode(800, 600, 32);	// ��ʃ��[�h�ݒ�
	ChangeWindowMode(TRUE);	// �E�B���h�E���[�h�ɕύX

	if(DxLib_Init() == -1)	// �c�w���C�u��������������
	{
		return( -1 );	// �G���[���N�����璼���ɏI��
	}

	DrawPixel(320, 240, 0xffff);	// �_��ł�
	WaitKey();	// �L�[���͑҂�


	DxLib_End();	// �c�w���C�u�����g�p�̏I������

	return( 0 );	// �\�t�g�̏I�� 
}
