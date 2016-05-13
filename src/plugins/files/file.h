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
#include <QMimeType>
#include <map>
#include <chrono>
#include "iindexable.h"
#include "abstractobjects.hpp"

namespace Files {

class File final : public AlbertItem, public IIndexable
{
    class OpenFileAction;
    class RevealFileAction;
    class CopyFileAction;
    class CopyPathAction;

public:

    File() : usage_(0) {}
    File(QString path, QMimeType mimetype, short usage = 0)
        : path_(path), mimetype_(mimetype), usage_(usage){}

    /*
     * Implementation of AlbertItem interface
     */

    QString text() const override;
    QString subtext() const override;
    QString iconPath() const override;
    vector<QString> indexKeywords() const override;
    void activate(ExecutionFlags *) override;
    uint16_t usageCount() const { return usage_; }
    ActionSPtrVec actions() override;

    /*
     * Item specific members
     */

    /** Return the path of the file */
    const QString &path() const { return path_; }

    /** Return the usage count */
    uint16_t usage() const { return usage_; }

    /** Sets the usage count */
    void setUsage(uint16_t usage) { usage_ = usage; }

    /** Return the mimetype of the file */
    const QMimeType &mimetype() const { return mimetype_; }

    /** Serialize the desktop entry */
    void serialize(QDataStream &out);

    /** Deserialize the desktop entry */
    void deserialize(QDataStream &in);

private:

    QString path_;
    QMimeType mimetype_;
    mutable uint16_t usage_;
    struct CacheEntry {
        QString path;
        std::chrono::system_clock::time_point ctime;
    };
    static std::map<QString, CacheEntry> iconCache_;
};

}
