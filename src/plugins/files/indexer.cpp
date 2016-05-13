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

#include <QDirIterator>
#include <QDebug>
#include <QThread>
#include <map>
#include <set>
#include <functional>
#include "indexer.h"
#include "file.h"
#include "extension.h"


/** ***************************************************************************/
void Files::Extension::Indexer::run() {

    // Notification
    qDebug("[%s] Start indexing in background thread", extension_->name_);
    emit statusInfo("Indexing files ...");

    // Prepare the iterator properties
    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot;
    if (extension_->indexHidden_)
        filters |= QDir::Hidden;
    QDirIterator::IteratorFlags flags;
    if (extension_->followSymlinks_)
        flags = QDirIterator::FollowSymlinks;


    // Get a new index
    std::vector<shared_ptr<File>> newIndex;
    std::set<QString> indexedDirs;


    // Anonymous function that implemnents the index recursion
    std::function<void(const QFileInfo&)> indexRecursion =
            [this, &newIndex, &indexedDirs, &filters, &flags, &indexRecursion](const QFileInfo& fileInfo){
        if (abort_) return;

        const QString canonicalPath = fileInfo.canonicalFilePath();


        if (fileInfo.isFile()) {

            // If the file matches the index options, index it
            QMimeType mimetype = mimeDatabase_.mimeTypeForFile(canonicalPath);
            const QString mimeName = mimetype.name();
            if ((extension_->indexAudio_ && mimeName.startsWith("audio"))
                    ||(extension_->indexVideo_ && mimeName.startsWith("video"))
                    ||(extension_->indexImage_ && mimeName.startsWith("image"))
                    ||(extension_->indexDocs_ &&
                       (mimeName.startsWith("application") || mimeName.startsWith("text")))) {
                newIndex.push_back(std::make_shared<File>(canonicalPath, mimetype));
            }
        } else if (fileInfo.isDir()) {

            emit statusInfo(QString("Indexing %1.").arg(canonicalPath));

            // Skip if this dir has already been indexed
            if (indexedDirs.find(canonicalPath)!=indexedDirs.end())
                return;

            // Remember that this dir has been indexed to avoid loops
            indexedDirs.insert(canonicalPath);

            // If the dir matches the index options, index it
            if (extension_->indexDirs_) {
                QMimeType mimetype = mimeDatabase_.mimeTypeForFile(canonicalPath);
                newIndex.push_back(std::make_shared<File>(canonicalPath, mimetype));
            }

            // Ignore ignorefile by default
            std::vector<QRegExp> ignores;
            ignores.push_back(QRegExp(extension_->IGNOREFILE, Qt::CaseSensitive, QRegExp::Wildcard));

            // Read the ignore file, see http://doc.qt.io/qt-5/qregexp.html#wildcard-matching
            QFile file(QDir(canonicalPath).filePath(extension_->IGNOREFILE));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd())
                    ignores.push_back(QRegExp(in.readLine().trimmed(), Qt::CaseSensitive, QRegExp::Wildcard));
                file.close();
            }

            // Index all children in the dir
            QDirIterator dirIterator(canonicalPath, filters, flags);
            while (dirIterator.hasNext()) {
                dirIterator.next();

                // Skip if this file matches one of the ignore patterns
                for (const QRegExp& ignore : ignores){
                    QString s = dirIterator.fileName(); // This is insane works only if its a lvalue
                    if(ignore.exactMatch(s))
                        goto SKIP_THIS;
                }

                // Index this file
                indexRecursion(dirIterator.fileInfo());
                SKIP_THIS:;
            }
        }
    };


    // Start the indexing
    for (const QString &rootDir : extension_->rootDirs_) {
        indexRecursion(QFileInfo(rootDir));
        if (abort_) return;
    }


    // Sort the new index for linear usage copy [O(n*log(n))]
    emit statusInfo("Sorting ... ");
    std::sort(newIndex.begin(), newIndex.end(), [](const shared_ptr<File> &lhs, const shared_ptr<File> &rhs) {
                  return QString::compare(lhs->path(), rhs->path(), Qt::CaseInsensitive) < 0;
              });


    // Copy the usagecounters  [O(n)]
    emit statusInfo("Copy usage statistics ... ");
    size_t i=0, j=0;
    while (i < extension_->index_.size() && j < newIndex.size()) {
        if (extension_->index_[i]->path() == newIndex[j]->path()) {
            newIndex[j]->setUsage(extension_->index_[i]->usage());
            ++i;++j;
        } else if (extension_->index_[i]->path() < newIndex[j]->path()) {
            ++i;
        } else {// if ((*_fileIndex)[i]->path > (*newIndex)[j]->path) {
            ++j;
        }
    }

    /*
     *  ▼ CRITICAL ▼
     */

    // Lock the access
    QMutexLocker locker(&extension_->indexAccess_);

    // Abortion requested while block
    if (abort_)
        return;

    // Set the new index (use swap to shift destruction out of critical area)
    std::swap(extension_->index_, newIndex);

    // Rebuild the offline index
    extension_->offlineIndex_.clear();
    for (auto &item : extension_->index_)
        extension_->offlineIndex_.add(item);

    // Notification
    qDebug("[%s] Indexing done (%d items)", extension_->name_, static_cast<int>(extension_->index_.size()));
    emit statusInfo(QString("Indexed %1 files").arg(extension_->index_.size()));
}
