// RemotaCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemotaCtrl.h"
#include <direct.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <corecrt_io.h>


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

int RunFile() {

    std::string strpath;
    if (CSockserver::getinstance()->GetFilepath(strpath) == false) {
        return -1; //获取失败
    }
    //打开指定路径的文件，若文件为  .exe（程序文件）也会执行该程序，类型与Linux下的execl函数族
    ShellExecuteA(NULL, NULL, strpath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    Cpacket pack(3, NULL, 0);
    CSockserver::getinstance()->Send(pack);

    return 0;  //获取成功
}

#define BUF_SIZE 1024
int DownloadFile() {
    std::string strPath;
    long long data = 0;
    CSockserver::getinstance()->GetFilepath(strPath);
    FILE* pfile = NULL;
    errno_t err = fopen_s(&pfile ,strPath.c_str(), "rb");
    if (err != 0) {
        Cpacket pack(4, (BYTE*)&data, 8);
        CSockserver::getinstance()->Send(pack);
        return -1;
    }

    if (pfile != NULL) {
        //先发送文件的大小过去
        fseek(pfile, 0, SEEK_END);
        data = _ftelli64(pfile);
        Cpacket pack(4, (BYTE*)&data, 8);
        CSockserver::getinstance()->Send(pack);
        fseek(pfile, 0, SEEK_SET);

        //正式发送文件内容
        char* buf = new char[BUF_SIZE];
        size_t relen = 0;
        do {
            relen = fread(buf, 1, BUF_SIZE, pfile);
            Cpacket pack(4, (BYTE*)buf, relen);
            CSockserver::getinstance()->Send(pack);

        } while (relen >= BUF_SIZE);

        delete[]buf;
    }
    fclose(pfile);

    Cpacket pack1(4, NULL, 0);//最后发送一个作为结尾标志。
    CSockserver::getinstance()->Send(pack1);
    return 0;
}

int  MouseEvent() {
    MOUSEEV mouse;
    WORD nFlags = 0;
    if (CSockserver::getinstance()->GetMouseEvent(mouse)) {
        switch (mouse->nButton) { //哪一个鼠标按钮被按下
        case 0://左键
            nFlags = 0x0001;//第一个bit位
            break;
        case 1://右键
            nFlags = 0x0002;//第二个bit位
            break;
        case 2://中键
            nFlags = 0x0004;//第三个bit位
            break;
        case 4://中键
            nFlags = 0x0008;//鼠标移动，没有按下按钮
            break;
        }
        if(nFlags != 8)  SetCursorPos(mouse->Ptxy.x, mouse->Ptxy.y);//有按键按下，设置坐标。
        switch (mouse->nAction) { //点击了几下
        case 0://单击
            nFlags = nFlags | 0x0010;
            break;
        case 1://双击
            nFlags = nFlags | 0x0020;
            break;
        case 2://按下
            nFlags = nFlags | 0x0040;
            break;
        case 3://放开
            nFlags = nFlags | 0x0080;
            break;
        default:
            break;
        }
        switch (nFlags) {
        case 0x0021://左键双击 ,双击发送了，先在这里执行一次，又到单机那里执行一次。
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x0011://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0041://左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0081://左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0022://右键双击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());         
        case 0x0012://右键单击
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x0042://右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0082://右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0024://中键双击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
        case 0x0014://中键单击
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0044://中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0084://中建放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x0008://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse->Ptxy.x, mouse->Ptxy.y, 0, GetMessageExtraInfo());
            break;
        }
    Cpacket pack1(5, NULL, 0);//应答数据，代表收到了数据。
    CSockserver::getinstance()->Send(pack1);
    }
    else {
        OutputDebugString(_T("获取鼠标信息失败！！"));
        return -1;
    }

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
                MakeDirectoryInfo();
                break;
            case 3: //打开文件
                RunFile();
                break;
            case 4:
                DownloadFile();
                break;
            case 5://鼠标操作
                MouseEvent();
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
