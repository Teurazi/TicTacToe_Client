// TicTacClint.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "TicTacClint.h"
#include <WinSock2.h>
#include <process.h>
#include <iostream>
#include "resource.h"
#pragma comment(lib,"ws2_32.lib")

#define MAX_LOADSTRING 100
#define PORT	3500
#define BUF_SIZE 1024

#define ID_EDIT_InIP 1001   //IP 입력하는 창 id
#define ID_EDIT_Info 1002   //서버와의 연결을 표시해주는 창
#define ID_Button_InIP 2001 //ip 입력하는 버튼 id

// 전역 변수:
HWND hWnd;
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
int token;  //자신의 차례표시
int order;  //자신의 선,후공표시
int win;   //승패표시
bool end;   //게임종료표시

//setOrder가 0이상일때는 초기값을 받는걸로간주한다
//setOrder 값이 0일경우 게임중간으로 간주한다
//setToken == order 일경우 본인 차례로 간주하고 작동한다
//setResult에 자신의 order을 서버로 보낸다

struct Packet {
    int setOrder;   
    int setToken;
    int setResult;
    int _x;
    int _y;
};

Packet* packet;

HWND hEdit_InIP;     //IP 입력하는 창 핸들
HWND hButton_InIP;   //서버와의 연결을 표시해주는 핸들
HWND hEdit_Info;     //ip 입력하는 버튼 핸들

//인터넷 통신곤련 전역 변수
WSADATA WSAData;
SOCKADDR_IN addr;
SOCKET sock;
HANDLE recvThread;	//수신스레드, 송신스레드

unsigned WINAPI RecvMsg(void* arg);//쓰레드 수신함수

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.
    packet = new Packet();
    end = false;

    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == INVALID_SOCKET) {
    }

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TICTACCLINT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TICTACCLINT));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TICTACCLINT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TICTACCLINT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      500, 200, 400, 500, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        hEdit_InIP = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER , 25, 10, 125, 25, hWnd, (HMENU)ID_EDIT_InIP, hInst, NULL);  //ip입력 edit
        hButton_InIP = CreateWindow(L"button", L"접속", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 150, 10, 40, 25, hWnd, (HMENU)ID_Button_InIP, hInst, NULL);  //ip입력 button
        hEdit_Info = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY, 200, 10, 125, 25, hWnd, (HMENU)ID_EDIT_Info, hInst, NULL);  //
        break;
    case WM_LBUTTONDOWN: {
        if (!end) {
            int x;
            int y;
            x = LOWORD(lParam);
            y = HIWORD(lParam);
            //기본값은 -1로한다 이유는 다른곳을 클릭했을때 잘못된 좌표를 보내지 않기위해서이다
            packet->_x = -1;
            packet->_y = -1;
            if (packet->setToken == order && order != 0) {
                if ((x > 25 && y > 50) && (x < 125 && y < 150)) {   //좌상단
                    packet->_x = 0;
                    packet->_y = 0;
                }
                if ((x > 125 && y > 50) && (x < 225 && y < 150)) { //상단
                    packet->_x = 1;
                    packet->_y = 0;
                }
                if ((x > 255 && y > 50) && (x < 325 && y < 150)) { //우상단
                    packet->_x = 2;
                    packet->_y = 0;
                }
                if ((x > 25 && y > 150) && (x < 125 && y < 250)) { //좌측
                    packet->_x = 0;
                    packet->_y = 1;
                }
                if ((x > 125 && y > 150) && (x < 225 && y < 250)) { //중앙
                    packet->_x = 1;
                    packet->_y = 1;
                }
                if ((x > 255 && y > 150) && (x < 325 && y < 250)) { //우측
                    packet->_x = 2;
                    packet->_y = 1;
                }
                if ((x > 25 && y > 250) && (x < 125 && y < 350)) {   //좌하단
                    packet->_x = 0;
                    packet->_y = 2;
                }
                if ((x > 125 && y > 250) && (x < 225 && y < 350)) { //하단
                    packet->_x = 1;
                    packet->_y = 2;
                }
                if ((x > 255 && y > 250) && (x < 325 && y < 350)) { //우하단
                    packet->_x = 2;
                    packet->_y = 2;
                }
                if (packet->_x != -1 && packet->_y != -1) {
                    send(sock, (char*)packet, sizeof(Packet), 0);
                }
            }
        }
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
            case ID_Button_InIP: {
                char inputIP[128];
                wchar_t w_input[128];
                GetWindowText(hEdit_InIP, w_input, 256);

                int nLen = WideCharToMultiByte(CP_ACP, 0, w_input, -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, w_input, -1, inputIP, nLen, NULL, NULL);

                addr.sin_family = AF_INET;
                addr.sin_port = htons(PORT);
                addr.sin_addr.S_un.S_addr = inet_addr(inputIP); //아이피입력

                if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                    SetWindowText(hEdit_Info, L"연결실패"); // hEditOUTPUT의 Text 내보내기

                    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                    if (sock == INVALID_SOCKET) {
                         return 1;
                    }
                }
                else {//연결성공시
                    end = false;
                    SetWindowText(hEdit_Info, L"연결성공 및 대기"); // hEditOUTPUT의 Text 내보내기
                   recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&sock, 0, NULL);//메시지 수신용 쓰레드가 실행된다.

                }

            }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint(hWnd, &ps);

            //가로선 긋기
            MoveToEx(hdc,25,50,NULL);
            LineTo(hdc, 25, 350);

            MoveToEx(hdc, 125, 50, NULL);
            LineTo(hdc, 125, 350);

            MoveToEx(hdc, 225, 50, NULL);
            LineTo(hdc, 225, 350);

            MoveToEx(hdc, 325, 50, NULL);
            LineTo(hdc, 325, 350);
            //세로선 긋기
            MoveToEx(hdc, 25, 50, NULL);
            LineTo(hdc, 325, 50);

            MoveToEx(hdc, 25, 150, NULL);
            LineTo(hdc, 325, 150);

            MoveToEx(hdc, 25, 250, NULL);
            LineTo(hdc, 325, 250);

            MoveToEx(hdc, 25, 350, NULL);
            LineTo(hdc, 325, 350);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CREATE:
        break;
    case WM_INITDIALOG:
        if (win == 1) {
            SetDlgItemText(hDlg, IDC_STATIC_1, L"이겼습니다");
        }
        else if(win == 0) {
            SetDlgItemText(hDlg, IDC_STATIC_1, L"졌습니다");
        }
        else if (win == 2) {
            SetDlgItemText(hDlg, IDC_STATIC_1, L"비겼습니다");
        }
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }

    return (INT_PTR)FALSE;
}

