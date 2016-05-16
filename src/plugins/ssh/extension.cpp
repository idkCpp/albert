// albert extension ssh - open preconfigured ssh sessions from albert
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

#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QSettings>
#include <QProcess>
#include "albertapp.h"
#include "extension.h"
#include "configwidget.h"
#include "query.h"
#include "xdgiconlookup.h"
#include "objects.hpp"

/** ***************************************************************************/
SSH::Extension::Extension() : IExtension("SSH") {
    qDebug("[%s] Initialize extension", name_);
    // Do init

    iconPath_ = XdgIconLookup::instance()->themeIconPath("utilities-terminal.png", QIcon::themeName());

    load();

    buildIndex();

    rebuildTimer_ = new QTimer(this);
    rebuildTimer_->setInterval(5 * 60 * 1000);
    connect(rebuildTimer_, SIGNAL(timeout()), this, SLOT(buildIndex()));
    rebuildTimer_->start();

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
SSH::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);
    // Don't need to delete rebuildTimer_ this is done by Qt
    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
QWidget *SSH::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent, files_);
        connect(widget_, &ConfigWidget::dirsChanged, this, &Extension::rebuildIndex);
    }
    return widget_;
}



/** ***************************************************************************/
void SSH::Extension::handleQuery(shared_ptr<Query> query) {

    QStringList arguments  = query->searchTerm().split(' ', QString::SkipEmptyParts);

    if (arguments.size() < 2)
        return;

    arguments.removeFirst();

    QString hostIdentifier = arguments.join(' ');
    for (QRegExp rx : availableSshConnections_) {
        if (rx.indexIn(hostIdentifier) == 0) {
            std::shared_ptr<StandardItem> result = std::make_shared<StandardItem>();
            result->setText("Start SSH Session");
            result->setSubtext(hostIdentifier);
            result->setAction([hostIdentifier](){
                QString cmd("ssh %1");
                QProcess::startDetached(qApp->term().arg(cmd.arg(hostIdentifier)));
            });
            result->setIcon(iconPath_);
            query->addMatch(result);
        }
    }
}



/** ***************************************************************************/
void SSH::Extension::load() {
    QFile saveFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/org.albert.extension.ssh.dat");

    // Default values
    if (!saveFile.exists()) {
        files_.append("/etc/ssh/ssh_config");
        files_.append(QStandardPaths::locate(QStandardPaths::HomeLocation, ".ssh/config"));
        return;
    }

    if (saveFile.open(QFile::ReadOnly)) {
        files_.clear();
        QTextStream in(&saveFile);

        QString line;
        while (!in.atEnd()) {
            line = in.readLine();
            files_.append(line);
        }

        saveFile.close();
    } else {
        qCritical("[%s] Could not read file list!", name_);
    }
}



/** ***************************************************************************/
void SSH::Extension::save() {
    QFile saveFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/org.albert.extension.ssh.dat");
    if (saveFile.open(QFile::WriteOnly)) {
        QTextStream out(&saveFile);

        for (QString& file: files_) {
            out << file << "\n";
        }

        saveFile.close();
        qDebug("[%s] Saved!", name_);
    } else {
        qCritical("[%s] Could not write file list!", name_);
    }
}



/** ***************************************************************************/
void SSH::Extension::rebuildIndex(const QStringList &list) {
    files_.clear();
    files_ += list;
    save();
    buildIndex();
}



/** ***************************************************************************/
void SSH::Extension::buildIndex() {
    qDebug("[%s] Reading configuration files", name_);
    /* Already handled by default values
    QString sshConfigFileName = QStandardPaths::locate(QStandardPaths::HomeLocation, ".ssh/config");
    if (!sshConfigFileName.isNull() && !sshConfigFileName.isEmpty()) {
        readSshConfigFile(sshConfigFileName);
    }

    QString etcFile("/etc/ssh/ssh_config");
    if (QFile::exists(etcFile)) {
        readSshConfigFile(etcFile);
    }*/

    for (QString& file: files_)
        if (QFile::exists(file))
            readSshConfigFile(file);
}



/** ***************************************************************************/
void SSH::Extension::readSshConfigFile(QString &filepath) {
    QFile sshConfigFile(filepath);
    if (sshConfigFile.open(QFile::ReadOnly)) {
        // Trying to parse .ssh/config

        QTextStream reader(&sshConfigFile);
        QString line, work;
        while (!reader.atEnd()) {
            line = reader.readLine().trimmed();

            if (line.startsWith("#"))
                continue;

            // Keep this space here            ↓
            // It prevents a HostName line to  ↓
            // be recognized as Host line      ↓
            if (line.toLower().startsWith("host ")) {
                // Host line discovered
                work = line.right(line.length() - line.indexOf(' ') -1).trimmed();
                if (work.isEmpty())
                    continue;
                if (work.contains('?')) {
                    work.replace('?', "[A-Za-z0-9]{1}");
                }
                if (work.contains('*') && work.length() > 1) {
                    work.replace('*', "[A-Za-z0-9]*");
                }
                if (work.contains('.')) {
                    work.replace('.', "\\.");
                }
                availableSshConnections_.append(QRegExp(work + "$"));
                qDebug("[%s] Found: %s", name_, work.toStdString().c_str());
            }

        }

    } else {
        qWarning("Failed to open config file for read operation: %s", filepath.toStdString().c_str());
    }
}
