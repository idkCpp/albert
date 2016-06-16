// Qt Config-Widgets Automatic Configuration Updater and Util
// Copyright (C) 2016 Martin Buergmann
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

#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include "iextension.h"

#define AUTOCONF_MEMBER AutoConf *configurator;
#define AUTOCONF_SETUP(widget)  configurator = new AutoConf(widget, name_)
//#define AUTOCONF(sender, key, store)    configurator->registerUIItem(sender, key, store)
#define AUTOCONF(sender, key, store, init) configurator->registerUIItem(sender, key, store, init)

class Listener;

class AutoConf : public QObject
{
    Q_OBJECT

public:
    // Con-/Destructors
    AutoConf(QObject *parent, QString group);
    AutoConf(QObject *parent, const char *group);
    AutoConf(QObject *parent, IExtension *ext);
    AutoConf(QObject *parent, IExtension &ext);
    virtual ~AutoConf();

    // QLineEdit
    void registerUIItem(QLineEdit *, const char* key, QString& storage, const char* initialValue = nullptr);
    void registerUIItem(QLineEdit *, QString key, QString& storage, QString initialValue = QStringLiteral(""));

    // QListWidget
    void registerUIItem(QListWidget *, const char* key, QStringList &storage, QStringList &initialValue = emptyStringList);
    void registerUIItem(QListWidget *, QString key, QStringList &storage, QStringList &initialValue = emptyStringList);

    // QCheckBox
    void registerUIItem(QCheckBox *, const char *key, bool &storage, bool initialValue = false);
    void registerUIItem(QCheckBox *, QString key, bool &storage, bool initialValue = false);

    // QSpinBox
    void registerUIItem(QSpinBox *, const char *key, int &storage, int initialValue = 0);
    void registerUIItem(QSpinBox *, QString key, int &storage, int initialValue = 0);
    void registerUIItem(QSpinBox *, const char *key, uint &storage, uint initialValue = 0);
    void registerUIItem(QSpinBox *, QString key, uint &storage, uint initialValue = 0);

    // These void-objects can be used when there is no config variable that holds the value
    QString voidString;
    QStringList voidStringList;
    bool voidBool;
    int voidInt;
    uint voidUInt;

public slots:
    void destroyed(QObject *toDestroy);

private:
    static QStringList emptyStringList;
    QString configGroup;
    QList<Listener*> listeners;
};

