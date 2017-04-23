// albert - a simple application launcher for linux
// Copyright (C) 2017 Martin Buergmann
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

#include "introspect.h"
#include <QMap>

namespace Introspect {
static QMap<QString, QVariant*> variants;
static QMap<QString, std::function<void(void)>> signaling;



void unregister(const QString &key) {
    variants.remove(key);
    signaling.remove(key);
}



bool registerVariable(const QString &key, QVariant &value) {
    if (variants.contains(key))
        return false;
    if (signaling.contains(key))
        return false;
    variants.insert(key, &value);
    return true;
}



bool registerSignal(const QString &key, std::function<void ()> signalFunction) {
    if (variants.contains(key))
        return false;
    if (signaling.contains(key))
        return false;
    signaling.insert(key, std::move(signalFunction));
    return true;
}



void invoke(const QString &key) {
    QMap<QString, std::function<void(void)>>::iterator it = signaling.find(key);
    if (it == signaling.end())
        return;
    std::function<void(void)>& toInvoke = it.value();
    toInvoke();
}



QVariant *obtain(const QString &key) {
    QMap<QString, QVariant*>::iterator it = variants.find(key);
    if (it == variants.end())
        return nullptr;
    return it.value();
}

}
