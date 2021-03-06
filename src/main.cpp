#include "bl_common.hpp"
#include "bl_winhelp.cpp"
#include "key_info.cpp"

#include <gdiplus.h>
#include <cstring>
#include <cassert>

namespace gp {
using namespace Gdiplus;
}

static HWND WINDOW;

struct KeyCombo {
    u32  vk_key;
    bool isAltDown;
    bool isCtrlDown;
    bool isShiftDown;
};

struct KeyComboIter {
    i32  index;
    bool hasWrapped;
};

constexpr u32 MAX_KEY_COMBOS = 8;

struct AppState {
    HHOOK     kb_hook;
    HINSTANCE hInstance;
    bool      hideWindow;
    bool      hasError;
    UINT_PTR  timerID;
    f32       opacity;
    UINT      fadeOutMilliseconds;
    DWORD     lastTime;
    DWORD     timerStartTime;

    /*
     * Key combos will be maintained in a stack.  When there are no
     * more key presses over a short period of time the stack will be
     * emptied (i.e. the key press rectangle fades out of view).  If
     * the user is typing quickly and overflows the stack buffer then
     * turns into a ring buffer.  A keyComboIndex of -1 indicates
     * there are no stored key combos.
     */
    KeyCombo keyCombos[MAX_KEY_COMBOS];
    u32      maxUserConfigCombos;
    i32      keyComboIndex;
    bool     isOverflowed;

    KeyCombo possibleCombo;

    void set_key(DWORD vk, bool isDownState) {
        if (vk == VK_LCONTROL || vk == VK_RCONTROL) {
            possibleCombo.isCtrlDown = isDownState;
        }
        else if (vk == VK_LMENU || vk == VK_RMENU) {
            possibleCombo.isAltDown = isDownState;
        }
        else if (vk == VK_LSHIFT || vk == VK_RSHIFT) {
            possibleCombo.isShiftDown = isDownState;
        }
        else if (!isDownState && get_key_info(vk, false).key != L"") {
            possibleCombo.vk_key = vk;
            add_combo();
        }
    }

    bool is_empty() { return keyComboIndex == -1; }

    KeyComboIter begin() {
        return KeyComboIter { keyComboIndex, false };
    }

    bool at_end(KeyComboIter iter) {
        if (isOverflowed && iter.hasWrapped)
            return iter.index == keyComboIndex;

        return iter.index == -1;
    }

    void incr(KeyComboIter *iter) {
        iter->index = iter->index - 1;

        if (isOverflowed && iter->index == -1) {
            iter->hasWrapped = true;
            iter->index      = maxUserConfigCombos - 1;
        }
    }

    KeyCombo get_combo(KeyComboIter iter) {
        assert(iter.index > -1 && iter.index < maxUserConfigCombos);
        return keyCombos[iter.index];
    }

    void add_combo() {
        ++keyComboIndex;
        if (keyComboIndex == maxUserConfigCombos) {
            isOverflowed  = true;
            keyComboIndex = 0;
        }

        keyCombos[keyComboIndex] = possibleCombo;
    }

    void reset_combos() {
        keyComboIndex = -1;
        isOverflowed  = false;
    }

    void set_max_combos(u32 maxCombos) {
        assert(maxCombos >= 1 && maxCombos <= MAX_KEY_COMBOS);
        maxUserConfigCombos = maxCombos;
        reset_combos();
    }

    void check_status(gp::Status status) {
        if (status != gp::Ok && !hasError) {
            hasError = true;
            MessageBoxA(nullptr, "GDI+ error", "Error", MB_OK|MB_ICONEXCLAMATION);
        }
    }
};

enum PlacementJustification {
    Justification_Left,
    Justification_Right,
    Justification_Center
};

struct Placement {
    i32 offset_x;
    i32 offset_y;
    i32 width;
    i32 height;

    PlacementJustification justification;
};

inline void log(char const *msg)
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
    constexpr i32 CORNER_RADIUS = 10;
    auto x2 = x1 + width - CORNER_RADIUS;
    auto y2 = y1 + height - CORNER_RADIUS;

    gp::GraphicsPath path;
    gp::SolidBrush   brush(color);

    path.AddArc(x1, y1, CORNER_RADIUS, CORNER_RADIUS, -180, 90);
    path.AddArc(x2, y1, CORNER_RADIUS, CORNER_RADIUS,  -90, 90);
    path.AddArc(x2, y2, CORNER_RADIUS, CORNER_RADIUS,    0, 90);
    path.AddArc(x1, y2, CORNER_RADIUS, CORNER_RADIUS,   90, 90);
    graphics->FillPath(&brush, &path);
}

