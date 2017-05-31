
struct KeyInfo {
    wchar_t const *key;
    bool doesShiftAffectKey;
};

KeyInfo get_key_info(u32 vk_key, bool isShiftDown)
{
#define KEY_INFO(key, doesAffect) KeyInfo { key, doesAffect }

    switch (vk_key) {
    case 0x08: return KEY_INFO(L"BSPC",    false);
    case 0x09: return KEY_INFO(L"TAB",     false);
    case 0x0D: return KEY_INFO(L"ENTER",   false);
    case 0x14: return KEY_INFO(L"CAPS",    false);
    case 0x20: return KEY_INFO(L"SPC",     false);
    case 0x21: return KEY_INFO(L"PG_UP",   false);
    case 0x22: return KEY_INFO(L"PG_DOWN", false);
    case 0x23: return KEY_INFO(L"END",     false);
    case 0x24: return KEY_INFO(L"HOME",    false);
    case 0x25: return KEY_INFO(L"LEFT",    false);
    case 0x26: return KEY_INFO(L"UP",      false);
    case 0x27: return KEY_INFO(L"RIGHT",   false);
    case 0x28: return KEY_INFO(L"DOWN",    false);
    case 0x2D: return KEY_INFO(L"INS",     false);
    case 0x2E: return KEY_INFO(L"DEL",     false);
    case 0x30: return KEY_INFO(isShiftDown ? L")" : L"0", true);
    case 0x31: return KEY_INFO(isShiftDown ? L"!" : L"1", true);
    case 0x32: return KEY_INFO(isShiftDown ? L"@" : L"2", true);
    case 0x33: return KEY_INFO(isShiftDown ? L"#" : L"3", true);
    case 0x34: return KEY_INFO(isShiftDown ? L"$" : L"4", true);
    case 0x35: return KEY_INFO(isShiftDown ? L"%" : L"5", true);
    case 0x36: return KEY_INFO(isShiftDown ? L"^" : L"6", true);
    case 0x37: return KEY_INFO(isShiftDown ? L"&" : L"7", true);
    case 0x38: return KEY_INFO(isShiftDown ? L"*" : L"8", true);
    case 0x39: return KEY_INFO(isShiftDown ? L"(" : L"9", true);
    case 0x41: return KEY_INFO(isShiftDown ? L"A" : L"a", true);
    case 0x42: return KEY_INFO(isShiftDown ? L"B" : L"b", true);
    case 0x43: return KEY_INFO(isShiftDown ? L"C" : L"c", true);
    case 0x44: return KEY_INFO(isShiftDown ? L"D" : L"d", true);
    case 0x45: return KEY_INFO(isShiftDown ? L"E" : L"e", true);
    case 0x46: return KEY_INFO(isShiftDown ? L"F" : L"f", true);
    case 0x47: return KEY_INFO(isShiftDown ? L"G" : L"g", true);
    case 0x48: return KEY_INFO(isShiftDown ? L"H" : L"h", true);
    case 0x49: return KEY_INFO(isShiftDown ? L"I" : L"i", true);
    case 0x4A: return KEY_INFO(isShiftDown ? L"J" : L"j", true);
    case 0x4B: return KEY_INFO(isShiftDown ? L"K" : L"k", true);
    case 0x4C: return KEY_INFO(isShiftDown ? L"L" : L"l", true);
    case 0x4D: return KEY_INFO(isShiftDown ? L"M" : L"m", true);
    case 0x4E: return KEY_INFO(isShiftDown ? L"N" : L"n", true);
    case 0x4F: return KEY_INFO(isShiftDown ? L"O" : L"o", true);
    case 0x50: return KEY_INFO(isShiftDown ? L"P" : L"p", true);
    case 0x51: return KEY_INFO(isShiftDown ? L"Q" : L"q", true);
    case 0x52: return KEY_INFO(isShiftDown ? L"R" : L"r", true);
    case 0x53: return KEY_INFO(isShiftDown ? L"S" : L"s", true);
    case 0x54: return KEY_INFO(isShiftDown ? L"T" : L"t", true);
    case 0x55: return KEY_INFO(isShiftDown ? L"U" : L"u", true);
    case 0x56: return KEY_INFO(isShiftDown ? L"V" : L"v", true);
    case 0x57: return KEY_INFO(isShiftDown ? L"W" : L"w", true);
    case 0x58: return KEY_INFO(isShiftDown ? L"X" : L"x", true);
    case 0x59: return KEY_INFO(isShiftDown ? L"Y" : L"y", true);
    case 0x5A: return KEY_INFO(isShiftDown ? L"Z" : L"z", true);

    case 0xBA: return KEY_INFO(isShiftDown ? L":" : L";",  true);
    case 0xBB: return KEY_INFO(isShiftDown ? L"=" : L"+",  true);
    case 0xBC: return KEY_INFO(isShiftDown ? L"<" : L",",  true);
    case 0xBD: return KEY_INFO(isShiftDown ? L"_" : L"-",  true);
    case 0xBE: return KEY_INFO(isShiftDown ? L">" : L".",  true);
    case 0xBF: return KEY_INFO(isShiftDown ? L"?" : L"/",  true);
    case 0xC0: return KEY_INFO(isShiftDown ? L"~" : L"`",  true);
    case 0xDB: return KEY_INFO(isShiftDown ? L"{" : L"[",  true);
    case 0xDC: return KEY_INFO(isShiftDown ? L"|" : L"\\", true);
    case 0xDD: return KEY_INFO(isShiftDown ? L"}" : L"]",  true);
    case 0xDE: return KEY_INFO(isShiftDown ? L"\"" : L"'", true);

    case 0x70: return KEY_INFO(L"F1",  false);
    case 0x71: return KEY_INFO(L"F2",  false);
    case 0x72: return KEY_INFO(L"F3",  false);
    case 0x73: return KEY_INFO(L"F4",  false);
    case 0x74: return KEY_INFO(L"F5",  false);
    case 0x75: return KEY_INFO(L"F6",  false);
    case 0x76: return KEY_INFO(L"F7",  false);
    case 0x77: return KEY_INFO(L"F8",  false);
    case 0x78: return KEY_INFO(L"F9",  false);
    case 0x79: return KEY_INFO(L"F10", false);
    case 0x7A: return KEY_INFO(L"F11", false);
    case 0x7B: return KEY_INFO(L"F12", false);

    default: return KEY_INFO(L"", false);
    } // switch
#undef KEY_INFO
}
