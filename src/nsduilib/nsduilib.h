#ifndef __NSTBCIASKINENGINE_H__
#define __NSTBCIASKINENGINE_H__
#pragma  once

#include <UIlib.h>
#include "stdafx.h"
#include "pluginapi.h"
#include <windows.h>
#include "MsgDef.h"

#	if defined(NSDUILIB_EXPORTS)
#			define NSDUILIB_API  extern "C"  __declspec(dllexport)
#	else
#			define NSDUILIB_API  extern "C" __declspec(dllimport)
#	endif


/* ������ 1. skin��·�������setup.exe���ɵ�·����
  *           2. skin�����ļ���
  *           3. ��װҳ��tab������
  * ���ܣ� ��ʼ������
*/
NSDUILIB_API void InitTBCIASkinEngine(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. control������
  * ���ܣ� Ѱ���ض���control�Ƿ����
*/
NSDUILIB_API void FindControl(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. richedit control������
  *           2. ���Э���ļ�����
  * ���ܣ� ��ʾ���֤�ļ�
*/
NSDUILIB_API void ShowLicense(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. ��click�¼���control������
  *           2. ���Э���ļ�����
  * ���ܣ� Ϊ�ؼ��󶨶�Ӧ���¼�����click��Ϣʱִ�ж�Ӧ����
*/
NSDUILIB_API void  OnControlBindNSISScript(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. control������
  *           2. ����control������
  *  		   3. ���ݵ����� (�����ṩ�����������ͣ� 1. text; 2. bkimage; 3. link; 4. enable )
  * ���ܣ� Ϊ�ؼ��󶨶�Ӧ���¼�����click��Ϣʱִ�ж�Ӧ����
*/
NSDUILIB_API void  SetControlData(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. control������
  *           2. ���ݵ����� (�����ṩһ���������ͣ� 1. text; )
  * ���ܣ� Ϊ�ؼ��󶨶�Ӧ���¼�����click��Ϣʱִ�ж�Ӧ����
*/
NSDUILIB_API void  GetControlData(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. TimerID(һ���ǻص�������ID)
  *           2. interval
  * ���ܣ� ������ʱ��
*/
NSDUILIB_API void  TBCIACreatTimer(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. TimerID(һ���ǻص�������ID)
  * ���ܣ� ɱ����ʱ��
*/
NSDUILIB_API void  TBCIAKillTimer(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. ��ϢHWND
  *            2. ��ϢID
  *			   3. WPARAM
  *			   4. LPARAM
  * ���ܣ� ����Ϣ
*/
NSDUILIB_API void  TBCIASendMessage(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. ���⣨���磺 ��ѡ���ļ��У�
  * ���ܣ� ����Ϣ
*/
NSDUILIB_API void  SelectFolderDialog(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. ��Ӧ��ʼ��װ���ȵĽ���������
  * ���ܣ� ��ʼ��װ��Ӧ
*/
NSDUILIB_API void  StartInstall(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. ��Ӧ��ʼж�ؽ��ȵĽ���������
  * ���ܣ� ��ʼ��װ��Ӧ
*/
NSDUILIB_API void  StartUninstall(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ ��
  * ���ܣ� ��ʾ���棨ע�⣺һ��������Show������
*/
NSDUILIB_API void  ShowPage(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ ��
  * ���ܣ� �˳���װ
*/
NSDUILIB_API void  ExitTBCIASkinEngine(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* ������ 1. �����ļ�������
               2. ����ؼ�����
			   3. ��ʾ���ݿؼ�����
			   4. �رհ�ť�ؼ�����
			   5. ȷ����ť�ؼ�����
			   6. ȡ����ť�ؼ�����
  * ���ܣ� ��ʼ��MessageBox
*/
NSDUILIB_API void  InitTBCIAMessageBox(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/* Verify the Character in buffer
  * it must be in [a-zA-Z0-9_]  characters
*/
NSDUILIB_API void  VerifyCharaters(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);
/******************************************
*   to make the icon to handle
******************************************/
NSDUILIB_API void  SetIconImage(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);
/******************************************
verify number
******************************************/
NSDUILIB_API void  VerifyNumbers(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);

/******************************************
to free skin engine and free resource
******************************************/
NSDUILIB_API void FreeSkinEngine(HWND hwndParent, int string_size, char *variables, stack_t **stacktop, extra_parameters *extra);


#endif



