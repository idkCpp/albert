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

#include "autoconf.h"

#include "listener.hpp"


/** ***********************************************************************/
AutoConf::AutoConf(QObject *parent, QString group) : QObject(parent), configGroup(group) {
}



/** ***********************************************************************/
AutoConf::AutoConf(QObject *parent, const char *group) : QObject(parent), configGroup(group) {
}



/** ***********************************************************************/
AutoConf::AutoConf(QObject *parent, IExtension *ext) : QObject(parent), configGroup(ext->name_) {
}



/** ***********************************************************************/
AutoConf::AutoConf(QObject *parent, IExtension &ext) : QObject(parent), configGroup(ext.name_) {
}



/** ***********************************************************************/
AutoConf::~AutoConf() {
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QLineEdit *item, const char *key, QString &storage, const char *initialValue) {
    item->setText(initialValue);
    QString keyS(key);
    Listener *l = new Listener(this, item, configGroup, keyS, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QLineEdit *item, QString key, QString &storage, QString initialValue) {
    item->setText(initialValue);
    Listener *l = new Listener(this, item, configGroup, key, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QListWidget *item, const char *key, QStringList &storage, QStringList &initialValue) {
    item->addItems(initialValue);
    QString keyS(key);
    Listener *l = new Listener(this, item, configGroup, keyS, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QListWidget *item, QString key, QStringList &storage, QStringList &initialValue) {
    item->addItems(initialValue);
    Listener *l = new Listener(this, item, configGroup, key, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QCheckBox *item, const char *key, bool &storage, bool initialValue) {
    item->setChecked(initialValue);
    QString keyS(key);
    Listener *l = new Listener(this, item, configGroup, keyS, storage);
    connect(item, SIGNAL(toggled(bool)), l, SLOT(checkbox(bool)));
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QCheckBox *item, QString key, bool &storage, bool initialValue) {
    item->setChecked(initialValue);
    Listener *l = new Listener(this, item, configGroup, key, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QSpinBox *item, const char *key, int &storage, int initialValue) {
    item->setValue(initialValue);
    QString keyS(key);
    Listener *l = new Listener(this, item, configGroup, keyS, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QSpinBox *item, QString key, int &storage, int initialValue) {
    item->setValue(initialValue);
    Listener *l = new Listener(this, item, configGroup, key, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QSpinBox *item, const char *key, uint &storage, uint initialValue) {
    item->setValue(initialValue);
    QString keyS(key);
    Listener *l = new Listener(this, item, configGroup, keyS, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::registerUIItem(QSpinBox *item, QString key, uint &storage, uint initialValue) {
    item->setValue(initialValue);
    Listener *l = new Listener(this, item, configGroup, key, storage);
    listeners.append(l);
}



/** ***********************************************************************/
void AutoConf::destroyed(QObject *toDestroy) {
    for (Listener* l : listeners) {
        if (l->getObject() == toDestroy)
            delete l;
    }
}