void draw_keypresses(HWND hwnd, gp::Graphics *graphics, f32 opacity, Placement const &placement)
{
    auto state = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (state->is_empty())
        return;

    struct KeyPress {
        gp::RectF ltrDim;
        gp::RectF modDim;
        wchar_t const *key;
        wchar_t const *ctrl;
        wchar_t const *alt;
        wchar_t const *shift;
    };

    gp::Font letter(L"Consolas", 40, gp::FontStyleBold, gp::UnitPixel);
    gp::Font modifier(L"Consolas", 8, gp::FontStyleRegular, gp::UnitPixel);
    KeyPress keypresses[MAX_KEY_COMBOS];

    // Ensures that modifier text stack are left aligned.
    gp::StringFormat format;
    format.SetAlignment(gp::StringAlignmentNear);

    constexpr f32 MOD_LETTER_SPACING = -8.0f; // pixels
    constexpr f32 MOD_LINE_SPACING   = 2.0f;
    constexpr f32 COMBO_SPACING      = 2.0f;
    constexpr f32 BOX_PADDING        = 2.0f;

    f32 box_wd     = 2*BOX_PADDING;
    f32 box_ht     = 0.0f;
    i32 pressCount = 0;

    for (auto iter = state->begin(); !state->at_end(iter); state->incr(&iter)) {
        auto  combo   = state->get_combo(iter);
        auto  keyInfo = get_key_info(combo.vk_key, combo.isShiftDown);
        auto &press   = keypresses[pressCount];

        press.key   = keyInfo.key;
        press.ctrl  = combo.isCtrlDown ? L"CTRL" : L"";
        press.alt   = combo.isAltDown  ? L"ALT"  : L"";
        press.shift = L"";

        if (combo.isShiftDown && !keyInfo.doesShiftAffectKey)
            press.shift = L"SHIFT";

        gp::PointF pt;
        auto &ltr = press.ltrDim;
        auto &mod = press.modDim;

        graphics->MeasureString(press.key, -1, &letter, pt, &format, &ltr);
        box_wd += ltr.Width;
        box_ht  = ltr.Height > box_ht ? ltr.Height : box_ht;

        auto isAnyModDown = (combo.isCtrlDown || combo.isAltDown ||
                             (combo.isShiftDown && !keyInfo.doesShiftAffectKey));

        if (isAnyModDown) {
            graphics->MeasureString(L"SHIFT", -1, &modifier, pt, &format, &mod);
            box_wd += mod.Width + MOD_LETTER_SPACING;
        }
        ++pressCount;
    }

    // The state->empty() check earlier should make this true.
    assert(pressCount > 0);

    u8 alpha = u8(opacity * 255);
    gp::SolidBrush white(gp::Color(alpha, 255, 255, 255));
    gp::Color      black(alpha, 0, 0, 0);

    box_wd += (pressCount - 1) * COMBO_SPACING;
    box_ht += 2.0f*BOX_PADDING;

    f32 start_x = 0;
    f32 start_y = placement.height - placement.offset_y - box_ht;

    switch (placement.justification) {
    case Justification_Left:
        start_x = placement.offset_x;
        break;
    case Justification_Right:
        start_x = placement.width - placement.offset_x - box_wd;
        break;
    case Justification_Center:
        start_x = (placement.width / 2.0f) - (box_wd / 2.0f);
        break;
    }

    // This text rendering mode needs to be applied; otherwise, the alpha
    // value of the text color won't be properly applied.
    graphics->SetTextRenderingHint(gp::TextRenderingHintAntiAliasGridFit);
    graphics->SetSmoothingMode(gp::SmoothingModeHighQuality);
    draw_rectangle(graphics, start_x, start_y, box_wd, box_ht, black);

    f32 combo_x = BOX_PADDING + start_x;

    for (i32 idx = pressCount - 1; idx >= 0; --idx) {
        auto &press    = keypresses[idx];
        auto &ltr      = press.ltrDim;
        auto &mod      = press.modDim;
        auto  offset_y = (ltr.Height - (3.0f * mod.Height) - 2.0f) / 2.0f;
        auto  offset_x = mod.Width > 0 ? mod.Width + MOD_LETTER_SPACING : 0.0f;

        mod.Y = BOX_PADDING + offset_y + start_y;
        ltr.Y = BOX_PADDING + start_y;
        mod.X = combo_x;
        ltr.X = mod.X + offset_x;

        combo_x += COMBO_SPACING + ltr.Width + offset_x;

        graphics->DrawString(press.key, -1, &letter, ltr, &format, &white);
        graphics->DrawString(press.ctrl, -1, &modifier, mod, &format, &white);

        mod.Y += mod.Height;
        graphics->DrawString(press.alt, -1, &modifier, mod, &format, &white);

        mod.Y += mod.Height;
        graphics->DrawString(press.shift, -1, &modifier, mod, &format, &white);
    }
}

