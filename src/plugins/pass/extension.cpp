// albert - a simple application launcher for linux
// Copyright (C) 2016 Martin Bürgmann
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
#include "extension.h"
#include "configwidget.h"
#include "item.h"
#include "query.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStandardItem>
#include <functional>

/** ***************************************************************************/
Pass::Extension::Extension() : IExtension("Pass") {
    qDebug("[%s] Initialize extension", name_);

    QString passDirPath = QStandardPaths::locate(QStandardPaths::HomeLocation, ".password-store", QStandardPaths::LocateDirectory);
    if (passDirPath.isEmpty()) {
        qWarning("[%s] Directory .password-store not found!", name_);
        return;
    }

    int passPathLen = passDirPath.length(); // The length we can skip on other paths
    if (!passDirPath.endsWith("/"))         // The path does not end with "/"
        passPathLen++;                      // But we will have to ignore it too

    sim_ = new QStandardItemModel(this);
//    sim_->appendColumn(QList<QStandardItem*>() << new QStandardItem("Founde Passwords"));
    int foundKeys = 0;
    int foundGroups = 0;
    std::function<void(QString,QStandardItem*)> recursion = [&recursion, this, passPathLen, &foundGroups, &foundKeys](QString dirToScan, QStandardItem* parent) {
        QStandardItem* me = new QStandardItem(dirToScan.right(dirToScan.length() - passPathLen));
        if (parent != nullptr)
            parent->appendRow(me);
        else
            sim_->appendRow(me);

        QDirIterator passDirIt(dirToScan);
        while (passDirIt.hasNext()) {
            passDirIt.next();

            if (passDirIt.fileName() == "." || passDirIt.fileName() == "..")
                continue;

            QFileInfo fInfo = passDirIt.fileInfo();

            QString path = passDirIt.filePath();
            path = path.right(path.length() - passPathLen);

            if (fInfo.isDir()) {
                foundGroups++;
                recursion(passDirIt.filePath(), me);
            } else {
                foundKeys++;
                me->appendRow(new QStandardItem(path));
            }
        }
    };

    recursion(passDirPath, nullptr);
    /*
        if (passDirIt.fileInfo().isDir()) {
            // Is a dir -> group
            foundGroups++;
            groups_.append(passDirIt.fileName());
        } else {
            foundKeys++;
            QString path = passDirIt.filePath();
            path = path.right(path.length() - passPathLen);
            QStringList parts = path.split("/");
            QString entry = parts.takeLast();
            int index;
            for (QString group : parts) {
                index = groups_.indexOf(group);
                QMap<int, QStringList>::iterator finder = entries_.find(index);
                QStringList *entriesInGroup;
                if (finder == entries_.end()) {
                    entriesInGroup = new QStringList;
                    entries_.insert(index, *entriesInGroup);
                } else {
                    QStringList &eIG = finder.value();
                    entriesInGroup = &eIG;
                }
                entriesInGroup->append(entry);
            }
        }
    }*/

    qDebug("[%s] Found %d password files in %d grouping directories", name_, foundKeys, foundGroups);

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
Pass::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);
    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
QWidget *Pass::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
        //QTreeWidget *tree = widget_->ui.treeWidget;
        QTreeView *tree = widget_->ui.treeView;
        tree->setModel(sim_);
        /*
        for (int i = 0; i < groups_.length(); i++) {
            QMap<int, QStringList>::iterator finder = entries_.find(i);
            QTreeWidgetItem *topGroupItem;
            if (finder == entries_.end())
                topGroupItem = new QTreeWidgetItem(tree);
            else {
                topGroupItem = new QTreeWidgetItem(tree);
                topGroupItem->setExpanded(true);
                QStringList &entries = finder.value();
                for (QString &ent : entries)
                    new QTreeWidgetItem(topGroupItem, {ent});
            }
            topGroupItem->setText(0, groups_.at(i));
        }
        */
    }
    return widget_;
}



/** ***************************************************************************/
void Pass::Extension::handleQuery(shared_ptr<Query> query) {
    QStringList queryWords = query->searchTerm().toLower().split(QRegExp(" /"), QString::SkipEmptyParts);

    std::function<void(QStandardItem*)> recursion = [&query, &queryWords, &recursion](QStandardItem *si){
        if (si->hasChildren())
            for (int i = 0; i < si->rowCount(); i++)
                recursion(si->child(i));
        else if (si->text().startsWith(query->searchTerm()))
            query->addMatch(std::shared_ptr<AlbertItem>(new Item(si->text())));
    };
    for (int i = 0; i < sim_->rowCount(); i++) {
        recursion(sim_->item(i));
    }
/*
    for (QStringList &entries : entries_.values())
        for (QString &entry : entries)
            for (QString &queryWord: queryWords)
                if (entry.toLower().startsWith(queryWord))
                    query->addMatch(shared_ptr<Item>(new Item(entry)), queryWord.length()/entry.length());

    for (int i = 0; i < groups_.length(); i++) {
        const QString &group = groups_.at(i);
        for (QString &queryWord : queryWords)
            if (group.toLower().startsWith(queryWord))
                for (QString &entry : entries_.find(i).value())
                    query->addMatch(shared_ptr<Item>(new Item(entry)), queryWord.length()/2/group.length());
    }*/
}
