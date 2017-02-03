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

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLibrary>
#include <QPluginLoader>
#include <QSettings>
#include <QStandardPaths>
#include <chrono>
#include <memory>
#include "extensionmanager.h"
#include "extensionspec.h"
using std::set;
using std::unique_ptr;
using std::vector;
using std::chrono::system_clock;
#include "trackbert.h"

namespace {
const QString CFG_BLACKLIST = "blacklist";
}

Core::ExtensionManager *Core::ExtensionManager::instance = nullptr;

/** ***************************************************************************/
class Core::ExtensionManagerPrivate {
public:
    vector<unique_ptr<ExtensionSpec>> extensionSpecs_; // TASK: Rename _
    set<QObject*> extensions_;
    QStringList blacklist_;
    QStringList pluginDirs;
};


/** ***************************************************************************/
Core::ExtensionManager::ExtensionManager() : d(new ExtensionManagerPrivate) {
    // Load blacklist
    d->blacklist_ = QSettings(qApp->applicationName()).value(CFG_BLACKLIST).toStringList();
    // DO NOT LOAD EXTENSIONS HERE!

    // Get plugindirs
#if defined __linux__

    QStringList dirs = {
        "/usr/lib/", "/usr/local/lib/", "/usr/lib64/", "/usr/local/lib64/",
        QDir::home().filePath(".local/lib/"),
        QDir::home().filePath(".local/lib64/")
    };

    for ( const QString& dir : dirs ) {
        QFileInfo fileInfo = QFileInfo(QDir(dir).filePath("albert/plugins"));
        if ( fileInfo.isDir() )
            d->pluginDirs << fileInfo.absoluteFilePath();
    }

#elif defined __APPLE__
    throw "TODO";
#elif defined _WIN32
    throw "TODO";
#endif

}


/** ***************************************************************************/
Core::ExtensionManager::~ExtensionManager() {
    for (unique_ptr<ExtensionSpec> & extensionSpec : d->extensionSpecs_)
        unloadExtension(extensionSpec);
}


/** ***************************************************************************/
void Core::ExtensionManager::setPluginDirs(const QStringList &dirs) {
    d->pluginDirs = dirs;
}


/** ***************************************************************************/
void Core::ExtensionManager::reloadExtensions() {

    // Unload everything
    for (unique_ptr<ExtensionSpec> & extensionSpec : d->extensionSpecs_)
        unloadExtension(extensionSpec);

    d->extensionSpecs_.clear();

    // Iterate over all files in the plugindirs
    for (const QString &pluginDir : d->pluginDirs) {
        QDirIterator dirIterator(pluginDir, QDir::Files);
        while (dirIterator.hasNext()) {
            dirIterator.next();

            QString path = dirIterator.fileInfo().canonicalFilePath();

            // Check if this path is a lib
            if (QLibrary::isLibrary(path)) {

                QPluginLoader loader(path);

                // Check for a sane interface ID  (IID)
                QString iid = loader.metaData()["IID"].toString();
                if (iid != ALBERT_EXTENSION_IID)
                    continue;

                // Check for duplicates
                QString id = loader.metaData()["MetaData"].toObject()["id"].toString();
                if (std::find_if (d->extensionSpecs_.begin(), d->extensionSpecs_.end(),
                                  [&id](const unique_ptr<ExtensionSpec> & extensionSpec){
                                      return id == extensionSpec->id();
                                  }) != d->extensionSpecs_.end())
                    continue;

                // Put it to the results
                d->extensionSpecs_.emplace_back(new ExtensionSpec(path));
            }
        }
    }

    // Sort alphabetically
    std::sort(d->extensionSpecs_.begin(),
              d->extensionSpecs_.end(),
              [](const unique_ptr<ExtensionSpec>& lhs, const unique_ptr<ExtensionSpec>& rhs){ return lhs->name() < rhs->name(); });

    // Load if not blacklisted
    for (unique_ptr<ExtensionSpec> & extensionSpec : d->extensionSpecs_)
        if (!d->blacklist_.contains(extensionSpec->id()))
            loadExtension(extensionSpec);
}


/** ***************************************************************************/
const vector<unique_ptr<Core::ExtensionSpec>>& Core::ExtensionManager::extensionSpecs() const {
    return d->extensionSpecs_;
}


/** ***************************************************************************/
const set<QObject*> Core::ExtensionManager::objects() const {
    return d->extensions_;
}


/** ***************************************************************************/
void Core::ExtensionManager::loadExtension(const unique_ptr<ExtensionSpec> &spec) {
    if (spec->state() != ExtensionSpec::State::Loaded){
        system_clock::time_point start = system_clock::now();
        if ( spec->load() ) {
            auto msecs = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now()-start);
            qDebug() << QString("Loading %1 done in %2 milliseconds").arg(spec->id()).arg(msecs.count()).toLocal8Bit().data();
            memstat_t mem = allocatedMemory();
            qDebug("Current memory usage: %.3f MiB with additional %.2f %% overhead", (float)mem.payloadSize / 1024/1024, mem.overheadRatio*100);
            d->extensions_.insert(spec->instance());
        } else
            qDebug() << QString("Loading %1 failed. (%2)").arg(spec->id(), spec->lastError()).toLocal8Bit().data();
    }
}


/** ***************************************************************************/
void Core::ExtensionManager::unloadExtension(const unique_ptr<ExtensionSpec> &spec) {
    if (spec->state() != ExtensionSpec::State::NotLoaded) {
        d->extensions_.erase(spec->instance());
        spec->unload();
    }
}


/** ***************************************************************************/
void Core::ExtensionManager::enableExtension(const unique_ptr<ExtensionSpec> &spec) {
    d->blacklist_.removeAll(spec->id());
    QSettings(qApp->applicationName()).setValue(CFG_BLACKLIST, d->blacklist_);
    loadExtension(spec);
}


/** ***************************************************************************/
void Core::ExtensionManager::disableExtension(const unique_ptr<ExtensionSpec> &spec) {
    if (!d->blacklist_.contains(spec->id())){
        d->blacklist_.push_back(spec->id());
        QSettings(qApp->applicationName()).setValue(CFG_BLACKLIST, d->blacklist_);
    }
    unloadExtension(spec);
}


/** ***************************************************************************/
bool Core::ExtensionManager::extensionIsEnabled(const unique_ptr<ExtensionSpec> &spec) {
    return !d->blacklist_.contains(spec->id());
}


/** ***************************************************************************/
void Core::ExtensionManager::registerObject(QObject *object) {
    d->extensions_.insert(object);
}


/** ***************************************************************************/
void Core::ExtensionManager::unregisterObject(QObject *object) {
    d->extensions_.erase(object);
}
