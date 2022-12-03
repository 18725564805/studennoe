// RemotaCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemotaCtrl.h"
#include <direct.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

std::string MakeDriveInfo() { 
    //函数参数：1--》A盘  2--》B盘 （A，B都属于软盘） 3--》C盘 类推
    std::string result;
    for (int i = 0; i < 26; i++) {
        if (_chdrive(i + 1) == 0) {
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i;
        }
    }

    Cpacket pack(1, (BYTE*)result.c_str(), result.size());
    CSockserver::getinstance()->Send(pack);
    //_chdrive(); //改变当前的驱动。
    return 0;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            //CSockserver* pserver = CSockserver::getinstance();
            //// TODO: 在此处为应用程序的行为编写代码。
            //if (pserver) {
            //    if (false == pserver->Initsockserv()) {
            //        MessageBox(NULL, _T("网络错误"), _T("请检测网络链接"), MB_OK | MB_ICONERROR);
            //        exit(0);
            //    }
            //    int count = 0;
            //    while (NULL != CSockserver::getinstance()) {
            //        if (count == 3) {
            //            MessageBox(NULL, _T("链接失败"), _T("请重试"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        if (pserver->InitAccept() == false) {
            //            MessageBox(NULL, _T("链接错误"), _T("正在重连"), MB_OK | MB_ICONERROR);
            //            count++;
            //        }
            //        pserver->Dealcommnat();
            //    }
            //}
            int nCmd = 1;
            switch (1) {
            case 1: //查看磁盘分区。
                MakeDriveInfo();
                break;
            }
            
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
