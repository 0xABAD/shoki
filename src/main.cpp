#include "bl_common.hpp"
#include "bl_winhelp.cpp" // allocate_console

#include <gdiplus.h>
#include <cstring>
#include <cassert>

namespace gp {
using namespace Gdiplus;
}

struct AppState {
    HHOOK     kb_hook;
    HINSTANCE hInstance;
    bool      hideWindow;
    bool      hasError;

    void check_status(gp::Status status) {
        if (status != gp::Ok && !hasError) {
            hasError = true;
            MessageBoxA(nullptr, "GDI+ error", "Error", MB_OK|MB_ICONEXCLAMATION);
        }
    }
};

static HWND WINDOW;

static bool ALT_KEY   = false;
static bool CTRL_KEY  = false;
static bool SHIFT_KEY = false;

constexpr u32 MAX_KEYS = 10;

static u32 VK_KEYS[MAX_KEYS];
static u32 KEY_POS = 0;

u32 last_pos()
{
    return (KEY_POS - 1) % MAX_KEYS;
}

void set_key(u32 vk_key)
{
    // avoid setting keys that are held down
    if (vk_key != VK_KEYS[last_pos()]) {
        VK_KEYS[KEY_POS] = vk_key;
        KEY_POS++;
        KEY_POS = KEY_POS % MAX_KEYS;
    }
}

wchar_t const * get_key(u32 vk_key, bool isShiftDown)
{
    switch (vk_key) {
    case 0x08: return L"BSPC";
    case 0x09: return L"TAB";
    case 0x0D: return L"ENTER";
    case 0x14: return L"CAPS";
    case 0x20: return L"SPC";
    case 0x21: return L"PG_UP";
    case 0x22: return L"PG_DOWN";
    case 0x23: return L"END";
    case 0x24: return L"HOME";
    case 0x25: return L"LEFT";
    case 0x26: return L"UP";
    case 0x27: return L"RIGHT";
    case 0x28: return L"DOWN";
    case 0x2D: return L"INS";
    case 0x2E: return L"DEL";
    case 0x30: return isShiftDown ? L")" : L"0";
    case 0x31: return isShiftDown ? L"!" : L"1";
    case 0x32: return isShiftDown ? L"@" : L"2";
    case 0x33: return isShiftDown ? L"#" : L"3";
    case 0x34: return isShiftDown ? L"$" : L"4";
    case 0x35: return isShiftDown ? L"%" : L"5";
    case 0x36: return isShiftDown ? L"^" : L"6";
    case 0x37: return isShiftDown ? L"&" : L"7";
    case 0x38: return isShiftDown ? L"*" : L"8";
    case 0x39: return isShiftDown ? L"(" : L"9";
    case 0x41: return L"A";
    case 0x42: return L"B";
    case 0x43: return L"C";
    case 0x44: return L"D";
    case 0x45: return L"E";
    case 0x46: return L"F";
    case 0x47: return L"G";
    case 0x48: return L"H";
    case 0x49: return L"I";
    case 0x4A: return L"J";
    case 0x4B: return L"K";
    case 0x4C: return L"L";
    case 0x4D: return L"M";
    case 0x4E: return L"N";
    case 0x4F: return L"O";
    case 0x50: return L"P";
    case 0x51: return L"Q";
    case 0x52: return L"R";
    case 0x53: return L"S";
    case 0x54: return L"T";
    case 0x55: return L"U";
    case 0x56: return L"V";
    case 0x57: return L"W";
    case 0x58: return L"X";
    case 0x59: return L"Y";
    case 0x5A: return L"Z";

    case 0xBA: return isShiftDown ? L":" : L";";
    case 0xBB: return isShiftDown ? L"=" : L"+";
    case 0xBC: return isShiftDown ? L"<" : L",";
    case 0xBD: return isShiftDown ? L"_" : L"-";
    case 0xBE: return isShiftDown ? L">" : L".";
    case 0xBF: return isShiftDown ? L"?" : L"/";
    case 0xC0: return isShiftDown ? L"~" : L"`";
    case 0xDB: return isShiftDown ? L"{" : L"[";
    case 0xDC: return isShiftDown ? L"|" : L"\\";
    case 0xDD: return isShiftDown ? L"}" : L"]";
    case 0xDE: return isShiftDown ? L"\"" : L"'";

    case 0x70: return L"F1";
    case 0x71: return L"F2";
    case 0x72: return L"F3";
    case 0x73: return L"F4";
    case 0x74: return L"F5";
    case 0x75: return L"F6";
    case 0x76: return L"F7";
    case 0x77: return L"F8";
    case 0x78: return L"F9";
    case 0x79: return L"F10";
    case 0x7A: return L"F11";
    case 0x7B: return L"F12";

    default: return L"";
    }
}

constexpr void log(char const *msg)
{
#if defined(DEBUG)
    puts(msg);
#endif
}

