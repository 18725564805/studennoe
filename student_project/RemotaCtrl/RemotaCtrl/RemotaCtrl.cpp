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

#define FILE_NAME_SIZE 256

typedef struct file_info {
    file_info() {
        IsInvalid = FALSE;
        IsDirectory = -1;
        Hasnext = TRUE;
        memset(szFileName, 0x00, sizeof(szFileName));
    }

    BOOL IsInvalid; //是否无效
    BOOL IsDirectory;//是否为目录 1代表目录 0代表普通文件。
    BOOL Hasnext; //是否还有后续文件，0没有 1 有
    char szFileName[FILE_NAME_SIZE];//文件名
}FILEINFO, *PFILEINFO;

std::string MakeDriveInfo() {  //拿到目录
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

int MakeDirectoryInfo() { //获取指定文件夹下的信息。
    std::string path;
    //std::list<FILEINFO> listFileinfos;
    if (CSockserver::getinstance()->GetFilepath(path) == false) {
        OutputDebugString(_T("错误命令 ! ! !"));
        return -1;
    }

    if (_chdir(path.c_str()) != 0) {
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;
        finfo.IsDirectory = TRUE;
        finfo.Hasnext = FALSE;
        memcpy(finfo.szFileName, path.c_str(), path.size());
        //listFileinfos.push_back(finfo);
        Cpacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CSockserver::getinstance()->Send(pack);
        OutputDebugString(_T("错误路径 ! ! !"));
        return -2;
    }

    _finddata_t fdata;
    int hfind;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("无文件 ! ! !"));
        return -3;
    }

    do {
        FILEINFO finfo;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //fdata.attrib & _A_SUBDIR != 0  ||| fdata.attrib中有_A_SUBDIR属性，说明他是目录。
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR != 0);
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
        //listFileinfos.push_back(finfo);
        Cpacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CSockserver::getinstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
    //发生信息到控制端
    FILEINFO finfo;
    finfo.Hasnext = FALSE;
    Cpacket pack(2, (BYTE*)&finfo, sizeof(finfo));
    CSockserver::getinstance()->Send(pack);
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
            case 2:  //查看指定目录下的文件。

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
