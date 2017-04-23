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

#pragma once
#include <QObject>
#include <QVariant>
#include <QString>
#include <functional>

namespace Introspect {
    bool registerVariable(const QString &key, QVariant &value);
    bool registerSignal(const QString &key, std::function<void(void)> signalFunction);
    void unregister(const QString &key);
    void invoke(const QString &key);
    QVariant* obtain(const QString &key);
}
