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

#pragma once
#include <QObject>
#include <QLocale>
#include <QPointer>
#include "iextension.h"

namespace mu{
    class Parser;
}

namespace Calculator {

class ConfigWidget;

class Extension final : public QObject, public IExtension
{
    Q_OBJECT
    Q_INTERFACES(IExtension)
    Q_PLUGIN_METADATA(IID ALBERT_EXTENSION_IID FILE "metadata.json")

public:

    Extension();
    ~Extension();

    /*
     * Implementation of extension interface
     */

    QWidget *widget(QWidget *parent = nullptr) override;
    void handleQuery(shared_ptr<Query> query) override;

    /*
     * Extension specific members
     */

    /* const */
    static const QString CFG_SEPS;
    static const bool    CFG_SEPS_DEF;

private:
    QPointer<ConfigWidget> widget_;
    mu::Parser *parser_;
    QLocale loc_;
    QString iconPath_;
};
}