void draw_rectangle(gp::Graphics *graphics,
                    i32 x1,
                    i32 y1,
                    i32 width,
                    i32 height,
                    gp::Color color)
{
    auto x2 = x1 + width;
    auto y2 = y1 + height;

    gp::GraphicsPath path;
    gp::SolidBrush   brush(color);

    path.AddArc(x1, y1, 25, 25, -180, 90);
    path.AddArc(x2, y1, 25, 25,  -90, 90);
    path.AddArc(x2, y2, 25, 25,    0, 90);
    path.AddArc(x1, y2, 25, 25,   90, 90);
    graphics->FillPath(&brush, &path);
}

void draw_keypresses(HWND hwnd, HDC hdc, RECT clientArea)
{
    wchar_t buf[32] = {0};
    auto state   = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    auto vk      = VK_KEYS[last_pos()];
    auto key     = get_key(vk, SHIFT_KEY);
    auto ctrl    = CTRL_KEY ? L"CTRL + " : L"";
    auto alt     = ALT_KEY ? L"ALT + " : L"";
    auto shift   = SHIFT_KEY ? L"SHIFT + " : L"";
    auto written = swprintf_s(buf, L"%s%s%s%s", ctrl, alt, shift, key);

    assert(written != -1);

    gp::Graphics     graphics(hdc);
    gp::Font         font(L"Consolas", 15, gp::FontStyleRegular, gp::UnitPixel);
    gp::RectF        textDim(20, 55, 200, 15);
    gp::SolidBrush   white(gp::Color(255, 255, 255, 255));
    gp::Color        black(255, 1, 1, 1);
    gp::StringFormat format;

    graphics.SetSmoothingMode(gp::SmoothingModeHighQuality);
    draw_rectangle(&graphics, 20, 20, 200, 80, black);
    format.SetAlignment(gp::StringAlignmentCenter);
    graphics.DrawString(buf, -1, &font, textDim, &format, &white);
}

void render(HWND hwnd)
{
    auto state = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (state->hideWindow) {
        RECT wndDim;
        GetWindowRect(WINDOW, &wndDim);

        auto width   = wndDim.right - wndDim.left;
        auto height  = wndDim.bottom - wndDim.top;
        auto screen  = GetDC(nullptr);
        auto hdc     = CreateCompatibleDC(screen);
        auto bmap    = CreateCompatibleBitmap(screen, width, height);
        auto blend   = BLENDFUNCTION{};
        auto dstPt   = POINT{wndDim.left, wndDim.top};
        auto srcPt   = POINT{0, 0};
        auto wndSz   = SIZE{width, height};

        defer(DeleteObject(bmap));
        defer(DeleteDC(hdc));

        SelectObject(hdc, bmap);

        gp::Graphics graphics(hdc);
        gp::Pen      blackPen(gp::Color(255, 0, 0, 1), 5);

        graphics.DrawRectangle(&blackPen, 0, 0, i32(width), i32(height));
        draw_keypresses(hwnd, hdc, wndDim);

        blend.BlendOp             = AC_SRC_OVER;
        blend.BlendFlags          = 0;
        blend.AlphaFormat         = AC_SRC_ALPHA;
        blend.SourceConstantAlpha = 128;

        UpdateLayeredWindow(hwnd,
                            screen,
                            &dstPt,
                            &wndSz,
                            hdc,
                            &srcPt,
                            RGB(0, 0, 0),
                            &blend,
                            ULW_COLORKEY);
    }
    else {
        auto rect = RECT{};
        auto ps   = PAINTSTRUCT{};
        auto hdc  = BeginPaint(hwnd, &ps);

        defer(EndPaint(hwnd, &ps));
        GetClientRect(hwnd, &rect);
    
        auto width  = rect.right - rect.left - 1;
        auto height = rect.bottom - rect.top - 1;

        gp::Graphics   graphics(hdc);
        gp::SolidBrush white(gp::Color(255, 255, 255, 255));

        SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 255, LWA_ALPHA);
        graphics.FillRectangle(&white, 0, 0, width, height);
        draw_keypresses(hwnd, hdc, rect);
    }
}

