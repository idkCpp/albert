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
#include <QSettings>
#include <QTimer>
#include "iextension.h"

namespace SSH {

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
    QStringList triggers() const override {return {"ssh"};}

    /*
     * Extension specific members
     */
    void load();
    void save();

public slots:
    void rebuildIndex(const QStringList &list);
    void buildIndex();

private:
    static QSettings settings_;
    QPointer<ConfigWidget> widget_;
    QString iconPath_;
    QList<QRegExp> availableSshConnections_;
    QStringList files_;
    QTimer *rebuildTimer_;

    void readSshConfigFile(QString& filepath);
};
}
