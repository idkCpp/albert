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

#include <QObject>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QSettings>

#define TYPE_LINEEDIT   1
#define TYPE_LISTWIDGET 2
#define TYPE_CHECKBOX   3
#define TYPE_SPINBOX    4

class Listener : public QObject {
    Q_OBJECT

public:

    // These void-variables are used to initialize references that are unused
    // This is required because C++ does not allow uninitialized references
    QString voidString;
    QStringList voidStringList;
    bool voidBool;
    int voidInt;
    uint voidUInt;



    /** ***********************************************************************/
    Listener(QObject *parent, QLineEdit *item, QString &configGroup, QString &key, QString &storage) :
            QObject(parent),
            type_(TYPE_LINEEDIT),
            obj_(item),
            key_(key),
            stringStore_(storage),
            stringListStore_(voidStringList),
            boolStore_(voidBool),
            intStore_(voidInt),
            uintStore_(voidUInt) {
        settings_.beginGroup(configGroup);
        connect(item, SIGNAL(textChanged(QString)), this, SLOT(lineEdit(QString)));
    }



    /** ***********************************************************************/
    Listener(QObject *parent, QListWidget *item, QString &configGroup, QString &key, QStringList &storage) :
            QObject(parent),
            type_(TYPE_LISTWIDGET),
            obj_(item),
            key_(key),
            stringStore_(voidString),
            stringListStore_(storage),
            boolStore_(voidBool),
            intStore_(voidInt),
            uintStore_(voidUInt) {
        settings_.beginGroup(configGroup);
        connect(item, SIGNAL(destroyed()), this, SLOT(listView()));
    }



    /** ***********************************************************************/
    Listener(QObject *parent, QCheckBox *item, QString &configGroup, QString &key, bool &storage) :
            QObject(parent),
            type_(TYPE_CHECKBOX),
            obj_(item),
            key_(key),
            stringStore_(voidString),
            stringListStore_(voidStringList),
            boolStore_(storage),
            intStore_(voidInt),
            uintStore_(voidUInt) {
        settings_.beginGroup(configGroup);
        connect(item, SIGNAL(toggled(bool)), this, SLOT(checkbox(bool)));
    }



    /** ***********************************************************************/
    Listener(QObject *parent, QSpinBox *item, QString& configGroup, QString &key, int &storage) :
            QObject(parent),
            type_(TYPE_SPINBOX),
            obj_(item),
            key_(key),
            stringStore_(voidString),
            stringListStore_(voidStringList),
            boolStore_(voidBool),
            intStore_(storage),
            uintStore_(voidUInt) {
        settings_.beginGroup(configGroup);
        connect(item, SIGNAL(valueChanged(int)), this, SLOT(spinbox(int)));
    }



    /** ***********************************************************************/
    Listener(QObject *parent, QSpinBox *item, QString &configGroup, QString &key, uint &storage) :
            QObject(parent),
            type_(TYPE_SPINBOX),
            obj_(item),
            key_(key),
            stringStore_(voidString),
            stringListStore_(voidStringList),
            boolStore_(voidBool),
            intStore_(voidInt),
            uintStore_(storage) {
        settings_.beginGroup(configGroup);
        connect(item, SIGNAL(valueChanged(int)), this, SLOT(spinbox(int)));
    }



    /** ***********************************************************************/
    virtual ~Listener() {
    }



    /** ***********************************************************************/
    void* getObject() {
        return obj_;
    }

public slots:

    /** ***********************************************************************/
    void lineEdit(QString line) {
        stringStore_ = line;
        settings_.setValue(key_, line);
        settings_.sync();
    }



    /** ***********************************************************************/
    void listView() {
        QListWidget* listWidget = (QListWidget*) obj_;
        stringListStore_.clear();
        for (int i = 0; i < listWidget->count(); i++) {
            stringListStore_.append(listWidget->item(i)->text());
        }
        settings_.setValue(key_, stringListStore_);
        settings_.sync();
    }



    /** ***********************************************************************/
    void checkbox(bool newState) {
        boolStore_ = newState;
        settings_.setValue(key_, newState);
        settings_.sync();
    }



    /** ***********************************************************************/
    void spinbox(int content) {
        intStore_ = content;
        uintStore_ = content;
        settings_.setValue(key_, content);
        settings_.sync();
    }

private:
    int type_;
    void* obj_;
    QSettings settings_;
    QString key_;
    QString &stringStore_;
    QStringList &stringListStore_;
    bool &boolStore_;
    int &intStore_;
    uint &uintStore_;
};
