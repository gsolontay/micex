/***********************
***	www.solontay.com ***
************************/

/***********************
** georgy@solontay.com *
************************/

#pragma comment(lib, "micex_md_142.lib")
#include "micex_md_142.h"
#include <atomic>
#include <process.h>
#include <windows.h>

#define MAX_LOADSTRING          100
#define IDC_MICEXVIEWER			109
#define IDM_EXIT				105

const int _px_width = 90;
const int _qty_width = 80;
const int _q_height = 20;
const int _t_height = 18;

const size_t _x_instr = 8;
const size_t _y_instr = 2;
const size_t _instr = _x_instr * _y_instr;

int wWidth, wHeight;

const COLORREF _aBkRGB = RGB(220, 190, 190);
const COLORREF _bBkRGB = RGB(180, 220, 180);
const COLORREF _bkRGB = RGB(220, 220, 200);
const COLORREF _tBkRGB = RGB(230, 230, 230);

const int
    DefaultPriority = 0,    /// not set, use OS default 
    IdlePriority = 1,       /// Idle thread priority
    LowestPriority = 2,     /// Lower than LowPriority
    LowPriority = 3,        /// Lower than NormalPriority
    NormalPriority = 4,     /// Normal thread priority
    HighPriority = 5,       /// Higher than NormalPriority
    HighestPriority = 6,    /// Higher than HighPriority
    RealtimePriority = 15;   /// Realtime thread priority

class AstsThread {
private:
    HANDLE m_handle;
    int priority_;
    static void starthook(void* ptr) {
        ((AstsThread*)ptr)->run();
    }
public:
    HANDLE getHandle() const { return m_handle; }
    AstsThread() : priority_(0) {}
    void start() {
        m_handle = (HANDLE)_beginthread(&this->starthook, 0, (void*)this);
        if (priority_ != 0) {
            SetThreadPriority(m_handle, priority_);
        }
    }
    virtual void run() = 0;
    ~AstsThread() {}
    void join() {
        WaitForSingleObject(m_handle, INFINITE);
    }
    void setCurrentThreadPriority(const int priority) {
        priority_ = priority;
    }
    void sleep(const size_t ms) {
        Sleep(ms);
    }
};

class AstsMutex {
    std::atomic_flag flag{ ATOMIC_FLAG_INIT };
public:
    inline void lock() {
        while (flag.test_and_set(std::memory_order_acquire));
    }
    inline bool tryLock() {
        return !(flag.test_and_set(std::memory_order_acquire));
    }
    inline void unlock() {
        flag.clear(std::memory_order_release);
    }
};


struct Quote {
    double px;
    size_t qty;
    LARGE_INTEGER timer;
    size_t ul;
    Quote(const double p, const size_t q) {
        px = p; qty = q;
        QueryPerformanceCounter(&timer);
        ul = true;
    }
};

HFONT font, tFont;
HBRUSH hbrBk, tHbrBk, bHbrBk, aHbrBk;
HWND bidPxHwnd[_instr][10], bidQtyHwnd[_instr][10], askPxHwnd[_instr][10], askQtyHwnd[_instr][10],
    tHwnd[_instr], hWnd;
std::vector<Quote> bid[_instr], ask[_instr];
std::vector<std::string> t;
volatile bool _working = true;
size_t wBidPxChange[_instr], wAskPxChange[_instr];
bool wBidQtyChange[_instr][10], wAskQtyChange[_instr][10];
AstsMutex instrBidLocker[_instr], instrAskLocker[_instr], ctBidLocker[_instr], ctAskLocker[_instr];
HINSTANCE hInst;                                
WCHAR szTitle[MAX_LOADSTRING];                  
WCHAR szWindowClass[MAX_LOADSTRING];  
LARGE_INTEGER freq;

inline std::wstring pxStr(const double px) {
    std::wstring str = std::to_wstring(px);
    str.resize(8);
    return str;
}

