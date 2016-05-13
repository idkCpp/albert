// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "hotkeymanager.h"
#include "hotkeymanager_p.h"

#include <QKeySequence>

/** ***************************************************************************/
HotkeyManager::HotkeyManager(QObject *parent) :
    QObject(parent) {
    impl_ = new HotkeyManagerPrivate;
    connect(impl_, &HotkeyManagerPrivate::hotKeyPressed, this, &HotkeyManager::onHotkeyPressed);
    enabled_ = true;
}

/** ***************************************************************************/
HotkeyManager::~HotkeyManager() {
    delete impl_;
}

/** ***************************************************************************/
bool HotkeyManager::registerHotkey(const QString &hk) {
	return registerHotkey(QKeySequence(hk));
}

/** ***************************************************************************/
bool HotkeyManager::registerHotkey(const QKeySequence &hk) {
	if (hk.count() != 1)
		return false;
	return registerHotkey(hk[0]);
}

/** ***************************************************************************/
bool HotkeyManager::registerHotkey(const int hk) {
    if (hotkeys_.contains(hk))
        return true;
    if (impl_->registerNativeHotkey(hk)) {
        hotkeys_.insert(hk);
		return true;
	}
    return false;
}

/** ***************************************************************************/
bool HotkeyManager::unregisterHotkey(const QString &hk) {
    return unregisterHotkey(QKeySequence(hk));
}

/** ***************************************************************************/
bool HotkeyManager::unregisterHotkey(const QKeySequence &hk) {
    if (hk.count() != 1)
        return false;
    unregisterHotkey(hk[0]);
    return true;
}

/** ***************************************************************************/
void HotkeyManager::unregisterHotkey(const int hk) {
    if (!hotkeys_.contains(hk))
        return;
    impl_->unregisterNativeHotkey(hk);
    hotkeys_.remove(hk);
}

/** ***************************************************************************/
void HotkeyManager::onHotkeyPressed() {
    if (enabled_)
		emit hotKeyPressed();
}

