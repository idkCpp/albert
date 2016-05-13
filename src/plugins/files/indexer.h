// albert - a simple application launcher for linux
// Copyright (C) 2014-2016 Manuel Schneider
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
#include <QRunnable>
#include <QMimeDatabase>
#include <QMutex>
#include "extension.h"

namespace Files {

class Extension::Indexer final : public QObject, public QRunnable
{
    Q_OBJECT
public:
    Indexer(Extension *ext)
        : extension_(ext), abort_(false) {}
    void run() override;
    void abort(){abort_=true;}

private:
    Extension *extension_;
    QMimeDatabase mimeDatabase_;
    bool abort_;

signals:
    void statusInfo(const QString&);
};
}