unsigned WINAPI RecvMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg);//서버용 소켓을 전달한다.
    char message[1024];
    int strLen;
    while (1) {//반복
        strLen = recv(sock, message, sizeof(message) - 1, 0);//서버로부터 메시지를 수신한다.
        if (strLen <= 0) {
            SetWindowText(hEdit_Info, L"서버가 가득참"); // hEditOUTPUT의 Text 내보내기
            break;
        }
        message[strLen] = '\0';

        packet = (Packet*)message;

        if (packet->setOrder == 0) {
            if (packet->setResult == 1) {   // 1은 O (선공)
                HDC _hdc = GetDC(hWnd);
                Ellipse(_hdc, 25 + (packet->_x * 100), 50 + (packet->_y * 100), 25 + ((packet->_x + 1) * 100), 50 + ((packet->_y + 1) * 100));
            }
            else  if (packet->setResult == 2) { // 2는 X (후공)
                HDC _hdc = GetDC(hWnd);

                MoveToEx(_hdc, 25 + (packet->_x * 100), 50 + (packet->_y * 100), NULL);
                LineTo(_hdc, 25 + ((packet->_x + 1) * 100), 50 + ((packet->_y + 1) * 100));

                MoveToEx(_hdc, 25 + ((packet->_x + 1) * 100), 50 + (packet->_y * 100), NULL);
                LineTo(_hdc, 25 + (packet->_x * 100), 50 + ((packet->_y + 1) * 100));
            }
        }
        else  if (packet->setOrder == 3) {  //게임종료 일경우
            if (packet->setToken == order) {
                win = 1;
            }
            else {
                win = 0;
            }
            if (packet->setToken == 3) {    //토큰이 3인경우는 무승부이다
                win = 2;    //win이 2로둬서 무승부로 표시
            }
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            //게임종료후 정리
            closesocket(sock);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            end = true;
            SetWindowText(hEdit_Info, L"게임 및 연결종료"); // hEditOUTPUT의 Text 내보내기
            InvalidateRect(hWnd, NULL, TRUE); //다시 그린다.
            break;
        } else  if (packet->setOrder == 4) {
            closesocket(sock);
            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            end = true;
            SetWindowText(hEdit_Info, L"상대끊김 및 종료"); // hEditOUTPUT의 Text 내보내기
            InvalidateRect(hWnd, NULL, TRUE); //다시 그린다.
            break;
        } else  if (packet->setOrder != 0) {
            order = packet->setOrder;
        }

        if (packet->setToken == order && order != 0) {
            SetWindowText(hEdit_Info, L"나의차례");
        }
        else if (packet->setToken != order && order != 0) {
            SetWindowText(hEdit_Info, L"상대차례");
        }
    }
    return 0;
}