inline std::wstring qtyStr(const size_t qty) {
    std::wstring str = std::to_wstring(qty).append(L" ");
    return str;
}

inline bool ulca(const size_t i, const size_t l) {
    
    if (!ask[i][l].ul) return false;
    LARGE_INTEGER ct;
    QueryPerformanceCounter(&ct);
    if (double(ct.QuadPart - ask[i][l].timer.QuadPart) / freq.QuadPart > 10.0) {
        ask[i][l].ul = false;
        return true;
    }
    
    return false;
}

inline bool ulcb(const size_t i, const size_t l) {
    
    if (!bid[i][l].ul) return false;
    LARGE_INTEGER ct;
    QueryPerformanceCounter(&ct);
    if (double(ct.QuadPart - bid[i][l].timer.QuadPart) / freq.QuadPart > 10.0) {
        bid[i][l].ul = false;
        return true;
    }
    
    return false;
}

class AstsMonitor : public AstsThread {
    size_t i, l, c, n; std::wstring s;
public:
    void run() {
        while (_working) {
            for (n = 0; n < _instr; ++n) {
                instrBidLocker[n].lock();
                l = bid[n].size();
                c = wBidPxChange[n];
                wBidPxChange[n] = 10;
                for (i = 0; i < 10; ++i) {
                    if (i < l) {
                        if (i >= c || ulcb(n, i)) {
                            SendMessage(bidPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)pxStr(bid[n][i].px).c_str());
                            SendMessage(bidQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)qtyStr(bid[n][i].qty).c_str());
                        }
                        else {
                            if (wBidQtyChange[n][i]) {
                                wBidQtyChange[n][i] = false;
                                SendMessage(bidPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)pxStr(bid[n][i].px).c_str());
                                SendMessage(bidQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)qtyStr(bid[n][i].qty).c_str());
                            }
                        }
                    }
                    else {
                        if (i >= c) {
                            SendMessage(bidPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                            SendMessage(bidQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                        }
                        else {
                            if (wBidQtyChange[n][i]) {
                                wBidQtyChange[n][i] = false;
                                SendMessage(bidPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                                SendMessage(bidQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                            }
                        }
                    }
                }
                instrBidLocker[n].unlock();
                instrAskLocker[n].lock();
                l = ask[n].size();
                c = wAskPxChange[n];
                wAskPxChange[n] = 10;
                for (i = 0; i < 10; ++i) {
                    if (i < l) {
                        if (i >= c || ulca(n, i)) {
                            SendMessage(askPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)pxStr(ask[n][i].px).c_str());
                            SendMessage(askQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)qtyStr(ask[n][i].qty).c_str());
                        }
                        else {
                            if (wAskQtyChange[n][i]) {
                                wAskQtyChange[n][i] = false;
                                SendMessage(askPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)pxStr(ask[n][i].px).c_str());
                                SendMessage(askQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)qtyStr(ask[n][i].qty).c_str());
                            }
                        }
                    }
                    else {
                        if (i >= c) {
                            SendMessage(askPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                            SendMessage(askQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                        }
                        else {
                            if (wAskQtyChange[n][i]) {
                                wAskQtyChange[n][i] = false;
                                SendMessage(askPxHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                                SendMessage(askQtyHwnd[n][i], WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
                            }
                        }
                    }
                }
                instrAskLocker[n].unlock();
            }
            this->sleep(30);
        }
    }
};