LRESULT CALLBACK keyboard_hook(int code, WPARAM wParam, LPARAM lParam)
{
    auto state = (AppState *)GetWindowLongPtr(WINDOW, GWLP_USERDATA);
    
    if (code >= 0) {
        bool doRedraw = false;
        
        switch (wParam) {
        case WM_KEYDOWN: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            auto vk = kb->vkCode;

            if (vk == VK_LCONTROL || vk == VK_RCONTROL)
                CTRL_KEY = true;
            else if (vk == VK_LMENU || vk == VK_RMENU)
                ALT_KEY = true;
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT)
                SHIFT_KEY = true;
            else
                set_key(kb->vkCode);

            doRedraw = true;
        } break;

        case WM_KEYUP: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            auto vk = kb->vkCode;

            if (vk == VK_LCONTROL || vk == VK_RCONTROL)
                CTRL_KEY = false;
            if (vk == VK_LMENU || vk == VK_RMENU)
                ALT_KEY = false;
            if (vk == VK_LSHIFT || vk == VK_RSHIFT) {
                SHIFT_KEY = false;
            }

            auto toggleWindow = (vk == VK_F6 && SHIFT_KEY && CTRL_KEY && ALT_KEY);
            if (toggleWindow) {
                state->hideWindow = !state->hideWindow;

                if (state->hideWindow) {
                    auto current = GetWindowLong(WINDOW, GWL_EXSTYLE);
                    auto cleared = current & ~WS_EX_LAYERED;

                    SetWindowLong(WINDOW, GWL_EXSTYLE, cleared);
                    SetWindowLong(WINDOW, GWL_EXSTYLE, cleared | WS_EX_LAYERED);
                }
            }

            doRedraw = true;
        } break;

        /*
         * The ALT key is treated differently as a system key.  So you have to
         * listen to this message, along with any of its letter combinations to see
         * that you have hit ALT + H or something similiar.  Furthermore, to make
         * matters worse, the contol and shift keys send different messages when
         * the ALT key is held down.  The shift key is only recognized as SYSKEYDOWN
         * while control is still a regular KEYDOWN when ALT is held and both shift
         * and control are recognized as SYSKEYUP if they are released before releasing
         * the ALT key.
         */
        case WM_SYSKEYDOWN: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            auto vk = kb->vkCode;

            if (vk == VK_LMENU || vk == VK_RMENU)
                ALT_KEY  = true;
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT)
                SHIFT_KEY = true;
            else
                set_key(kb->vkCode);

            doRedraw = true;
        } break;

        case WM_SYSKEYUP: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            auto vk = kb->vkCode;

            if (vk == VK_LSHIFT || vk == VK_RSHIFT)
                SHIFT_KEY = false;
            if (vk == VK_LCONTROL || vk == VK_RCONTROL)
                CTRL_KEY = false;

            doRedraw = true;
        } break;

        } // end switch

        if (doRedraw) {
            auto flags = RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN;
            RedrawWindow(WINDOW, nullptr, nullptr, flags);
        }
    }

    return CallNextHookEx(nullptr, code, wParam, lParam);
}

LRESULT CALLBACK win_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto state = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) {

    case WM_CREATE: {
        auto data = (CREATESTRUCT *)lParam;

        state = (AppState *)data->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(state));

        state->kb_hook = SetWindowsHookEx(WH_KEYBOARD_LL,
                                          &keyboard_hook,
                                          state->hInstance,
                                          0);
        if (state->kb_hook == nullptr)
            log("Failed to set keyboard hook");

        return 0;
    } break;

    case WM_PAINT: {
        render(hwnd);
        return 0;
    } break;

    case WM_DESTROY: {
        if (!UnhookWindowsHookEx(state->kb_hook))
            log("Failed to unkook keyboard hook");

        PostQuitMessage(0);

        return 0;
    } break;

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
#if defined(DEBUG)
    if (!bl::w32::allocate_console())
        return 0;
#endif
    
    auto wndClass = WNDCLASSEX{};
    auto state    = AppState{};
    auto gpInput  = gp::GdiplusStartupInput{};
    auto gpToken  = ULONG_PTR{};

    gp::GdiplusStartup(&gpToken, &gpInput, nullptr);
    defer(gp::GdiplusShutdown(gpToken));

    state.hInstance = hinstance;

    wndClass.cbSize        = sizeof(wndClass);
    wndClass.style         = CS_VREDRAW|CS_HREDRAW;
    wndClass.lpfnWndProc   = &win_proc;
    wndClass.hInstance     = hinstance;
    wndClass.lpszClassName = "pwThiefWClass";

    auto didRegister = RegisterClassEx(&wndClass);
    if (!didRegister) {
        log("Failed to register class\n");
        return 0;
    }

    auto hwnd = CreateWindowEx( 
        WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_LAYERED,
        wndClass.lpszClassName, // class name                   
        "pwThief",              // window name                  
        WS_OVERLAPPEDWINDOW,    // overlapped window            
        CW_USEDEFAULT,          // default horizontal position  
        CW_USEDEFAULT,          // default vertical position    
        600,                    // default width
        600,                    // default height
        (HWND) nullptr,         // no parent or owner window    
        (HMENU) nullptr,        // class menu used              
        hinstance,              // instance handle              
        &state);                // app state data      
 
    if (!hwnd) {
        log("Failed to create window\n");
        return 0;
    }
    else WINDOW = hwnd;
 
    ShowWindow(hwnd, SW_SHOW); 
    render(hwnd);

    auto msg = MSG{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
