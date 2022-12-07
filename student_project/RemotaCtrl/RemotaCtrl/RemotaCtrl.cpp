﻿// RemotaCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemotaCtrl.h"
#include <direct.h>
#include <atlimage.h>
#include "LockDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <corecrt_io.h>


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

#define FILE_NAME_SIZE 256  //文件名

CLockDialog dig;

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

int my_MakeDriveInfo() {  //拿到目录
    //函数参数：1--》A盘  2--》B盘 （A，B都属于软盘） 3--》C盘 类推
    std::string result;
    for (int i = 1; i < 26; i++) {
        if (_chdrive(i) == 0) { //_chdrive(); //改变当前的驱动。
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1;
        }
    }

    Cpacket pack(1, (BYTE*)result.c_str(), result.size());
    CSockserver::getinstance()->Send(pack);

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
        finfo.IsDirectory = ((fdata.attrib & _A_SUBDIR) != 0);
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

int RunFile() { //执行或者打开文件

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
int DownloadFile() { //下载文件
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

int  MouseEvent() {//鼠标移动，点击。消息的获取
    MOUSEEV mouse;
    WORD nFlags = 0;
    if (CSockserver::getinstance()->GetMouseEvent(mouse)) {
        switch (mouse.nButton) { //哪一个鼠标按钮被按下
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
        if(nFlags != 8)  SetCursorPos(mouse.Ptxy.x, mouse.Ptxy.y);//有按键按下，设置坐标。
        switch (mouse.nAction) { //点击了几下
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
            mouse_event(MOUSEEVENTF_MOVE, mouse.Ptxy.x, mouse.Ptxy.y, 0, GetMessageExtraInfo());
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

int  SendScreen() { //发送屏幕截图

    CImage screen;//GDI  全局设备接口。
    HDC hScreen = GetDC(NULL);
    //获取屏幕的位图(用多少bit位来表示色彩) 一百来说24位（rgb每个一个字节）
    int nBitperpixel = GetDeviceCaps(hScreen, BITSPIXEL);
    int nWidth = GetDeviceCaps(hScreen,HORZRES);//获取屏幕的宽
    int nHeigth = GetDeviceCaps(hScreen, VERTRES);//获取屏幕的高
    screen.Create(nWidth, nHeigth, nBitperpixel);//创建一个画布
    //将当前屏幕上的图片绘制到刚才创建的画布上。
    BitBlt(screen.GetDC(), 0, 0, 1920, 1020, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMen = GlobalAlloc(GMEM_MOVEABLE, 0);
    if (hMen == NULL) return -1;
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMen, TRUE, &pStream);
    //DWORD tick = GetTickCount64();//获取当前时间，毫秒级
    //screen.Save(pStream, Gdiplus::ImageFormatPNG);//更小一点，清晰度差不多
    //TRACE("pnd %d\r\n", GetTickCount64() - tick);
    //tick = GetTickCount64();
  /*  screen.Save(_T("test2022.jpeg"), Gdiplus::ImageFormatJPEG); 
    TRACE("jpeg %d\r\n", GetTickCount64() - tick);*/
    if (ret = S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);
        LARGE_INTEGER bg = { 0 };
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);
        PBYTE pData = (PBYTE)GlobalLock(hMen);
        SIZE_T nSize = GlobalSize(hMen);
        Cpacket pack(6, pData, nSize);
        //CSockserver::getinstance()->Send(pack);
        GlobalUnlock(hMen);
    }
    pStream->Release();
    GlobalFree(hMen);
    screen.ReleaseDC();
    return 0;
}

unsigned int threadID = 0;

unsigned __stdcall ThreadLockDlg(void* arg) {

    dig.Create(IDD_DIALOG_INFO, NULL); // 创建一个对话框
    dig.ShowWindow(SW_SHOW); //设置非模态对话框
    //遮蔽后台窗口。
  /*  CRect rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
    rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
    dig.MoveWindow(rect);
    rect.bottom *= 0.5;
    dig.SetWindowPos(&dig.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);*/ // 设置对话框位置。

    ShowCursor(false);//限制鼠标功能

    //隐藏任务栏
    //::ShowWindow(::FindWindow(_T("shell_trayWnd"), NULL), SW_HIDE);

   /* rect.left = 0;
    rect.top = 0;
    rect.right = 1;
    rect.bottom = 1;
    ClipCursor(rect);*///限制鼠标活动范围
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_KEYDOWN) {//有按键按下
            if (msg.wParam == 0x1B) {   //按键为esc
                break;
            }
        }
    }
    ShowCursor(true);
   // dig.DestroyWindow(); //销毁对话框

    //展示任务栏
    //::ShowWindow(::FindWindow(_T("shell_trayWnd"), NULL), SW_SHOW);

    dig.DestroyWindow(); //销毁对话框

    _endthreadex(0); //线程自动退出

    return 0;
}

int LockMachine()
{
    //加一个互斥锁。
    //为了防止控制端多次发送锁机命令而出现多次创建线程的情况。
    if ((dig.m_hWnd != NULL) && (dig.m_hWnd != INVALID_HANDLE_VALUE)) {
        //返回一个应答，代表锁机已经命令已经执行。
        Cpacket pack(7, NULL, 0);
        //CSockserver::getinstance()->Send(pack);
        return 0;
    } 
    //_beginthread(ThreadLockDlg, 0, NULL); // 拉起一个线程

    _beginthreadex(NULL, 0, ThreadLockDlg, NULL, 0, &threadID);

    Cpacket pack(7, NULL, 0);
    CSockserver::getinstance()->Send(pack);
    return 0;
}

int UnlockMachine()
{
    //将消息发送给指定的线程。
    PostThreadMessage(threadID, WM_KEYDOWN, 0x1B, 0);
    Cpacket pack(8, NULL, 0);
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
            CSockserver* pserver = CSockserver::getinstance();
            //创建对话框
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
            int nCmd = 2;
            switch (7) {
            case 1: //查看磁盘分区。
                my_MakeDriveInfo();
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
            case 6: //发送屏幕内容
                SendScreen();
                break;
            case 7: //锁机
                /* 锁机后为了防止在锁机函数阻塞，我们另起动一个线程，让该子线程去提示用户，主线程
                继续接收客户端（控制端）信息*/
                LockMachine();
                //延时500毫秒，如果不延时，可能第一个线程还没拉起来，第二个线程也开始了。线程不同步了
                //需要加一个互斥锁，来确保线程同步
                //Sleep(500);
                //LockMachine();
                break;
            case 8:// 解锁
                UnlockMachine();
                break;
            }
            Sleep(5000);
            UnlockMachine();
            while (dig.m_hWnd != NULL)
                Sleep(10);
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