AstsMonitor* am;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CTLCOLORSTATIC: {
        const ptrdiff_t _hm = ptrdiff_t(GetMenu(HWND(lParam)));
        if (_hm == 50000) {
            SetTextColor(HDC(wParam), RGB(0, 0, 0));
            SetBkColor(HDC(wParam), _tBkRGB);
            return (INT_PTR)tHbrBk;
        }
        else if (_hm % 100 > 49) {
            const ptrdiff_t _i = (_hm % 10000) / 100;
            const ptrdiff_t _l = _hm % 100 - 50;
            LARGE_INTEGER ct;
            QueryPerformanceCounter(&ct);
            SetBkColor(HDC(wParam), _bkRGB);
            ctAskLocker[_i].lock();
            if (ask[_i].size() > _l && double(ct.QuadPart - ask[_i][_l].timer.QuadPart) / freq.QuadPart < 10.0) {
                ctAskLocker[_i].unlock();
                SetTextColor(HDC(wParam), RGB(220, 0, 0));
                SetBkColor(HDC(wParam), _aBkRGB);
                return (INT_PTR)aHbrBk;
            }
            else {
                ctAskLocker[_i].unlock();
                SetTextColor(HDC(wParam), RGB(220, 0, 0));
                SetBkColor(HDC(wParam), _bkRGB);
                return (INT_PTR)hbrBk;
            }
        }
        else {
            const ptrdiff_t _i = (_hm % 10000) / 100;
            const ptrdiff_t _l = _hm % 100;
            LARGE_INTEGER ct;
            QueryPerformanceCounter(&ct);
            ctBidLocker[_i].lock();
            if (bid[_i].size() > _l && double(ct.QuadPart - bid[_i][_l].timer.QuadPart) / freq.QuadPart < 10.0) {
                ctBidLocker[_i].unlock();
                SetTextColor(HDC(wParam), RGB(0, 130, 0));
                SetBkColor(HDC(wParam), _bBkRGB);
                return (INT_PTR)bHbrBk;
            }
            else {
                ctBidLocker[_i].unlock();
                SetTextColor(HDC(wParam), RGB(0, 130, 0));
                SetBkColor(HDC(wParam), _bkRGB);
                return (INT_PTR)hbrBk;
            }
        }
    }
                          
    case WM_PAINT: {

        PAINTSTRUCT ps; int x = wWidth - 1, y = 0, xi=_x_instr, yi=_y_instr; 

        HDC hdc = BeginPaint(hWnd, &ps);
        while (yi > 0) {

            MoveToEx(hdc, 0, y, NULL);
            LineTo(hdc, x, y);

            y += _t_height + 1;
            MoveToEx(hdc, 0, y, NULL);
            LineTo(hdc, x, y);

            y += _q_height * 10 + 1;
            MoveToEx(hdc, 0, y, NULL);
            LineTo(hdc, x, y);

            y += _q_height * 10 + 1;
            MoveToEx(hdc, 0, y, NULL);
            LineTo(hdc, x, y);

            --yi;
        }
        x = _px_width + _qty_width;
        y = wHeight - 1;
        while (xi > 1) {
            MoveToEx(hdc, x, 0, NULL);
            LineTo(hdc, x, y);
             x += _px_width + _qty_width + 1;
           --xi;
        }



        EndPaint(hWnd, &ps);
        break;
    }
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = 0;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName  = L"main_window";
    wcex.hIconSm = 0;
    return RegisterClassExW(&wcex);
}

void printStr(const std::string str) {
}

void onError() {
    _working = false;
}


void recStart(const size_t instr) {
    bid[instr].clear();
    ask[instr].clear();
}

void recStop(const size_t instr) {
}

void newBidLine(const size_t instr, const size_t line, const double px, const size_t qty) {
    instrBidLocker[instr].lock();
    ctBidLocker[instr].lock();
    bid[instr].insert(bid[instr].begin() + line, Quote(px, qty));
    ctBidLocker[instr].unlock();
    if (line < 10) {
        wBidPxChange[instr] = min(wBidPxChange[instr], line);
    }
    instrBidLocker[instr].unlock();
}

void newAskLine(const size_t instr, const size_t line, const double px, const size_t qty) {
    instrAskLocker[instr].lock();
    ctAskLocker[instr].lock();
    ask[instr].insert(ask[instr].begin() + line, Quote(px, qty));
    ctAskLocker[instr].unlock();
    if (line < 10) {
        wAskPxChange[instr] = min(wAskPxChange[instr], line);
    }
    instrAskLocker[instr].unlock();
}

