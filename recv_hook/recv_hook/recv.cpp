#define _CRT_SECURE_NO_DEPRECATE
#pragma warning(disable:4996)
#include <string>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
#include <stdio.h>
#include <Python.h>
#include <Shlwapi.h>
#include <stdlib.h>
#include <iostream> 
#include "recv.h"

using namespace std;


#define ReciveMessage 0x315E98			//������Ϣ
#define ReciveMessageParam 0x126D7F8		//������Ϣ�Ĳ���ƫ��
wchar_t tempwxid[50] = { 0 };	//���΢��ID

DWORD r_esp = 0;
DWORD r_eax = 0;

CHAR originalCode[5] = { 0 };
//������ƫ��
DWORD dwParam = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessageParam;

//������ҪHOOK�ĵ�ַ
DWORD dwHookAddr = (DWORD)GetModuleHandle(L"WeChatWin.dll") + ReciveMessage - 5;

//���ص�ַ
DWORD RetAddr = dwHookAddr + 5;

//��Ϣ�ṹ��
struct Message
{
	wchar_t type[10];		//��Ϣ����
	wchar_t source[20];		//��Ϣ��Դ
	wchar_t wxid[40];		//΢��ID/ȺID
	wchar_t msgSender[40];	//��Ϣ������
	wchar_t content[200];	//��Ϣ����
};

PyObject* StringToPy(std::string p_obj)
{
	int wlen = ::MultiByteToWideChar(CP_ACP, NULL, p_obj.c_str(), int(p_obj.size()), NULL, 0);
	wchar_t* wszString = new wchar_t[wlen + 1];
	::MultiByteToWideChar(CP_ACP, NULL, p_obj.c_str(), int(p_obj.size()), wszString, wlen);
	wszString[wlen] = '\0';
	PyObject* pb = PyUnicode_FromUnicode((const Py_UNICODE*)wszString, wlen);
	delete wszString;
	return pb;
}

std::string WstringToString(const std::wstring str)
{// wstringתstring
	unsigned len = str.size() * 4;
	setlocale(LC_CTYPE, "");
	char* p = new char[len];
	wcstombs(p, str.c_str(), len);
	std::string str1(p);
	delete[] p;
	return str1;
}

void HookChatRecord()
{
	//��װ����
	BYTE bJmpCode[5] = { 0xE9 };
	*(DWORD*)& bJmpCode[1] = (DWORD)RecieveWxMesage - dwHookAddr - 5;

	//���浱ǰλ�õ�ָ��,��unhook��ʱ��ʹ�á�
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, originalCode, 5, 0);

	//����ָ�� B9 E8CF895C //mov ecx,0x5C89CFE8
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwHookAddr, bJmpCode, 5, 0);
}

__declspec(naked) void RecieveWxMesage()
{
	//�����ֳ�
	__asm
	{
		//���䱻���ǵĴ���
		//5B950573  |.  B9 E8CF895C           mov ecx,WeChatWi.5C89CFE8
		//mov ecx,0x5C89CFE8
		mov ecx, dwParam

		//��ȡesp�Ĵ������ݣ�����һ��������
		mov r_esp, esp
		mov r_eax, eax

		pushad
		pushfd
	}
	SendWxMessage();

	//�ָ��ֳ�
	__asm
	{
		popfd
		popad
		//���ر�HOOKָ�����һ��ָ��
		jmp RetAddr
	}
}

void SendWxMessage()
{
	Message* msg = new Message;
	//��Ϣ���λ��
	DWORD** msgAddress = (DWORD * *)r_esp;
	//��Ϣ����
	DWORD msgType = *((DWORD*)(**msgAddress + 0x30));
	wstring fullmessgaedata = GetMsgByAddress(**msgAddress + 0x68);

	LPVOID pWxid = *((LPVOID*)(**msgAddress + 0x40));
	swprintf_s(msg->wxid, L"%s", (wchar_t*)pWxid);

	//MessageBox(
	//	NULL,
	//	fullmessgaedata.c_str(),
	//	NULL,
	//	MB_OK
	//);

	if (StrStrW(msg->wxid, L"gh")) {
		//��ʼ��Python���� 
		Py_Initialize();
		PyRun_SimpleString("import sys");
		PyRun_SimpleString("import pymysql");
		//���Insertģ��·�� 
		//PyRun_SimpleString(chdir_cmd.c_str());
		PyRun_SimpleString("sys.path.append('./')");
		//����ģ�� 
		PyObject* pModule = PyImport_ImportModule("demo");
		PyObject* pFunc = NULL;
		pFunc = PyObject_GetAttrString(pModule, "save");
		std::string str = "";
		str = WstringToString(fullmessgaedata);
		const char* p = str.c_str();
		PyObject* pArgs = PyTuple_New(1);
		PyTuple_SetItem(pArgs, 0, StringToPy(str));
		PyObject* pRet = PyObject_CallObject(pFunc, pArgs);
		Py_Finalize();
	}


}


LPCWSTR GetMsgByAddress(DWORD memAddress)
{
	//��ȡ�ַ�������
	DWORD msgLength = *(DWORD*)(memAddress + 4);
	if (msgLength == 0)
	{
		WCHAR* msg = new WCHAR[1];
		msg[0] = 0;
		return msg;
	}

	WCHAR* msg = new WCHAR[msgLength + 1];
	ZeroMemory(msg, msgLength + 1);

	//��������
	wmemcpy_s(msg, msgLength + 1, (WCHAR*)(*(DWORD*)memAddress), msgLength + 1);
	return msg;
}


