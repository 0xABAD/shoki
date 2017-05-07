#include "bl_common.hpp"
#include "bl_winhelp.cpp" // allocate_console

struct AppState {
    HHOOK     kb_hook;
    HINSTANCE hInstance;
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

char const * get_key(u32 vk_key, bool isShiftDown)
{
    switch (vk_key) {
    case 0x08: return "BSPC";
    case 0x09: return "TAB";
    case 0x0D: return "ENTER";
    case 0x14: return "CAPS";
    case 0x20: return "SPC";
    case 0x21: return "PG_UP";
    case 0x22: return "PG_DOWN";
    case 0x23: return "END";
    case 0x24: return "HOME";
    case 0x25: return "LEFT";
    case 0x26: return "UP";
    case 0x27: return "RIGHT";
    case 0x28: return "DOWN";
    case 0x2D: return "INS";
    case 0x2E: return "DEL";
    case 0x30: return isShiftDown ? ")" : "0";
    case 0x31: return isShiftDown ? "!" : "1";
    case 0x32: return isShiftDown ? "@" : "2";
    case 0x33: return isShiftDown ? "#" : "3";
    case 0x34: return isShiftDown ? "$" : "4";
    case 0x35: return isShiftDown ? "%" : "5";
    case 0x36: return isShiftDown ? "^" : "6";
    case 0x37: return isShiftDown ? "&" : "7";
    case 0x38: return isShiftDown ? "*" : "8";
    case 0x39: return isShiftDown ? "(" : "9";
    case 0x41: return "A";
    case 0x42: return "B";
    case 0x43: return "C";
    case 0x44: return "D";
    case 0x45: return "E";
    case 0x46: return "F";
    case 0x47: return "G";
    case 0x48: return "H";
    case 0x49: return "I";
    case 0x4A: return "J";
    case 0x4B: return "K";
    case 0x4C: return "L";
    case 0x4D: return "M";
    case 0x4E: return "N";
    case 0x4F: return "O";
    case 0x50: return "P";
    case 0x51: return "Q";
    case 0x52: return "R";
    case 0x53: return "S";
    case 0x54: return "T";
    case 0x55: return "U";
    case 0x56: return "V";
    case 0x57: return "W";
    case 0x58: return "X";
    case 0x59: return "Y";
    case 0x5A: return "Z";

    case 0xBA: return isShiftDown ? ":" : ";";
    case 0xBB: return isShiftDown ? "=" : "+";
    case 0xBC: return isShiftDown ? "<" : ",";
    case 0xBD: return isShiftDown ? "_" : "-";
    case 0xBE: return isShiftDown ? ">" : ".";
    case 0xBF: return isShiftDown ? "?" : "/";
    case 0xC0: return isShiftDown ? "~" : "`";
    case 0xDB: return isShiftDown ? "{" : "[";
    case 0xDC: return isShiftDown ? "|" : "\\";
    case 0xDD: return isShiftDown ? "}" : "]";
    case 0xDE: return isShiftDown ? "\"" : "'";

    case 0x70: return "F1";
    case 0x71: return "F2";
    case 0x72: return "F3";
    case 0x73: return "F4";
    case 0x74: return "F5";
    case 0x75: return "F6";
    case 0x76: return "F7";
    case 0x77: return "F8";
    case 0x78: return "F9";
    case 0x79: return "F10";
    case 0x7A: return "F11";
    case 0x7B: return "F12";

    default: return "";
    }
}

constexpr void log(char const *msg)
{
#if defined(DEBUG)
    puts(msg);
#endif
}

LRESULT CALLBACK keyboard_hook(int code, WPARAM wParam, LPARAM lParam)
{
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

        if (doRedraw)
            RedrawWindow(WINDOW, nullptr, nullptr, RDW_INVALIDATE|RDW_ERASENOW);
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
        char buf[32] = {};
        auto vk      = VK_KEYS[last_pos()];
        auto key     = get_key(vk, SHIFT_KEY);
        auto ctrl    = CTRL_KEY ? "CTRL + " : "";
        auto alt     = ALT_KEY ? "ALT + " : "";
        auto shift   = SHIFT_KEY ? "SHIFT + " : "";

        snprintf(buf, COUNT_OF(buf), "%s%s%s%s", ctrl, alt, shift, key);

        auto rect   = RECT{};
        auto ps     = PAINTSTRUCT{};
        auto hdc    = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, HBRUSH(COLOR_WINDOW + 1));
        //DrawText(hdc, buf, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        TextOut(hdc, 50, 50, buf, strnlen(buf, COUNT_OF(buf)));
        EndPaint(hwnd, &ps);

        return 0;
    } break;

    case WM_DESTROY: {
        if (!UnhookWindowsHookEx(state->kb_hook))
            log("Failed to unkook keyboard hook");
        PostQuitMessage(0);
        return 0;
    } break;

    default: {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    } break;
    }
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int)
{
#if defined(DEBUG)
    if (!bl::w32::allocate_console())
        return 0;
#endif
    
    auto wndClass = WNDCLASSEX{};
    auto state    = AppState{};

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
        WS_EX_TOOLWINDOW|WS_EX_TOPMOST, // extended styles           
        wndClass.lpszClassName, // class name                   
        "pwThief",              // window name                  
        WS_OVERLAPPEDWINDOW,    // overlapped window            
        CW_USEDEFAULT,          // default horizontal position  
        CW_USEDEFAULT,          // default vertical position    
        300,                    // default width                
        200,                    // default height               
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
    UpdateWindow(hwnd);

    auto msg = MSG{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
