/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2017 Martin Flöser <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "input/xkb/keyboard.h"

#include <QtTest>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace KWin;

class XkbTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testToQtKey_data();
    void testToQtKey();
};

// from kwindowsystem/src/platforms/xcb/kkeyserver.cpp
// adjusted to xkb
struct TransKey {
    Qt::Key keySymQt;
    xkb_keysym_t keySymX;
    Qt::KeyboardModifiers modifiers;
};

static const TransKey g_rgQtToSymX[]
    = {{Qt::Key_Escape, XKB_KEY_Escape, Qt::KeyboardModifiers()},
       {Qt::Key_Tab, XKB_KEY_Tab, Qt::KeyboardModifiers()},
       {Qt::Key_Backtab, XKB_KEY_ISO_Left_Tab, Qt::KeyboardModifiers()},
       {Qt::Key_Backspace, XKB_KEY_BackSpace, Qt::KeyboardModifiers()},
       {Qt::Key_Return, XKB_KEY_Return, Qt::KeyboardModifiers()},
       {Qt::Key_Insert, XKB_KEY_Insert, Qt::KeyboardModifiers()},
       {Qt::Key_Delete, XKB_KEY_Delete, Qt::KeyboardModifiers()},
       {Qt::Key_Pause, XKB_KEY_Pause, Qt::KeyboardModifiers()},
       {Qt::Key_Print, XKB_KEY_Print, Qt::KeyboardModifiers()},
       {Qt::Key_SysReq, XKB_KEY_Sys_Req, Qt::KeyboardModifiers()},
       {Qt::Key_Home, XKB_KEY_Home, Qt::KeyboardModifiers()},
       {Qt::Key_End, XKB_KEY_End, Qt::KeyboardModifiers()},
       {Qt::Key_Left, XKB_KEY_Left, Qt::KeyboardModifiers()},
       {Qt::Key_Up, XKB_KEY_Up, Qt::KeyboardModifiers()},
       {Qt::Key_Right, XKB_KEY_Right, Qt::KeyboardModifiers()},
       {Qt::Key_Down, XKB_KEY_Down, Qt::KeyboardModifiers()},
       {Qt::Key_PageUp, XKB_KEY_Prior, Qt::KeyboardModifiers()},
       {Qt::Key_PageDown, XKB_KEY_Next, Qt::KeyboardModifiers()},
       {Qt::Key_CapsLock, XKB_KEY_Caps_Lock, Qt::KeyboardModifiers()},
       {Qt::Key_NumLock, XKB_KEY_Num_Lock, Qt::KeyboardModifiers()},
       {Qt::Key_ScrollLock, XKB_KEY_Scroll_Lock, Qt::KeyboardModifiers()},
       {Qt::Key_F1, XKB_KEY_F1, Qt::KeyboardModifiers()},
       {Qt::Key_F2, XKB_KEY_F2, Qt::KeyboardModifiers()},
       {Qt::Key_F3, XKB_KEY_F3, Qt::KeyboardModifiers()},
       {Qt::Key_F4, XKB_KEY_F4, Qt::KeyboardModifiers()},
       {Qt::Key_F5, XKB_KEY_F5, Qt::KeyboardModifiers()},
       {Qt::Key_F6, XKB_KEY_F6, Qt::KeyboardModifiers()},
       {Qt::Key_F7, XKB_KEY_F7, Qt::KeyboardModifiers()},
       {Qt::Key_F8, XKB_KEY_F8, Qt::KeyboardModifiers()},
       {Qt::Key_F9, XKB_KEY_F9, Qt::KeyboardModifiers()},
       {Qt::Key_F10, XKB_KEY_F10, Qt::KeyboardModifiers()},
       {Qt::Key_F11, XKB_KEY_F11, Qt::KeyboardModifiers()},
       {Qt::Key_F12, XKB_KEY_F12, Qt::KeyboardModifiers()},
       {Qt::Key_F13, XKB_KEY_F13, Qt::KeyboardModifiers()},
       {Qt::Key_F14, XKB_KEY_F14, Qt::KeyboardModifiers()},
       {Qt::Key_F15, XKB_KEY_F15, Qt::KeyboardModifiers()},
       {Qt::Key_F16, XKB_KEY_F16, Qt::KeyboardModifiers()},
       {Qt::Key_F17, XKB_KEY_F17, Qt::KeyboardModifiers()},
       {Qt::Key_F18, XKB_KEY_F18, Qt::KeyboardModifiers()},
       {Qt::Key_F19, XKB_KEY_F19, Qt::KeyboardModifiers()},
       {Qt::Key_F20, XKB_KEY_F20, Qt::KeyboardModifiers()},
       {Qt::Key_F21, XKB_KEY_F21, Qt::KeyboardModifiers()},
       {Qt::Key_F22, XKB_KEY_F22, Qt::KeyboardModifiers()},
       {Qt::Key_F23, XKB_KEY_F23, Qt::KeyboardModifiers()},
       {Qt::Key_F24, XKB_KEY_F24, Qt::KeyboardModifiers()},
       {Qt::Key_F25, XKB_KEY_F25, Qt::KeyboardModifiers()},
       {Qt::Key_F26, XKB_KEY_F26, Qt::KeyboardModifiers()},
       {Qt::Key_F27, XKB_KEY_F27, Qt::KeyboardModifiers()},
       {Qt::Key_F28, XKB_KEY_F28, Qt::KeyboardModifiers()},
       {Qt::Key_F29, XKB_KEY_F29, Qt::KeyboardModifiers()},
       {Qt::Key_F30, XKB_KEY_F30, Qt::KeyboardModifiers()},
       {Qt::Key_F31, XKB_KEY_F31, Qt::KeyboardModifiers()},
       {Qt::Key_F32, XKB_KEY_F32, Qt::KeyboardModifiers()},
       {Qt::Key_F33, XKB_KEY_F33, Qt::KeyboardModifiers()},
       {Qt::Key_F34, XKB_KEY_F34, Qt::KeyboardModifiers()},
       {Qt::Key_F35, XKB_KEY_F35, Qt::KeyboardModifiers()},
       {Qt::Key_Super_L, XKB_KEY_Super_L, Qt::KeyboardModifiers()},
       {Qt::Key_Super_R, XKB_KEY_Super_R, Qt::KeyboardModifiers()},
       {Qt::Key_Menu, XKB_KEY_Menu, Qt::KeyboardModifiers()},
       {Qt::Key_Hyper_L, XKB_KEY_Hyper_L, Qt::KeyboardModifiers()},
       {Qt::Key_Hyper_R, XKB_KEY_Hyper_R, Qt::KeyboardModifiers()},
       {Qt::Key_Help, XKB_KEY_Help, Qt::KeyboardModifiers()},
       {Qt::Key_Space, XKB_KEY_KP_Space, Qt::KeypadModifier},
       {Qt::Key_Tab, XKB_KEY_KP_Tab, Qt::KeypadModifier},
       {Qt::Key_Enter, XKB_KEY_KP_Enter, Qt::KeypadModifier},
       {Qt::Key_Home, XKB_KEY_KP_Home, Qt::KeypadModifier},
       {Qt::Key_Left, XKB_KEY_KP_Left, Qt::KeypadModifier},
       {Qt::Key_Up, XKB_KEY_KP_Up, Qt::KeypadModifier},
       {Qt::Key_Right, XKB_KEY_KP_Right, Qt::KeypadModifier},
       {Qt::Key_Down, XKB_KEY_KP_Down, Qt::KeypadModifier},
       {Qt::Key_PageUp, XKB_KEY_KP_Prior, Qt::KeypadModifier},
       {Qt::Key_PageDown, XKB_KEY_KP_Next, Qt::KeypadModifier},
       {Qt::Key_End, XKB_KEY_KP_End, Qt::KeypadModifier},
       {Qt::Key_Clear, XKB_KEY_KP_Begin, Qt::KeypadModifier},
       {Qt::Key_Insert, XKB_KEY_KP_Insert, Qt::KeypadModifier},
       {Qt::Key_Delete, XKB_KEY_KP_Delete, Qt::KeypadModifier},
       {Qt::Key_Equal, XKB_KEY_KP_Equal, Qt::KeypadModifier},
       {Qt::Key_Asterisk, XKB_KEY_KP_Multiply, Qt::KeypadModifier},
       {Qt::Key_Plus, XKB_KEY_KP_Add, Qt::KeypadModifier},
       {Qt::Key_Comma, XKB_KEY_KP_Separator, Qt::KeypadModifier},
       {Qt::Key_Minus, XKB_KEY_KP_Subtract, Qt::KeypadModifier},
       {Qt::Key_Period, XKB_KEY_KP_Decimal, Qt::KeypadModifier},
       {Qt::Key_Slash, XKB_KEY_KP_Divide, Qt::KeypadModifier},
       {Qt::Key_Back, XKB_KEY_XF86Back, Qt::KeyboardModifiers()},
       {Qt::Key_Forward, XKB_KEY_XF86Forward, Qt::KeyboardModifiers()},
       {Qt::Key_Stop, XKB_KEY_XF86Stop, Qt::KeyboardModifiers()},
       {Qt::Key_Refresh, XKB_KEY_XF86Refresh, Qt::KeyboardModifiers()},
       {Qt::Key_Favorites, XKB_KEY_XF86Favorites, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchMedia, XKB_KEY_XF86AudioMedia, Qt::KeyboardModifiers()},
       {Qt::Key_OpenUrl, XKB_KEY_XF86OpenURL, Qt::KeyboardModifiers()},
       {Qt::Key_HomePage, XKB_KEY_XF86HomePage, Qt::KeyboardModifiers()},
       {Qt::Key_Search, XKB_KEY_XF86Search, Qt::KeyboardModifiers()},
       {Qt::Key_VolumeDown, XKB_KEY_XF86AudioLowerVolume, Qt::KeyboardModifiers()},
       {Qt::Key_VolumeMute, XKB_KEY_XF86AudioMute, Qt::KeyboardModifiers()},
       {Qt::Key_VolumeUp, XKB_KEY_XF86AudioRaiseVolume, Qt::KeyboardModifiers()},
       {Qt::Key_MediaPlay, XKB_KEY_XF86AudioPlay, Qt::KeyboardModifiers()},
       {Qt::Key_MediaPause, XKB_KEY_XF86AudioPause, Qt::KeyboardModifiers()},
       {Qt::Key_MediaStop, XKB_KEY_XF86AudioStop, Qt::KeyboardModifiers()},
       {Qt::Key_MediaPrevious, XKB_KEY_XF86AudioPrev, Qt::KeyboardModifiers()},
       {Qt::Key_MediaNext, XKB_KEY_XF86AudioNext, Qt::KeyboardModifiers()},
       {Qt::Key_MediaRecord, XKB_KEY_XF86AudioRecord, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchMail, XKB_KEY_XF86Mail, Qt::KeyboardModifiers()},
       {Qt::Key_Launch0, XKB_KEY_XF86MyComputer, Qt::KeyboardModifiers()},
       {Qt::Key_Launch1, XKB_KEY_XF86Calculator, Qt::KeyboardModifiers()},
       {Qt::Key_Memo, XKB_KEY_XF86Memo, Qt::KeyboardModifiers()},
       {Qt::Key_ToDoList, XKB_KEY_XF86ToDoList, Qt::KeyboardModifiers()},
       {Qt::Key_Calendar, XKB_KEY_XF86Calendar, Qt::KeyboardModifiers()},
       {Qt::Key_PowerDown, XKB_KEY_XF86PowerDown, Qt::KeyboardModifiers()},
       {Qt::Key_ContrastAdjust, XKB_KEY_XF86ContrastAdjust, Qt::KeyboardModifiers()},
       {Qt::Key_Standby, XKB_KEY_XF86Standby, Qt::KeyboardModifiers()},
       {Qt::Key_MonBrightnessUp, XKB_KEY_XF86MonBrightnessUp, Qt::KeyboardModifiers()},
       {Qt::Key_MonBrightnessDown, XKB_KEY_XF86MonBrightnessDown, Qt::KeyboardModifiers()},
       {Qt::Key_KeyboardLightOnOff, XKB_KEY_XF86KbdLightOnOff, Qt::KeyboardModifiers()},
       {Qt::Key_KeyboardBrightnessUp, XKB_KEY_XF86KbdBrightnessUp, Qt::KeyboardModifiers()},
       {Qt::Key_KeyboardBrightnessDown, XKB_KEY_XF86KbdBrightnessDown, Qt::KeyboardModifiers()},
       {Qt::Key_PowerOff, XKB_KEY_XF86PowerOff, Qt::KeyboardModifiers()},
       {Qt::Key_WakeUp, XKB_KEY_XF86WakeUp, Qt::KeyboardModifiers()},
       {Qt::Key_Eject, XKB_KEY_XF86Eject, Qt::KeyboardModifiers()},
       {Qt::Key_ScreenSaver, XKB_KEY_XF86ScreenSaver, Qt::KeyboardModifiers()},
       {Qt::Key_WWW, XKB_KEY_XF86WWW, Qt::KeyboardModifiers()},
       {Qt::Key_Sleep, XKB_KEY_XF86Sleep, Qt::KeyboardModifiers()},
       {Qt::Key_LightBulb, XKB_KEY_XF86LightBulb, Qt::KeyboardModifiers()},
       {Qt::Key_Shop, XKB_KEY_XF86Shop, Qt::KeyboardModifiers()},
       {Qt::Key_History, XKB_KEY_XF86History, Qt::KeyboardModifiers()},
       {Qt::Key_AddFavorite, XKB_KEY_XF86AddFavorite, Qt::KeyboardModifiers()},
       {Qt::Key_HotLinks, XKB_KEY_XF86HotLinks, Qt::KeyboardModifiers()},
       {Qt::Key_BrightnessAdjust, XKB_KEY_XF86BrightnessAdjust, Qt::KeyboardModifiers()},
       {Qt::Key_Finance, XKB_KEY_XF86Finance, Qt::KeyboardModifiers()},
       {Qt::Key_Community, XKB_KEY_XF86Community, Qt::KeyboardModifiers()},
       {Qt::Key_AudioRewind, XKB_KEY_XF86AudioRewind, Qt::KeyboardModifiers()},
       {Qt::Key_BackForward, XKB_KEY_XF86BackForward, Qt::KeyboardModifiers()},
       {Qt::Key_ApplicationLeft, XKB_KEY_XF86ApplicationLeft, Qt::KeyboardModifiers()},
       {Qt::Key_ApplicationRight, XKB_KEY_XF86ApplicationRight, Qt::KeyboardModifiers()},
       {Qt::Key_Book, XKB_KEY_XF86Book, Qt::KeyboardModifiers()},
       {Qt::Key_CD, XKB_KEY_XF86CD, Qt::KeyboardModifiers()},
       {Qt::Key_Calculator, XKB_KEY_XF86Calculater, Qt::KeyboardModifiers()},
       {Qt::Key_Clear, XKB_KEY_XF86Clear, Qt::KeyboardModifiers()},
       {Qt::Key_ClearGrab, XKB_KEY_XF86ClearGrab, Qt::KeyboardModifiers()},
       {Qt::Key_Close, XKB_KEY_XF86Close, Qt::KeyboardModifiers()},
       {Qt::Key_Copy, XKB_KEY_XF86Copy, Qt::KeyboardModifiers()},
       {Qt::Key_Cut, XKB_KEY_XF86Cut, Qt::KeyboardModifiers()},
       {Qt::Key_Display, XKB_KEY_XF86Display, Qt::KeyboardModifiers()},
       {Qt::Key_DOS, XKB_KEY_XF86DOS, Qt::KeyboardModifiers()},
       {Qt::Key_Documents, XKB_KEY_XF86Documents, Qt::KeyboardModifiers()},
       {Qt::Key_Excel, XKB_KEY_XF86Excel, Qt::KeyboardModifiers()},
       {Qt::Key_Explorer, XKB_KEY_XF86Explorer, Qt::KeyboardModifiers()},
       {Qt::Key_Game, XKB_KEY_XF86Game, Qt::KeyboardModifiers()},
       {Qt::Key_Go, XKB_KEY_XF86Go, Qt::KeyboardModifiers()},
       {Qt::Key_iTouch, XKB_KEY_XF86iTouch, Qt::KeyboardModifiers()},
       {Qt::Key_LogOff, XKB_KEY_XF86LogOff, Qt::KeyboardModifiers()},
       {Qt::Key_Market, XKB_KEY_XF86Market, Qt::KeyboardModifiers()},
       {Qt::Key_Meeting, XKB_KEY_XF86Meeting, Qt::KeyboardModifiers()},
       {Qt::Key_MenuKB, XKB_KEY_XF86MenuKB, Qt::KeyboardModifiers()},
       {Qt::Key_MenuPB, XKB_KEY_XF86MenuPB, Qt::KeyboardModifiers()},
       {Qt::Key_MySites, XKB_KEY_XF86MySites, Qt::KeyboardModifiers()},
       {Qt::Key_News, XKB_KEY_XF86News, Qt::KeyboardModifiers()},
       {Qt::Key_OfficeHome, XKB_KEY_XF86OfficeHome, Qt::KeyboardModifiers()},
       {Qt::Key_Option, XKB_KEY_XF86Option, Qt::KeyboardModifiers()},
       {Qt::Key_Paste, XKB_KEY_XF86Paste, Qt::KeyboardModifiers()},
       {Qt::Key_Phone, XKB_KEY_XF86Phone, Qt::KeyboardModifiers()},
       {Qt::Key_Reply, XKB_KEY_XF86Reply, Qt::KeyboardModifiers()},
       {Qt::Key_Reload, XKB_KEY_XF86Reload, Qt::KeyboardModifiers()},
       {Qt::Key_RotateWindows, XKB_KEY_XF86RotateWindows, Qt::KeyboardModifiers()},
       {Qt::Key_RotationPB, XKB_KEY_XF86RotationPB, Qt::KeyboardModifiers()},
       {Qt::Key_RotationKB, XKB_KEY_XF86RotationKB, Qt::KeyboardModifiers()},
       {Qt::Key_Save, XKB_KEY_XF86Save, Qt::KeyboardModifiers()},
       {Qt::Key_Send, XKB_KEY_XF86Send, Qt::KeyboardModifiers()},
       {Qt::Key_Spell, XKB_KEY_XF86Spell, Qt::KeyboardModifiers()},
       {Qt::Key_SplitScreen, XKB_KEY_XF86SplitScreen, Qt::KeyboardModifiers()},
       {Qt::Key_Support, XKB_KEY_XF86Support, Qt::KeyboardModifiers()},
       {Qt::Key_TaskPane, XKB_KEY_XF86TaskPane, Qt::KeyboardModifiers()},
       {Qt::Key_Terminal, XKB_KEY_XF86Terminal, Qt::KeyboardModifiers()},
       {Qt::Key_Tools, XKB_KEY_XF86Tools, Qt::KeyboardModifiers()},
       {Qt::Key_Travel, XKB_KEY_XF86Travel, Qt::KeyboardModifiers()},
       {Qt::Key_Video, XKB_KEY_XF86Video, Qt::KeyboardModifiers()},
       {Qt::Key_Word, XKB_KEY_XF86Word, Qt::KeyboardModifiers()},
       {Qt::Key_Xfer, XKB_KEY_XF86Xfer, Qt::KeyboardModifiers()},
       {Qt::Key_ZoomIn, XKB_KEY_XF86ZoomIn, Qt::KeyboardModifiers()},
       {Qt::Key_ZoomOut, XKB_KEY_XF86ZoomOut, Qt::KeyboardModifiers()},
       {Qt::Key_Away, XKB_KEY_XF86Away, Qt::KeyboardModifiers()},
       {Qt::Key_Messenger, XKB_KEY_XF86Messenger, Qt::KeyboardModifiers()},
       {Qt::Key_WebCam, XKB_KEY_XF86WebCam, Qt::KeyboardModifiers()},
       {Qt::Key_MailForward, XKB_KEY_XF86MailForward, Qt::KeyboardModifiers()},
       {Qt::Key_Pictures, XKB_KEY_XF86Pictures, Qt::KeyboardModifiers()},
       {Qt::Key_Music, XKB_KEY_XF86Music, Qt::KeyboardModifiers()},
       {Qt::Key_Battery, XKB_KEY_XF86Battery, Qt::KeyboardModifiers()},
       {Qt::Key_Bluetooth, XKB_KEY_XF86Bluetooth, Qt::KeyboardModifiers()},
       {Qt::Key_WLAN, XKB_KEY_XF86WLAN, Qt::KeyboardModifiers()},
       {Qt::Key_UWB, XKB_KEY_XF86UWB, Qt::KeyboardModifiers()},
       {Qt::Key_AudioForward, XKB_KEY_XF86AudioForward, Qt::KeyboardModifiers()},
       {Qt::Key_AudioRepeat, XKB_KEY_XF86AudioRepeat, Qt::KeyboardModifiers()},
       {Qt::Key_AudioRandomPlay, XKB_KEY_XF86AudioRandomPlay, Qt::KeyboardModifiers()},
       {Qt::Key_Subtitle, XKB_KEY_XF86Subtitle, Qt::KeyboardModifiers()},
       {Qt::Key_AudioCycleTrack, XKB_KEY_XF86AudioCycleTrack, Qt::KeyboardModifiers()},
       {Qt::Key_Time, XKB_KEY_XF86Time, Qt::KeyboardModifiers()},
       {Qt::Key_Select, XKB_KEY_XF86Select, Qt::KeyboardModifiers()},
       {Qt::Key_View, XKB_KEY_XF86View, Qt::KeyboardModifiers()},
       {Qt::Key_TopMenu, XKB_KEY_XF86TopMenu, Qt::KeyboardModifiers()},
       {Qt::Key_Bluetooth, XKB_KEY_XF86Bluetooth, Qt::KeyboardModifiers()},
       {Qt::Key_Suspend, XKB_KEY_XF86Suspend, Qt::KeyboardModifiers()},
       {Qt::Key_Hibernate, XKB_KEY_XF86Hibernate, Qt::KeyboardModifiers()},
       {Qt::Key_TouchpadToggle, XKB_KEY_XF86TouchpadToggle, Qt::KeyboardModifiers()},
       {Qt::Key_TouchpadOn, XKB_KEY_XF86TouchpadOn, Qt::KeyboardModifiers()},
       {Qt::Key_TouchpadOff, XKB_KEY_XF86TouchpadOff, Qt::KeyboardModifiers()},
       {Qt::Key_MicMute, XKB_KEY_XF86AudioMicMute, Qt::KeyboardModifiers()},
       {Qt::Key_Launch2, XKB_KEY_XF86Launch0, Qt::KeyboardModifiers()},
       {Qt::Key_Launch3, XKB_KEY_XF86Launch1, Qt::KeyboardModifiers()},
       {Qt::Key_Launch4, XKB_KEY_XF86Launch2, Qt::KeyboardModifiers()},
       {Qt::Key_Launch5, XKB_KEY_XF86Launch3, Qt::KeyboardModifiers()},
       {Qt::Key_Launch6, XKB_KEY_XF86Launch4, Qt::KeyboardModifiers()},
       {Qt::Key_Launch7, XKB_KEY_XF86Launch5, Qt::KeyboardModifiers()},
       {Qt::Key_Launch8, XKB_KEY_XF86Launch6, Qt::KeyboardModifiers()},
       {Qt::Key_Launch9, XKB_KEY_XF86Launch7, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchA, XKB_KEY_XF86Launch8, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchB, XKB_KEY_XF86Launch9, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchC, XKB_KEY_XF86LaunchA, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchD, XKB_KEY_XF86LaunchB, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchE, XKB_KEY_XF86LaunchC, Qt::KeyboardModifiers()},
       {Qt::Key_LaunchF, XKB_KEY_XF86LaunchD, Qt::KeyboardModifiers()},

       /*
        * Latin 1
        * (ISO/IEC 8859-1 = Unicode U+0020..U+00FF)
        * Byte 3 = 0
        */
       {Qt::Key_Exclam, XKB_KEY_exclam, Qt::KeyboardModifiers()},
       {Qt::Key_QuoteDbl, XKB_KEY_quotedbl, Qt::KeyboardModifiers()},
       {Qt::Key_NumberSign, XKB_KEY_numbersign, Qt::KeyboardModifiers()},
       {Qt::Key_Dollar, XKB_KEY_dollar, Qt::KeyboardModifiers()},
       {Qt::Key_Percent, XKB_KEY_percent, Qt::KeyboardModifiers()},
       {Qt::Key_Ampersand, XKB_KEY_ampersand, Qt::KeyboardModifiers()},
       {Qt::Key_Apostrophe, XKB_KEY_apostrophe, Qt::KeyboardModifiers()},
       {Qt::Key_ParenLeft, XKB_KEY_parenleft, Qt::KeyboardModifiers()},
       {Qt::Key_ParenRight, XKB_KEY_parenright, Qt::KeyboardModifiers()},
       {Qt::Key_Asterisk, XKB_KEY_asterisk, Qt::KeyboardModifiers()},
       {Qt::Key_Plus, XKB_KEY_plus, Qt::KeyboardModifiers()},
       {Qt::Key_Comma, XKB_KEY_comma, Qt::KeyboardModifiers()},
       {Qt::Key_Minus, XKB_KEY_minus, Qt::KeyboardModifiers()},
       {Qt::Key_Period, XKB_KEY_period, Qt::KeyboardModifiers()},
       {Qt::Key_Slash, XKB_KEY_slash, Qt::KeyboardModifiers()},
       {Qt::Key_0, XKB_KEY_0, Qt::KeyboardModifiers()},
       {Qt::Key_1, XKB_KEY_1, Qt::KeyboardModifiers()},
       {Qt::Key_2, XKB_KEY_2, Qt::KeyboardModifiers()},
       {Qt::Key_3, XKB_KEY_3, Qt::KeyboardModifiers()},
       {Qt::Key_4, XKB_KEY_4, Qt::KeyboardModifiers()},
       {Qt::Key_5, XKB_KEY_5, Qt::KeyboardModifiers()},
       {Qt::Key_6, XKB_KEY_6, Qt::KeyboardModifiers()},
       {Qt::Key_7, XKB_KEY_7, Qt::KeyboardModifiers()},
       {Qt::Key_8, XKB_KEY_8, Qt::KeyboardModifiers()},
       {Qt::Key_9, XKB_KEY_9, Qt::KeyboardModifiers()},
       {Qt::Key_Colon, XKB_KEY_colon, Qt::KeyboardModifiers()},
       {Qt::Key_Semicolon, XKB_KEY_semicolon, Qt::KeyboardModifiers()},
       {Qt::Key_Less, XKB_KEY_less, Qt::KeyboardModifiers()},
       {Qt::Key_Equal, XKB_KEY_equal, Qt::KeyboardModifiers()},
       {Qt::Key_Greater, XKB_KEY_greater, Qt::KeyboardModifiers()},
       {Qt::Key_Question, XKB_KEY_question, Qt::KeyboardModifiers()},
       {Qt::Key_At, XKB_KEY_at, Qt::KeyboardModifiers()},
       {Qt::Key_A, XKB_KEY_A, Qt::ShiftModifier},
       {Qt::Key_B, XKB_KEY_B, Qt::ShiftModifier},
       {Qt::Key_C, XKB_KEY_C, Qt::ShiftModifier},
       {Qt::Key_D, XKB_KEY_D, Qt::ShiftModifier},
       {Qt::Key_E, XKB_KEY_E, Qt::ShiftModifier},
       {Qt::Key_F, XKB_KEY_F, Qt::ShiftModifier},
       {Qt::Key_G, XKB_KEY_G, Qt::ShiftModifier},
       {Qt::Key_H, XKB_KEY_H, Qt::ShiftModifier},
       {Qt::Key_I, XKB_KEY_I, Qt::ShiftModifier},
       {Qt::Key_J, XKB_KEY_J, Qt::ShiftModifier},
       {Qt::Key_K, XKB_KEY_K, Qt::ShiftModifier},
       {Qt::Key_L, XKB_KEY_L, Qt::ShiftModifier},
       {Qt::Key_M, XKB_KEY_M, Qt::ShiftModifier},
       {Qt::Key_N, XKB_KEY_N, Qt::ShiftModifier},
       {Qt::Key_O, XKB_KEY_O, Qt::ShiftModifier},
       {Qt::Key_P, XKB_KEY_P, Qt::ShiftModifier},
       {Qt::Key_Q, XKB_KEY_Q, Qt::ShiftModifier},
       {Qt::Key_R, XKB_KEY_R, Qt::ShiftModifier},
       {Qt::Key_S, XKB_KEY_S, Qt::ShiftModifier},
       {Qt::Key_T, XKB_KEY_T, Qt::ShiftModifier},
       {Qt::Key_U, XKB_KEY_U, Qt::ShiftModifier},
       {Qt::Key_V, XKB_KEY_V, Qt::ShiftModifier},
       {Qt::Key_W, XKB_KEY_W, Qt::ShiftModifier},
       {Qt::Key_X, XKB_KEY_X, Qt::ShiftModifier},
       {Qt::Key_Y, XKB_KEY_Y, Qt::ShiftModifier},
       {Qt::Key_Z, XKB_KEY_Z, Qt::ShiftModifier},
       {Qt::Key_BracketLeft, XKB_KEY_bracketleft, Qt::KeyboardModifiers()},
       {Qt::Key_Backslash, XKB_KEY_backslash, Qt::KeyboardModifiers()},
       {Qt::Key_BracketRight, XKB_KEY_bracketright, Qt::KeyboardModifiers()},
       {Qt::Key_AsciiCircum, XKB_KEY_asciicircum, Qt::KeyboardModifiers()},
       {Qt::Key_Underscore, XKB_KEY_underscore, Qt::KeyboardModifiers()},
       {Qt::Key_QuoteLeft, XKB_KEY_quoteleft, Qt::KeyboardModifiers()},
       {Qt::Key_A, XKB_KEY_a, Qt::KeyboardModifiers()},
       {Qt::Key_B, XKB_KEY_b, Qt::KeyboardModifiers()},
       {Qt::Key_C, XKB_KEY_c, Qt::KeyboardModifiers()},
       {Qt::Key_D, XKB_KEY_d, Qt::KeyboardModifiers()},
       {Qt::Key_E, XKB_KEY_e, Qt::KeyboardModifiers()},
       {Qt::Key_F, XKB_KEY_f, Qt::KeyboardModifiers()},
       {Qt::Key_G, XKB_KEY_g, Qt::KeyboardModifiers()},
       {Qt::Key_H, XKB_KEY_h, Qt::KeyboardModifiers()},
       {Qt::Key_I, XKB_KEY_i, Qt::KeyboardModifiers()},
       {Qt::Key_J, XKB_KEY_j, Qt::KeyboardModifiers()},
       {Qt::Key_K, XKB_KEY_k, Qt::KeyboardModifiers()},
       {Qt::Key_L, XKB_KEY_l, Qt::KeyboardModifiers()},
       {Qt::Key_M, XKB_KEY_m, Qt::KeyboardModifiers()},
       {Qt::Key_N, XKB_KEY_n, Qt::KeyboardModifiers()},
       {Qt::Key_O, XKB_KEY_o, Qt::KeyboardModifiers()},
       {Qt::Key_P, XKB_KEY_p, Qt::KeyboardModifiers()},
       {Qt::Key_Q, XKB_KEY_q, Qt::KeyboardModifiers()},
       {Qt::Key_R, XKB_KEY_r, Qt::KeyboardModifiers()},
       {Qt::Key_S, XKB_KEY_s, Qt::KeyboardModifiers()},
       {Qt::Key_T, XKB_KEY_t, Qt::KeyboardModifiers()},
       {Qt::Key_U, XKB_KEY_u, Qt::KeyboardModifiers()},
       {Qt::Key_V, XKB_KEY_v, Qt::KeyboardModifiers()},
       {Qt::Key_W, XKB_KEY_w, Qt::KeyboardModifiers()},
       {Qt::Key_X, XKB_KEY_x, Qt::KeyboardModifiers()},
       {Qt::Key_Y, XKB_KEY_y, Qt::KeyboardModifiers()},
       {Qt::Key_Z, XKB_KEY_z, Qt::KeyboardModifiers()},
       {Qt::Key_BraceLeft, XKB_KEY_braceleft, Qt::KeyboardModifiers()},
       {Qt::Key_Bar, XKB_KEY_bar, Qt::KeyboardModifiers()},
       {Qt::Key_BraceRight, XKB_KEY_braceright, Qt::KeyboardModifiers()},
       {Qt::Key_AsciiTilde, XKB_KEY_asciitilde, Qt::KeyboardModifiers()},

       {Qt::Key_nobreakspace, XKB_KEY_nobreakspace, Qt::KeyboardModifiers()},
       {Qt::Key_exclamdown, XKB_KEY_exclamdown, Qt::KeyboardModifiers()},
       {Qt::Key_cent, XKB_KEY_cent, Qt::KeyboardModifiers()},
       {Qt::Key_sterling, XKB_KEY_sterling, Qt::KeyboardModifiers()},
       {Qt::Key_currency, XKB_KEY_currency, Qt::KeyboardModifiers()},
       {Qt::Key_yen, XKB_KEY_yen, Qt::KeyboardModifiers()},
       {Qt::Key_brokenbar, XKB_KEY_brokenbar, Qt::KeyboardModifiers()},
       {Qt::Key_section, XKB_KEY_section, Qt::KeyboardModifiers()},
       {Qt::Key_diaeresis, XKB_KEY_diaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_copyright, XKB_KEY_copyright, Qt::KeyboardModifiers()},
       {Qt::Key_ordfeminine, XKB_KEY_ordfeminine, Qt::KeyboardModifiers()},
       {Qt::Key_guillemotleft, XKB_KEY_guillemotleft, Qt::KeyboardModifiers()},
       {Qt::Key_notsign, XKB_KEY_notsign, Qt::KeyboardModifiers()},
       {Qt::Key_hyphen, XKB_KEY_hyphen, Qt::KeyboardModifiers()},
       {Qt::Key_registered, XKB_KEY_registered, Qt::KeyboardModifiers()},
       {Qt::Key_macron, XKB_KEY_macron, Qt::KeyboardModifiers()},
       {Qt::Key_degree, XKB_KEY_degree, Qt::KeyboardModifiers()},
       {Qt::Key_plusminus, XKB_KEY_plusminus, Qt::KeyboardModifiers()},
       {Qt::Key_twosuperior, XKB_KEY_twosuperior, Qt::KeyboardModifiers()},
       {Qt::Key_threesuperior, XKB_KEY_threesuperior, Qt::KeyboardModifiers()},
       {Qt::Key_acute, XKB_KEY_acute, Qt::KeyboardModifiers()},
       {Qt::Key_mu, XKB_KEY_mu, Qt::KeyboardModifiers()},
       {Qt::Key_paragraph, XKB_KEY_paragraph, Qt::KeyboardModifiers()},
       {Qt::Key_periodcentered, XKB_KEY_periodcentered, Qt::KeyboardModifiers()},
       {Qt::Key_cedilla, XKB_KEY_cedilla, Qt::KeyboardModifiers()},
       {Qt::Key_onesuperior, XKB_KEY_onesuperior, Qt::KeyboardModifiers()},
       {Qt::Key_masculine, XKB_KEY_masculine, Qt::KeyboardModifiers()},
       {Qt::Key_guillemotright, XKB_KEY_guillemotright, Qt::KeyboardModifiers()},
       {Qt::Key_onequarter, XKB_KEY_onequarter, Qt::KeyboardModifiers()},
       {Qt::Key_onehalf, XKB_KEY_onehalf, Qt::KeyboardModifiers()},
       {Qt::Key_threequarters, XKB_KEY_threequarters, Qt::KeyboardModifiers()},
       {Qt::Key_questiondown, XKB_KEY_questiondown, Qt::KeyboardModifiers()},
       {Qt::Key_Agrave, XKB_KEY_Agrave, Qt::ShiftModifier},
       {Qt::Key_Aacute, XKB_KEY_Aacute, Qt::ShiftModifier},
       {Qt::Key_Acircumflex, XKB_KEY_Acircumflex, Qt::ShiftModifier},
       {Qt::Key_Atilde, XKB_KEY_Atilde, Qt::ShiftModifier},
       {Qt::Key_Adiaeresis, XKB_KEY_Adiaeresis, Qt::ShiftModifier},
       {Qt::Key_Aring, XKB_KEY_Aring, Qt::ShiftModifier},
       {Qt::Key_AE, XKB_KEY_AE, Qt::ShiftModifier},
       {Qt::Key_Ccedilla, XKB_KEY_Ccedilla, Qt::ShiftModifier},
       {Qt::Key_Egrave, XKB_KEY_Egrave, Qt::ShiftModifier},
       {Qt::Key_Eacute, XKB_KEY_Eacute, Qt::ShiftModifier},
       {Qt::Key_Ecircumflex, XKB_KEY_Ecircumflex, Qt::ShiftModifier},
       {Qt::Key_Ediaeresis, XKB_KEY_Ediaeresis, Qt::ShiftModifier},
       {Qt::Key_Igrave, XKB_KEY_Igrave, Qt::ShiftModifier},
       {Qt::Key_Iacute, XKB_KEY_Iacute, Qt::ShiftModifier},
       {Qt::Key_Icircumflex, XKB_KEY_Icircumflex, Qt::ShiftModifier},
       {Qt::Key_Idiaeresis, XKB_KEY_Idiaeresis, Qt::ShiftModifier},
       {Qt::Key_ETH, XKB_KEY_ETH, Qt::ShiftModifier},
       {Qt::Key_Ntilde, XKB_KEY_Ntilde, Qt::ShiftModifier},
       {Qt::Key_Ograve, XKB_KEY_Ograve, Qt::ShiftModifier},
       {Qt::Key_Oacute, XKB_KEY_Oacute, Qt::ShiftModifier},
       {Qt::Key_Ocircumflex, XKB_KEY_Ocircumflex, Qt::ShiftModifier},
       {Qt::Key_Otilde, XKB_KEY_Otilde, Qt::ShiftModifier},
       {Qt::Key_Odiaeresis, XKB_KEY_Odiaeresis, Qt::ShiftModifier},
       {Qt::Key_multiply, XKB_KEY_multiply, Qt::ShiftModifier},
       {Qt::Key_Ooblique, XKB_KEY_Ooblique, Qt::ShiftModifier},
       {Qt::Key_Ugrave, XKB_KEY_Ugrave, Qt::ShiftModifier},
       {Qt::Key_Uacute, XKB_KEY_Uacute, Qt::ShiftModifier},
       {Qt::Key_Ucircumflex, XKB_KEY_Ucircumflex, Qt::ShiftModifier},
       {Qt::Key_Udiaeresis, XKB_KEY_Udiaeresis, Qt::ShiftModifier},
       {Qt::Key_Yacute, XKB_KEY_Yacute, Qt::ShiftModifier},
       {Qt::Key_THORN, XKB_KEY_THORN, Qt::ShiftModifier},
       {Qt::Key_ssharp, XKB_KEY_ssharp, Qt::KeyboardModifiers()},
       {Qt::Key_Agrave, XKB_KEY_agrave, Qt::KeyboardModifiers()},
       {Qt::Key_Aacute, XKB_KEY_aacute, Qt::KeyboardModifiers()},
       {Qt::Key_Acircumflex, XKB_KEY_acircumflex, Qt::KeyboardModifiers()},
       {Qt::Key_Atilde, XKB_KEY_atilde, Qt::KeyboardModifiers()},
       {Qt::Key_Adiaeresis, XKB_KEY_adiaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_Aring, XKB_KEY_aring, Qt::KeyboardModifiers()},
       {Qt::Key_AE, XKB_KEY_ae, Qt::KeyboardModifiers()},
       {Qt::Key_Ccedilla, XKB_KEY_ccedilla, Qt::KeyboardModifiers()},
       {Qt::Key_Egrave, XKB_KEY_egrave, Qt::KeyboardModifiers()},
       {Qt::Key_Eacute, XKB_KEY_eacute, Qt::KeyboardModifiers()},
       {Qt::Key_Ecircumflex, XKB_KEY_ecircumflex, Qt::KeyboardModifiers()},
       {Qt::Key_Ediaeresis, XKB_KEY_ediaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_Igrave, XKB_KEY_igrave, Qt::KeyboardModifiers()},
       {Qt::Key_Iacute, XKB_KEY_iacute, Qt::KeyboardModifiers()},
       {Qt::Key_Icircumflex, XKB_KEY_icircumflex, Qt::KeyboardModifiers()},
       {Qt::Key_Idiaeresis, XKB_KEY_idiaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_ETH, XKB_KEY_eth, Qt::KeyboardModifiers()},
       {Qt::Key_Ntilde, XKB_KEY_ntilde, Qt::KeyboardModifiers()},
       {Qt::Key_Ograve, XKB_KEY_ograve, Qt::KeyboardModifiers()},
       {Qt::Key_Oacute, XKB_KEY_oacute, Qt::KeyboardModifiers()},
       {Qt::Key_Ocircumflex, XKB_KEY_ocircumflex, Qt::KeyboardModifiers()},
       {Qt::Key_Otilde, XKB_KEY_otilde, Qt::KeyboardModifiers()},
       {Qt::Key_Odiaeresis, XKB_KEY_odiaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_division, XKB_KEY_division, Qt::KeyboardModifiers()},
       {Qt::Key_Ooblique, XKB_KEY_ooblique, Qt::KeyboardModifiers()},
       {Qt::Key_Ugrave, XKB_KEY_ugrave, Qt::KeyboardModifiers()},
       {Qt::Key_Uacute, XKB_KEY_uacute, Qt::KeyboardModifiers()},
       {Qt::Key_Ucircumflex, XKB_KEY_ucircumflex, Qt::KeyboardModifiers()},
       {Qt::Key_Udiaeresis, XKB_KEY_udiaeresis, Qt::KeyboardModifiers()},
       {Qt::Key_Yacute, XKB_KEY_yacute, Qt::KeyboardModifiers()},
       {Qt::Key_THORN, XKB_KEY_thorn, Qt::KeyboardModifiers()},
       {Qt::Key_ydiaeresis, XKB_KEY_ydiaeresis, Qt::KeyboardModifiers()},
       /*
        * Numpad
        */
       {Qt::Key_0, XKB_KEY_KP_0, Qt::KeypadModifier},
       {Qt::Key_1, XKB_KEY_KP_1, Qt::KeypadModifier},
       {Qt::Key_2, XKB_KEY_KP_2, Qt::KeypadModifier},
       {Qt::Key_3, XKB_KEY_KP_3, Qt::KeypadModifier},
       {Qt::Key_4, XKB_KEY_KP_4, Qt::KeypadModifier},
       {Qt::Key_5, XKB_KEY_KP_5, Qt::KeypadModifier},
       {Qt::Key_6, XKB_KEY_KP_6, Qt::KeypadModifier},
       {Qt::Key_7, XKB_KEY_KP_7, Qt::KeypadModifier},
       {Qt::Key_8, XKB_KEY_KP_8, Qt::KeypadModifier},
       {Qt::Key_9, XKB_KEY_KP_9, Qt::KeypadModifier}};

void XkbTest::testToQtKey_data()
{
    QTest::addColumn<Qt::Key>("qt");
    QTest::addColumn<xkb_keysym_t>("keySym");
    for (std::size_t i = 0; i < sizeof(g_rgQtToSymX) / sizeof(TransKey); i++) {
        const QByteArray row = QByteArray::number(g_rgQtToSymX[i].keySymX, 16);
        QTest::newRow(row.constData()) << g_rgQtToSymX[i].keySymQt << g_rgQtToSymX[i].keySymX;
    }
}

void XkbTest::testToQtKey()
{
    input::xkb::manager manager(nullptr);
    input::xkb::keyboard xkb_keyboard(manager);
    QFETCH(xkb_keysym_t, keySym);
    QTEST(xkb_keyboard.to_qt_key(keySym), "qt");
}

QTEST_MAIN(XkbTest)
#include "test_xkb.moc"