void update_opacity(AppState *state, DWORD currentTime)
{
    constexpr DWORD milliseconds = 1;
    auto elapsed  = currentTime - state->timerStartTime;
    auto interval = currentTime - state->lastTime;

    state->lastTime = currentTime;

    if (elapsed >= 300*milliseconds) {
        f32 fade  = f32(2 * state->fadeOutMilliseconds);
        f32 delta = interval / fade;

        state->opacity -= delta;
        if (state->opacity <= 0.0f) {
            state->opacity = 0.0f;
            KillTimer(WINDOW, state->timerID);
        }
    }
}

void render(HWND hwnd)
{
    constexpr i32 OFFSET_X = 20;
    constexpr i32 OFFSET_Y = 15;

    auto state = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (state->hideWindow) {
        auto wndDim = RECT{};
        auto place  = Placement{};

        GetWindowRect(hwnd, &wndDim);

        place.width         = wndDim.right - wndDim.left;
        place.height        = wndDim.bottom - wndDim.top;
        place.offset_x      = OFFSET_X;
        place.offset_y      = OFFSET_Y;
        place.justification = Justification_Center;

        auto screen  = GetDC(nullptr);
        auto hdc     = CreateCompatibleDC(screen);
        auto bmap    = CreateCompatibleBitmap(screen, place.width, place.height);

        defer(DeleteObject(bmap));
        defer(DeleteDC(hdc));
        defer(ReleaseDC(hwnd, screen));

        SelectObject(hdc, bmap);

        gp::Graphics graphics(hdc);

#if DEBUG
        gp::Pen blackPen(gp::Color(255, 0, 0, 0), 5);
        graphics.DrawRectangle(&blackPen, 0, 0, i32(place.width), i32(place.height));
#endif

        update_opacity(state, GetTickCount());
        draw_keypresses(hwnd, &graphics, state->opacity, place);

        auto dstPt = POINT{wndDim.left, wndDim.top};
        auto srcPt = POINT{0, 0};
        auto wndSz = SIZE{place.width, place.height};
        auto blend = BLENDFUNCTION{};

        blend.BlendOp             = AC_SRC_OVER;
        blend.BlendFlags          = 0;
        blend.AlphaFormat         = AC_SRC_ALPHA;
        blend.SourceConstantAlpha = 255;

        UpdateLayeredWindow(hwnd,
                            screen,
                            &dstPt,
                            &wndSz,
                            hdc,
                            &srcPt,
                            RGB(0, 0, 0),
                            &blend,
                            ULW_ALPHA);
    }
    else {
        auto rect  = RECT{};
        auto ps    = PAINTSTRUCT{};
        auto place = Placement{};
        auto hdc   = BeginPaint(hwnd, &ps);

        defer(EndPaint(hwnd, &ps));
        GetClientRect(hwnd, &rect);

        place.width         = rect.right - rect.left - 1;
        place.height        = rect.bottom - rect.top - 1;
        place.offset_x      = OFFSET_X;
        place.offset_y      = OFFSET_Y;
        place.justification = Justification_Center;

        gp::Graphics   graphics(hdc);
        gp::SolidBrush white(gp::Color(255, 255, 255, 255));
        gp::SolidBrush magenta(gp::Color(255, 255, 0, 255));

        auto top    = f32(place.height) * 0.2;
        auto bottom = f32(place.height) * 0.8;

        SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 255, LWA_COLORKEY);
        graphics.FillRectangle(&white, 0, 0, place.width, top);
        graphics.FillRectangle(&magenta, 0, top, place.width + 1, bottom + 1);

        auto textRect = RECT{};

        textRect.left   = 6;
        textRect.top    = 0;
        textRect.right  = LONG(place.width);
        textRect.bottom = LONG(top);

        DrawTextEx(hdc,
                   "Preview Mode:  Press CTRL + ALT + SHIFT + F6 to toggle window.",
                   -1,
                   &textRect,
                   DT_SINGLELINE|DT_VCENTER|DT_LEFT|DT_WORD_ELLIPSIS,
                   nullptr);

        draw_keypresses(hwnd, &graphics, state->opacity, place);
    }

    if (state->opacity == 0.0f)
        state->reset_combos();
}