void bidLineChange(const size_t instr, const size_t line, const double px, const size_t qty) {
    instrBidLocker[instr].lock();
    bid[instr][line].qty = qty;
    QueryPerformanceCounter(&bid[instr][line].timer);
    if (!bid[instr][line].ul) bid[instr][line].ul = true;
    if (line < 10) {
        wBidQtyChange[instr][line] = true;
    }
    instrBidLocker[instr].unlock();
}

void askLineChange(const size_t instr, const size_t line, const double px, const size_t qty) {
    instrAskLocker[instr].lock();
    ask[instr][line].qty = qty;
    QueryPerformanceCounter(&ask[instr][line].timer);
    if (!ask[instr][line].ul) ask[instr][line].ul = true;
     if (line < 10) {
        wAskQtyChange[instr][line] = true;
    }
    instrAskLocker[instr].unlock();
}

void bidLineRemove(const size_t instr, const size_t line, const double px) {
    instrBidLocker[instr].lock();
    ctBidLocker[instr].lock();
    bid[instr].erase(bid[instr].begin() + line);
    ctBidLocker[instr].unlock();
    if (line < 10) {
        wBidPxChange[instr] = min(wBidPxChange[instr], line);
    }
    instrBidLocker[instr].unlock();
}

void askLineRemove(const size_t instr, const size_t line, const double px) {
    instrAskLocker[instr].lock();
    ctAskLocker[instr].lock();
    ask[instr].erase(ask[instr].begin() + line);
    ctAskLocker[instr].unlock();
    if (line < 10) {
        wAskPxChange[instr] = min(wAskPxChange[instr], line);
    }
    instrAskLocker[instr].unlock();
}

