// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include <QObject>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QDataStream>

class History final : public QObject
{
    Q_OBJECT
public:
    explicit History(QString filePath, QObject *parent = 0)
        : QObject(parent), filePath_(filePath) {
        QFile historyFile(filePath_);
        if (historyFile.exists()){
            if (historyFile.open(QIODevice::ReadOnly| QIODevice::Text)) {
                QDataStream in(&historyFile);
                in >> lines_;
                historyFile.close();
            } else qWarning() << "Could not open file" << historyFile.fileName();
        }
        currentLine_ = -1; // This means historymode is not active
    }

    ~History() {
        QFile historyFile(filePath_);
        if (historyFile.open(QIODevice::ReadWrite| QIODevice::Text)) {
            QDataStream out(&historyFile);
            out << lines_;
            historyFile.close();
        } else qCritical() << "Could not write to " << historyFile.fileName();
    }

    Q_INVOKABLE void add(QString str) {
        if (!str.isEmpty()){
            if (lines_.contains(str))
                lines_.removeAll(str); // Remove dups
            lines_.prepend(str);
        }
    }

    Q_INVOKABLE QString next() {
        if (currentLine_+1 < static_cast<int>(lines_.size())
                && static_cast<int>(lines_.size())!=0 ) {
            ++currentLine_;
            return lines_[currentLine_];
        } else return QString();
    }

    Q_INVOKABLE QString prev() {
        if (0 < currentLine_) {
            --currentLine_;
            return lines_[currentLine_];
        } else return QString();
    }

public slots:
    void resetIterator() {currentLine_ = -1;}
    void clearHistory() {lines_.clear();}

private:
    QStringList lines_;
    int currentLine_;
    const QString filePath_;
};