VOID CALLBACK fade_out(HWND hwnd, UINT, UINT_PTR, DWORD dwTime)
{
    auto state = (AppState *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    auto flags = RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN;

    /*
     * When the window is set to WS_EX_LAYERED mode, that is everything
     * but the key presses will be transparent, then this callback
     * passed to SetTimer will never be called.  In fact, the WM_TIMER
     * message is never passed to the window procedure either.  Instead,
     * Windows appears to put the window on an separate animation loop
     * when the WS_EX_LAYERED is set, which may explain why the documentation
     * states that the window should be as small as possible when this
     * is the case for system performance reasons.  Due to this, we have
     * hoist out the opacity manipulation into a separate function so it
     * can be also called when the window is being rendered while the
     * app is set to transparent/hidden mode.
     */
    update_opacity(state, dwTime);
    RedrawWindow(hwnd, nullptr, nullptr, flags);
}

LRESULT CALLBACK keyboard_hook(int code, WPARAM wParam, LPARAM lParam)
{
    auto state = (AppState *)GetWindowLongPtr(WINDOW, GWLP_USERDATA);
    
    if (code >= 0) {
        bool doRedraw = false;
        
        switch (wParam) {
        case WM_KEYDOWN: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            state->set_key(kb->vkCode, true);
        } break;

        case WM_KEYUP: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            auto vk = kb->vkCode;

            state->set_key(vk, false);
            doRedraw = true;

            auto toggleWindow = (vk == VK_F6 &&
                                 state->possibleCombo.isShiftDown &&
                                 state->possibleCombo.isCtrlDown &&
                                 state->possibleCombo.isAltDown);

            if (toggleWindow) {
                /*
                 * MSDN documentation on layered windows state that when switching
                 * between UpdateLayeredWindow and SetLayeredWindowAttributes, which
                 * occurs when we hide or show the window, that the WS_EX_LAYERED
                 * attribute must be reset.
                 */
                auto current = GetWindowLong(WINDOW, GWL_EXSTYLE);
                auto cleared = current & ~WS_EX_LAYERED;

                state->hideWindow = !state->hideWindow;
                SetWindowLong(WINDOW, GWL_EXSTYLE, cleared);
                SetWindowLong(WINDOW, GWL_EXSTYLE, cleared | WS_EX_LAYERED);
            }
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
            state->set_key(kb->vkCode, true);
        } break;

        case WM_SYSKEYUP: {
            auto kb = (KBDLLHOOKSTRUCT *)lParam;
            state->set_key(kb->vkCode, false);
            doRedraw = true;
        } break;

        } // end switch

        if (doRedraw) {
            constexpr UINT milliseconds = 1;

            state->opacity        = 1.0f;
            state->timerStartTime = GetTickCount();
            state->lastTime       = state->timerStartTime;

            if (!SetTimer(WINDOW, state->timerID, 17*milliseconds, fade_out))
                log("failed to create window timer");

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
    } break;

    case WM_SYSCOMMAND: {
        if (wParam == SC_KEYMENU)
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

    state.hInstance           = hinstance;
    state.opacity             = 1.0f;
    state.timerID             = 1;
    state.fadeOutMilliseconds = 200;
    state.lastTime            = 0;
    state.set_max_combos(4);

    wndClass.cbSize        = sizeof(wndClass);
    wndClass.style         = CS_VREDRAW|CS_HREDRAW;
    wndClass.lpfnWndProc   = &win_proc;
    wndClass.hInstance     = hinstance;
    wndClass.lpszClassName = "shokiWClass";

    auto didRegister = RegisterClassEx(&wndClass);
    if (!didRegister) {
        log("Failed to register class\n");
        return 0;
    }

    auto hwnd = CreateWindowEx( 
        WS_EX_TOOLWINDOW|WS_EX_TOPMOST|WS_EX_LAYERED,
        wndClass.lpszClassName, // class name                   
        "Shoki",                // window name
        WS_OVERLAPPEDWINDOW,    // overlapped window            
        CW_USEDEFAULT,          // default horizontal position  
        CW_USEDEFAULT,          // default vertical position    
        650,                    // default width
        150,                    // default height
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
