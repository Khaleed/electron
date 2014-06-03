// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/win/notify_icon.h"

#include "atom/browser/ui/win/notify_icon_host.h"
#include "atom/browser/ui/win/menu_2.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

namespace atom {

NotifyIcon::NotifyIcon(NotifyIconHost* host,
                       UINT id,
                       HWND window,
                       UINT message)
    : host_(host),
      icon_id_(id),
      window_(window),
      message_id_(message),
      menu_model_(NULL) {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags = NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  // This can happen if the explorer process isn't running when we try to
  // create the icon for some reason (for example, at startup).
  if (!result)
    LOG(WARNING) << "Unable to create status tray icon.";
}

NotifyIcon::~NotifyIcon() {
  // Remove our icon.
  host_->Remove(this);
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  Shell_NotifyIcon(NIM_DELETE, &icon_data);
}

void NotifyIcon::HandleClickEvent(const gfx::Point& cursor_pos,
                                  bool left_mouse_click) {
  // Pass to the observer if appropriate.
  if (left_mouse_click) {
    NotifyClicked();
    return;
  }

  if (!menu_model_)
    return;

  // Set our window as the foreground window, so the context menu closes when
  // we click away from it.
  if (!SetForegroundWindow(window_))
    return;

  menu_.reset(new Menu2(menu_model_));
  menu_->RunContextMenuAt(cursor_pos);
}

void NotifyIcon::ResetIcon() {
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  // Delete any previously existing icon.
  Shell_NotifyIcon(NIM_DELETE, &icon_data);
  InitIconData(&icon_data);
  icon_data.uFlags = NIF_MESSAGE;
  icon_data.uCallbackMessage = message_id_;
  icon_data.hIcon = icon_.Get();
  // If we have an image, then set the NIF_ICON flag, which tells
  // Shell_NotifyIcon() to set the image for the status icon it creates.
  if (icon_data.hIcon)
    icon_data.uFlags |= NIF_ICON;
  // Re-add our icon.
  BOOL result = Shell_NotifyIcon(NIM_ADD, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to re-create status tray icon.";
}

void NotifyIcon::SetImage(const gfx::ImageSkia& image) {
  // Create the icon.
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags = NIF_ICON;
  icon_.Set(IconUtil::CreateHICONFromSkBitmap(*image.bitmap()));
  icon_data.hIcon = icon_.Get();
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Error setting status tray icon image";
  else
    host_->UpdateIconVisibilityInBackground(this);
}

void NotifyIcon::SetPressedImage(const gfx::ImageSkia& image) {
  // Ignore pressed images, since the standard on Windows is to not highlight
  // pressed status icons.
}

void NotifyIcon::SetToolTip(const std::string& tool_tip) {
  // Create the icon.
  NOTIFYICONDATA icon_data;
  InitIconData(&icon_data);
  icon_data.uFlags = NIF_TIP;
  wcscpy_s(icon_data.szTip, UTF8ToUTF16(tool_tip).c_str());
  BOOL result = Shell_NotifyIcon(NIM_MODIFY, &icon_data);
  if (!result)
    LOG(WARNING) << "Unable to set tooltip for status tray icon";
}

void NotifyIcon::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_model_ = menu_model;
}

void NotifyIcon::InitIconData(NOTIFYICONDATA* icon_data) {
  if (base::win::GetVersion() >= base::win::VERSION_VISTA) {
    memset(icon_data, 0, sizeof(NOTIFYICONDATA));
    icon_data->cbSize = sizeof(NOTIFYICONDATA);
  } else {
    memset(icon_data, 0, NOTIFYICONDATA_V3_SIZE);
    icon_data->cbSize = NOTIFYICONDATA_V3_SIZE;
  }

  icon_data->hWnd = window_;
  icon_data->uID = icon_id_;
}

}  // namespace atom
