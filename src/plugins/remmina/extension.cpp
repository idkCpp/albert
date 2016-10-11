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

#include <QDebug>
#include <QFile>
#include <QDirIterator>
#include <QSettings>
#include <QStandardPaths>
#include "extension.h"
#include "configwidget.h"
#include "item.h"
#include "query.h"
#include "standardobjects.h"
#include "xdgiconlookup.h"

/** ***************************************************************************/
Remmina::Extension::Extension() : AbstractExtension("org.albert.extension.remmina") {
    qDebug("[%s] Initialize extension", name_);

    // Checking if remmina is detected
    error_ = QStandardPaths::findExecutable("remmina").isEmpty();
    if (error_)
        qCritical("[%s] Remmina-executable not found!", name_);

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
Remmina::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);
    // Do sth.
    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
QWidget *Remmina::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
    }
    return widget_;
}



/** ***************************************************************************/
void Remmina::Extension::setupSession() {
    // Don't need to read files if we have no exe
    if (error_)
        return;

    // Delete the old ones first
    for (Item* item : availableItems_)
        delete item;
    availableItems_.clear();

    // Iterate through the .remmina dir
    QDir remminaDir = QDir::home();
    remminaDir.cd(".remmina");
    QDirIterator it(remminaDir);
    while (it.hasNext()) {
        it.next();
        if (it.fileName() == "." || it.fileName() == "..")
            continue;

        QFile nextFile(it.filePath());
        if (nextFile.open(QFile::ReadOnly)) {

            char* firstBytes = new char[10];
            nextFile.read(firstBytes, 9);
            firstBytes[9] = 0;
            QString signature(firstBytes);
            delete firstBytes;
            if (signature != "[remmina]") {
                qDebug("[%s] Ignoring file %s", name_, it.fileName().toStdString().c_str());
                continue;
            }

            nextFile.close();
            availableItems_.append(new Item(it.filePath()));

        } else
            qWarning("[%s] Could not read file %s", name_, it.fileName().toStdString().c_str());
    }
}



/** ***************************************************************************/
void Remmina::Extension::handleQuery(Query query) {
    static QString errorItemPath = XdgIconLookup::instance()->themeIconPath("dialog-error", QIcon::themeName());

    if (error_) {
        StandardItem *errorExeNotFound = new StandardItem("erroritem:remmina:exe-not-found");
        errorExeNotFound->setText("The Remmina-executable was not found");
        errorExeNotFound->setIconPath(errorItemPath);
        query.addMatch(std::shared_ptr<StandardItem>(errorExeNotFound));
        return;
    }
    if (availableItems_.size() == 0) {
        StandardItem *errorExeConfigEmpty = new StandardItem("erroritem:remmina:config-empty");
        errorExeConfigEmpty->setText("No config entry found");
        errorExeConfigEmpty->setIconPath(errorItemPath);
        query.addMatch(std::shared_ptr<StandardItem>(errorExeConfigEmpty));
        return;
    }
    QStringList words = query.searchTerm().split(" ");
    words.removeFirst();
    QString searchPrefix;
    bool matchAll = false;
    if (words.isEmpty())
        matchAll = true;
    else
        searchPrefix = words.join(" ").toLower();
    for (Item* item : availableItems_) {
        if (matchAll || item->getName().toLower().startsWith(searchPrefix))
            query.addMatch(std::shared_ptr<AbstractItem>(item->copy()));
    }
}
