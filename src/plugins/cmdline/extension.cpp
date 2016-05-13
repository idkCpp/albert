// albert extension cmdline - a cmdline launcher for albert
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
#include <QString>
#include <QStringList>
#include <QStandardPaths>
#include <QSettings>
#include <QTemporaryFile>
#include <QProcess>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "extension.h"
#include "albertapp.h"
#include "configwidget.h"
#include "item.h"
#include "query.h"
#include "objects.hpp"
#include "xdgiconlookup.h"

#define CFG_TERMINAL        "terminal"
#define DEF_TERMINAL        "/usr/bin/xterm"
#define CFG_TERMINAL_NAME   "terminal-name"
#define DEF_TERMINAL_NAME   "xterm"
#define CFG_TERMINAL_OPTS   "terminal-options"
#define DEF_TERMINAL_OPTS   "-hold -e \"%1\""

/** ***************************************************************************/
cmdline::Extension::Extension() : IExtension("CmdLine") {
    qDebug("[%s] Initialize extension", name_);

    iconPath_ = XdgIconLookup::instance()->themeIconPath("utilities-terminal", QIcon::themeName());
    Item::setIconPath(iconPath_);

    QSettings s;
    s.beginGroup(name_);
    cmdExe        = s.value(CFG_TERMINAL     , DEF_TERMINAL     ).toString();
    cmdExeName    = s.value(CFG_TERMINAL_NAME, DEF_TERMINAL_NAME).toString();
    cmdExeOptions = s.value(CFG_TERMINAL_OPTS, DEF_TERMINAL_OPTS).toString();

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
cmdline::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);
    // Do sth.
    QSettings s;
    s.beginGroup(name_);
    s.setValue(CFG_TERMINAL, cmdExe);
    s.setValue(CFG_TERMINAL_NAME, cmdExeName);
    s.setValue(CFG_TERMINAL_OPTS, cmdExeOptions);

    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
void cmdline::Extension::newExe(QString path, QString params) {
    //qDebug("Using %s", path.toStdString().c_str());
    cmdExe = path;
    int i = path.lastIndexOf("/");
    cmdExeName = path.right(path.length() - i -1);
    cmdExeOptions = params;
}



/** ***************************************************************************/
QWidget *cmdline::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent, cmdExe, cmdExeOptions);
        connect(widget_, &ConfigWidget::requestNewExecutable, this, &Extension::newExe);
    }
    return widget_;
}



/** ***************************************************************************/
void cmdline::Extension::setupSession() {

}



/** ***************************************************************************/
void cmdline::Extension::teardownSession() {

}



/** ***************************************************************************/
void cmdline::Extension::handleQuery(shared_ptr<Query> query) {

    QString line(query->searchTerm());
    QString workingDir = "~";

    // Check if user wants to specify a CWD
    if (line.contains("$")) {
        int i = line.indexOf('$');
        workingDir = line.left(i);
        struct stat s;
        int res = stat(workingDir.toStdString().c_str(), &s);
        if (res == -1 || !S_ISDIR(s.st_mode)) {
            workingDir = "~";
        } else {
            line = line.right(line.length() - i -1);
        }
    }

    // Check if there are arguments
    if (line.contains(' ')) {
        // The line has a command and arguments seperated by a space

        QStringList parts = line.split(" ", QString::SkipEmptyParts);
        if (parts.length() == 0) {
            // This line appears to consist of spaces....
            return;
        }

        // Check for a executable
        QString exe = QStandardPaths::findExecutable(parts[0]);
        if (!exe.isEmpty()) {
            // Found it!
            execute(parts[0], line, query, workingDir);
        }
        // If not found, ignore!
    } else {
        // There are no arguements. Search whole string
        QString exe = QStandardPaths::findExecutable(line);
        if (!exe.isEmpty()) {
            // Found it!
            execute(line, line, query, workingDir);
        }
        // If not found, ignore!
    }
}



/** ***************************************************************************/
void cmdline::Extension::execute(QString& cmd, QString& line, shared_ptr<Query> query, QString& workingDir) {

    QString result = QString("Execute %1 in %2").arg(cmd).arg(cmdExeName);

    // Create a temporary script
    QString script("#!/bin/bash\n\n");
    script.append("cd ").append(workingDir).append("\n");
    script.append(line).append("\n");

    // Write it to a cache file
    bool success = false;
    QFile tmpFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation).append("/albert.sh"));
    if (tmpFile.open(QFile::WriteOnly)) {
        if (tmpFile.setPermissions(QFile::ExeUser | QFile::ReadUser | QFile::WriteUser)) {
            tmpFile.write(script.toStdString().c_str(), script.length());
            success = true;
        } else {
            qCritical("Cannot set temporary shell script to executable!");
        }
    } else {
        qCritical("Failed to create file!");
    }
    tmpFile.close();

    if (!success)
        return;

    // Create a commandline to execute the script
    QString toExec(cmdExe);
    toExec.append(" ");
    toExec.append(cmdExeOptions.arg(tmpFile.fileName()));


    // And make an item for this
    std::shared_ptr<AlbertItem> ptr(new Item(result, query->searchTerm(), toExec));
    query->addMatch(ptr, SHRT_MAX);
}



/** ***************************************************************************/
void cmdline::Extension::handleFallbackQuery(shared_ptr<Query> query) {
    // Avoid annoying warnings
    Q_UNUSED(query)
}