BOOL InitInstance(HINSTANCE hInstance) {
   _onInfoMessage = printStr;
   _onFatalError = onError;
   _onEquityRecoveryStart = recStart;
   _onEquityRecoveryComplete = recStop;
   _onEquityNewBidLine = newBidLine;
   _onEquityNewAskLine = newAskLine;
   _onEquityBidLineChange = bidLineChange;
   _onEquityAskLineChange = askLineChange;
   _onEquityBidLineRemove = bidLineRemove;
   _onEquityAskLineRemove = askLineRemove;

   OLR_FOND_Port[0] = 16041;
   OLR_FOND_Port[1] = 17041;
   OLR_FOND_MultiAddr[0] = "239.195.1.41";
   OLR_FOND_MultiAddr[1] = "239.195.1.169";
   OLS_FOND_Port[0] = 16042;
   OLS_FOND_Port[1] = 17042;
   OLS_FOND_MultiAddr[0] = "239.195.1.42";
   OLS_FOND_MultiAddr[1] = "239.195.1.170";
   OLS_FOND_IP[0] = "91.203.253.227";
   OLS_FOND_IP[1] = "91.203.255.227";

   t.push_back("SBER");
   t.push_back("GAZP");
   t.push_back("GMKN");
   t.push_back("ROSN");
   t.push_back("VTBR");
   t.push_back("LKOH");
   t.push_back("NVTK");
   t.push_back("TATN");

   t.push_back("CHMF");
   t.push_back("SNGS");
   t.push_back("NLMK");
   t.push_back("PLZL");
   t.push_back("MGNT");
   t.push_back("RTKM");
   t.push_back("GLTR");
   t.push_back("TCSG");

   QueryPerformanceFrequency(&freq);

   hbrBk = CreateSolidBrush(_bkRGB);
   aHbrBk = CreateSolidBrush(_aBkRGB);
   bHbrBk = CreateSolidBrush(_bBkRGB);
   tHbrBk = CreateSolidBrush(_tBkRGB);
   hInst = hInstance;

   const int _caption = GetSystemMetrics(SM_CYCAPTION);
   const int _xedge = GetSystemMetrics(SM_CXEDGE);
   const int _yedge = GetSystemMetrics(SM_CYEDGE);

   const int _x_shift = _px_width + _qty_width + 1;
   const int _y_shift = _q_height * 20 + _t_height + 3;

   wHeight = _y_instr * (_q_height * 20) +_y_instr * _t_height + _y_instr + 1;
   wWidth = _x_instr * (_qty_width + _px_width) + _x_instr;

   hWnd = CreateWindowW(L"main_window", L"MicexViewer", WS_POPUP | WS_CAPTION | WS_SYSMENU, 100, 100, wWidth + _xedge * 2, wHeight + _caption + 2 * _yedge,
       nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) return FALSE;
   std::wstring str;
   font = CreateFont(-16, 0, 0, 0, 700, ANSI_CHARSET, 0u, 0u, 0u, 0u, 0u, 0u, 0u, TEXT("Courier New"));
   tFont = CreateFont(-16, 0, 0, 0, 400, ANSI_CHARSET, 0u, 0u, 0u, 0u, 0u, 0u, 0u, TEXT("Courier New"));


   for (size_t yi = 0; yi < _y_instr; ++yi) {
       for (size_t xi = 0; xi < _x_instr; ++xi) {
           const size_t _n = yi * _x_instr + xi;
           tHwnd[_n] = CreateWindow(L"Static", L"", WS_CHILD | WS_VISIBLE | SS_CENTER,
               xi * _x_shift, 1 + yi * _y_shift, _px_width + _qty_width, _t_height, hWnd, (HMENU)50000, hInstance, NULL);
           SendMessage(tHwnd[_n], WM_SETFONT, (WPARAM)tFont, TRUE);
           SetWindowText(tHwnd[_n], std::wstring(t[_n].begin(), t[_n].end()).c_str());

           for (size_t i = 0; i < 10; ++i) {
               askPxHwnd[_n][9 - i] = CreateWindow(L"Static", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                   xi * _x_shift, i * _q_height + _t_height + 2 + yi * _y_shift, _px_width, _q_height, hWnd, 
                   (HMENU)(40000 + 100 * _n + 50 + 9 - i), hInstance, NULL);
               SendMessage(askPxHwnd[_n][9 - i], WM_SETFONT, (WPARAM)font, TRUE);
               askQtyHwnd[_n][9 - i] = CreateWindow(L"Static", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                   _px_width + xi * _x_shift, i * _q_height + _t_height + 2 + yi * _y_shift, _qty_width, _q_height, hWnd, 
                   (HMENU)(40000 + 100 * _n + 50 + 9 - i), hInstance, NULL);
               SendMessage(askQtyHwnd[_n][9 - i], WM_SETFONT, (WPARAM)font, TRUE);

               bidPxHwnd[_n][i] = CreateWindow(L"Static", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                   xi * _x_shift, (i + 10) * _q_height + _t_height + 3 + yi * _y_shift, _px_width, _q_height, hWnd, 
                   (HMENU)(40000 + 100 * _n + i), hInstance, NULL);
               SendMessage(bidPxHwnd[_n][i], WM_SETFONT, (WPARAM)font, TRUE);
               bidQtyHwnd[_n][i] = CreateWindow(L"Static", L"", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                   _px_width + xi * _x_shift, (i + 10) * _q_height + _t_height + 3 + yi * _y_shift, _qty_width, _q_height, hWnd, 
                   (HMENU)(40000 + 100 * _n + i), hInstance, NULL);
               SendMessage(bidQtyHwnd[_n][i], WM_SETFONT, (WPARAM)font, TRUE);
           }
       }
   }
   ShowWindow(hWnd, SW_SHOW);
   UpdateWindow(hWnd);
   astsInit(2, 20, t, true);
   return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    MyRegisterClass(hInstance);
    if (!InitInstance (hInstance)) return FALSE;
    MSG msg;
    am = new AstsMonitor();
    am->start();
    while (_working && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            _working = false;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}



