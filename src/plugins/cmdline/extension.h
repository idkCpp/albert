// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
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
#include <QPointer>
#include "iextension.h"

namespace cmdline {

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
    void setupSession() override;
    void teardownSession() override;
    void handleQuery(shared_ptr<Query> query) override;
    void handleFallbackQuery(shared_ptr<Query>) override;

    /*
     * Extension specific members
     */
    void newExe(QString path, QString params);

private:
    QPointer<ConfigWidget> widget_;
    QStringList executables;
    QString iconPath_;
    QString cmdExe;
    QString cmdExeName;
    QString cmdExeOptions;

    void execute(QString &cmd, QString& line, shared_ptr<Query>, QString &workingDir);
};
}
