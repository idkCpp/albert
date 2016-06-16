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

#include <QDebug>
#include <QStandardPaths>
#include <QMessageBox>
#include <QThreadPool>
#include <QDir>
#include <memory>
#include "extension.h"
#include "configwidget.h"
#include "indexer.h"
#include "file.h"
#include "query.h"
#include "albertapp.h"

const char* Files::Extension::CFG_PATHS           = "paths";
const char* Files::Extension::CFG_FUZZY           = "fuzzy";
const bool  Files::Extension::DEF_FUZZY           = false;
const char* Files::Extension::CFG_INDEX_AUDIO     = "index_audio";
const bool  Files::Extension::DEF_INDEX_AUDIO     = true;
const char* Files::Extension::CFG_INDEX_VIDEO     = "index_video";
const bool  Files::Extension::DEF_INDEX_VIDEO     = true;
const char* Files::Extension::CFG_INDEX_IMAGE     = "index_image";
const bool  Files::Extension::DEF_INDEX_IMAGE     = true;
const char* Files::Extension::CFG_INDEX_DOC       = "index_docs";
const bool  Files::Extension::DEF_INDEX_DOC       = true;
const char* Files::Extension::CFG_INDEX_DIR       = "index_dirs";
const bool  Files::Extension::DEF_INDEX_DIR       = true;
const char* Files::Extension::CFG_INDEX_HIDDEN    = "index_hidden";
const bool  Files::Extension::DEF_INDEX_HIDDEN    = false;
const char* Files::Extension::CFG_FOLLOW_SYMLINKS = "follow_symlinks";
const bool  Files::Extension::DEF_FOLLOW_SYMLINKS = false;
const char* Files::Extension::CFG_SCAN_INTERVAL   = "scan_interval";
const uint  Files::Extension::DEF_SCAN_INTERVAL   = 60;
const char* Files::Extension::IGNOREFILE          = ".albertignore";


/** ***************************************************************************/
Files::Extension::Extension() : IExtension("Files") {
    qDebug("[%s] Initialize extension", name_);
    minuteTimer_.setInterval(60000);

    // Load settings
    qApp->settings()->beginGroup(name_);
    indexAudio_ = qApp->settings()->value(CFG_INDEX_AUDIO, DEF_INDEX_AUDIO).toBool();
    indexVideo_ = qApp->settings()->value(CFG_INDEX_VIDEO, DEF_INDEX_VIDEO).toBool();
    indexImage_ = qApp->settings()->value(CFG_INDEX_IMAGE, DEF_INDEX_IMAGE).toBool();
    indexDocs_ =  qApp->settings()->value(CFG_INDEX_DOC, DEF_INDEX_DOC).toBool();
    indexDirs_ =  qApp->settings()->value(CFG_INDEX_DIR, DEF_INDEX_DIR).toBool();
    indexHidden_ = qApp->settings()->value(CFG_INDEX_HIDDEN, DEF_INDEX_HIDDEN).toBool();
    followSymlinks_ = qApp->settings()->value(CFG_FOLLOW_SYMLINKS, DEF_FOLLOW_SYMLINKS).toBool();
    offlineIndex_.setFuzzy(qApp->settings()->value(CFG_FUZZY, DEF_FUZZY).toBool());
    setScanInterval(qApp->settings()->value(CFG_SCAN_INTERVAL, DEF_SCAN_INTERVAL).toUInt());

    // Load the paths or set a default
    QVariant v = qApp->settings()->value(CFG_PATHS);
    if (v.isValid() && v.canConvert(QMetaType::QStringList))
        rootDirs_ = v.toStringList();
    else
        restorePaths();

    // Deserialize data
    QFile dataFile(QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).
                   filePath(QString("%1.dat").arg(name_)));
    if (dataFile.exists()) {
        if (dataFile.open(QIODevice::ReadOnly| QIODevice::Text)) {
            qDebug() << "[Files] Deserializing from" << dataFile.fileName();
            QDataStream in(&dataFile);
            quint64 count;
            for (in >> count ;count != 0; --count){
                shared_ptr<File> file = std::make_shared<File>();
                file->deserialize(in);
                index_.push_back(file);
            }
            dataFile.close();

            // Build the offline index
            for (const auto &item : index_)
                offlineIndex_.add(item);
        } else
            qWarning() << "Could not open file: " << dataFile.fileName();
    }

    // scan interval timer
    connect(&minuteTimer_, &QTimer::timeout, this, &Extension::onMinuteTick);

    // If the root dirs change write it to the settings
    connect(this, &Extension::rootDirsChanged, [this](const QStringList& dirs){
        qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_PATHS), dirs);
    });

    // Trigger an initial update
    updateIndex();

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
Files::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);

    /*
     * Stop and wait for background indexer.
     * This should be thread safe since this thread is responisble to start the
     * indexer and, connections to this thread are disconnected in the QObject
     * destructor and all events for a deleted object are removed from the event
     * queue.
     */
    minuteTimer_.stop();
    if (!indexer_.isNull()) {
        indexer_->abort();
        QEventLoop loop;
        connect(indexer_.data(), &Indexer::destroyed, &loop, &QEventLoop::quit);
        loop.exec();
    }

    // Serialize data
    QFile dataFile(QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).
                   filePath(QString("%1.dat").arg(name_)));
    if (dataFile.open(QIODevice::ReadWrite| QIODevice::Text)) {
        qDebug("[%s] Serializing to %s", name_, dataFile.fileName().toLocal8Bit().data());
        QDataStream out( &dataFile );
        out	<< static_cast<quint64>(index_.size());
        for (const auto &item : index_)
            item->serialize(out);
        dataFile.close();
    } else
        qCritical() << "Could not write to " << dataFile.fileName();

    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
QWidget *Files::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);

        AUTOCONF_SETUP(widget_);

        // Paths
        widget_->ui.listWidget_paths->addItems(rootDirs_);
        connect(this, &Extension::rootDirsChanged, widget_->ui.listWidget_paths, &QListWidget::clear);
        connect(this, &Extension::rootDirsChanged, widget_->ui.listWidget_paths, &QListWidget::addItems);
        connect(widget_.data(), &ConfigWidget::requestAddPath, this, &Extension::addDir);
        connect(widget_.data(), &ConfigWidget::requestRemovePath, this, &Extension::removeDir);
        connect(widget_->ui.pushButton_restore, &QPushButton::clicked, this, &Extension::restorePaths);
        connect(widget_->ui.pushButton_update, &QPushButton::clicked, this, &Extension::updateIndex, Qt::QueuedConnection);

        // Checkboxes
        AUTOCONF(widget_->ui.checkBox_audio, CFG_INDEX_AUDIO, indexAudio_, indexAudio());
        AUTOCONF(widget_->ui.checkBox_video, CFG_INDEX_VIDEO, indexVideo_, indexVideo());
        AUTOCONF(widget_->ui.checkBox_image, CFG_INDEX_IMAGE, indexImage_, indexImage());
        AUTOCONF(widget_->ui.checkBox_docs, CFG_INDEX_DOC, indexDocs_, indexDocs());
        AUTOCONF(widget_->ui.checkBox_dirs, CFG_INDEX_DIR, indexDirs_, indexDirs());
        AUTOCONF(widget_->ui.checkBox_hidden, CFG_INDEX_HIDDEN, indexHidden_, indexHidden());
        AUTOCONF(widget_->ui.checkBox_followSymlinks, CFG_FOLLOW_SYMLINKS, followSymlinks_, followSymlinks());

        //widget_->ui.checkBox_fuzzy->setChecked(fuzzy());
        AUTOCONF(widget_->ui.checkBox_fuzzy, CFG_FUZZY, configurator->voidBool, fuzzy());
        connect(widget_->ui.checkBox_fuzzy, &QCheckBox::toggled, this, &Extension::setFuzzy);

        widget_->ui.spinBox_interval->setValue(scanInterval());
        connect(widget_->ui.spinBox_interval, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Extension::setScanInterval);

        // Info
        widget_->ui.label_info->setText(QString("%1 files indexed.").arg(index_.size()));
        connect(this, &Extension::statusInfo, widget_->ui.label_info, &QLabel::setText);

        // If indexer is active connect its statusInfo to the infoLabel
        if (!indexer_.isNull())
            connect(indexer_.data(), &Indexer::statusInfo, widget_->ui.label_info, &QLabel::setText);
    }
    return widget_;
}



/** ***************************************************************************/
void Files::Extension::handleQuery(shared_ptr<Query> query) {
    qDebug() << "Index Audio?" << indexAudio_;

    // Skip  short terms since they pollute the output
    if ( query->searchTerm().size() < 3)
        return;

    // Search for matches. Lock memory against indexer
    indexAccess_.lock();
    vector<shared_ptr<IIndexable>> indexables = offlineIndex_.search(query->searchTerm().toLower());
    indexAccess_.unlock();

    // Add results to query. This cast is safe since index holds files only
    for (shared_ptr<IIndexable> obj : indexables)
        // TODO `Search` has to determine the relevance. Set to 0 for now
        query->addMatch(std::static_pointer_cast<File>(obj), 0);
}



/** ***************************************************************************/
void Files::Extension::addDir(const QString &dirPath) {
    qDebug() << "[Files] Adding dir" << dirPath;

    QFileInfo fileInfo(dirPath);

    // Get an absolute file path
    QString absPath = fileInfo.absoluteFilePath();

    // Check existance
    if (!fileInfo.exists()) {
        QMessageBox(QMessageBox::Critical, "Error", absPath + " does not exist.").exec();
        return;
    }

    // Check type
    if(!fileInfo.isDir()) {
        QMessageBox(QMessageBox::Critical, "Error", absPath + " is not a directory.").exec();
        return;
    }

    // Check if there is an identical existing path
    if (rootDirs_.contains(absPath)) {
        QMessageBox(QMessageBox::Critical, "Error", absPath + " has already been indexed.").exec();
        return;
    }

    /* Check if this dir is a sub/superdir of an existing dir. This is fine
       since user may have choosen to ignore some dirs (.albertignore). This is
       more complex but also more flexible. At least inform the user */
    for (const QString &p: rootDirs_)
        if (absPath.startsWith(p + '/'))
            QMessageBox(QMessageBox::Warning, "Warning", absPath + " is subdirectory of " + p).exec();
    for (const QString &p: rootDirs_)
        if (p.startsWith(absPath + '/'))
            QMessageBox(QMessageBox::Warning, "Warning", p + " is subdirectory of " + absPath).exec();

    // Add the path to root dirs
    rootDirs_ << absPath;

    // Inform observers
    emit rootDirsChanged(rootDirs_);
}



/** ***************************************************************************/
void Files::Extension::removeDir(const QString &dirPath) {
    qDebug() << "[Files] Removing path" << dirPath;

    // Get an absolute file path
    QString absPath = QFileInfo(dirPath).absoluteFilePath();

    // Check existance
    if (!rootDirs_.contains(absPath))
        return;

    // Remove the path
    rootDirs_.removeAll(absPath);

    // Update the widget, if it is visible atm
    emit rootDirsChanged(rootDirs_);
}



/** ***************************************************************************/
void Files::Extension::restorePaths() {
    qDebug() << "[Files] Restore paths to defaults";

    // Add standard paths
    rootDirs_.clear();
    addDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
}



/** ***************************************************************************/
void Files::Extension::updateIndex() {
    qDebug() << "[Files] Index update triggered";

    // If thread is running, stop it and start this functoin after termination
    if (!indexer_.isNull()) {
        indexer_->abort();
        if (!widget_.isNull())
            widget_->ui.label_info->setText("Waiting for indexer to shut down ...");
        connect(indexer_.data(), &Indexer::destroyed, this, &Extension::updateIndex, Qt::QueuedConnection);
    } else {
        // Create a new scanning runnable for the threadpool
        indexer_ = new Indexer(this);

        //  Run it
        QThreadPool::globalInstance()->start(indexer_);

        // Reset the timer
        minuteCounter_ = 0;
        minuteTimer_.start();

        // If widget is visible show the information in the status bat
        if (!widget_.isNull())
            connect(indexer_.data(), &Indexer::statusInfo, widget_->ui.label_info, &QLabel::setText);
    }
}



/** ***************************************************************************/
void Files::Extension::setIndexAudio(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_AUDIO), b);
    indexAudio_ = b;
}



/** ***************************************************************************/
void Files::Extension::setIndexVideo(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_VIDEO), b);
    indexVideo_ = b;
}



/** ***************************************************************************/
void Files::Extension::setIndexImage(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_IMAGE), b);
    indexImage_ = b;
}



/** ***************************************************************************/
void Files::Extension::setIndexDocs(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_DOC), b);
    indexDocs_ = b;
}



/** ***************************************************************************/
void Files::Extension::setIndexDirs(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_DIR), b);
    indexDirs_ = b;
}



/** ***************************************************************************/
void Files::Extension::setIndexHidden(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_INDEX_HIDDEN), b);
    indexHidden_ = b;
}



/** ***************************************************************************/
void Files::Extension::setFollowSymlinks(bool b)  {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_FOLLOW_SYMLINKS), b);
    followSymlinks_ = b;
}



/** ***************************************************************************/
void Files::Extension::setScanInterval(uint minutes) {
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_SCAN_INTERVAL), minutes);
    scanInterval_=minutes;
    minuteCounter_=0;
    (minutes == 0) ? minuteTimer_.stop() : minuteTimer_.start();
    qDebug() << "[Files] Scan interval set to" << scanInterval_ << "minutes.";
}



/** ***************************************************************************/
void Files::Extension::setFuzzy(bool b) {
    indexAccess_.lock();
    qApp->settings()->setValue(QString("%1/%2").arg(name_, CFG_FUZZY), b);
    offlineIndex_.setFuzzy(b);
    indexAccess_.unlock();
}



/** ***************************************************************************/
void Files::Extension::onMinuteTick() {
    ++minuteCounter_;
    if (minuteCounter_ == scanInterval_)
        updateIndex(); // resets minuteCounter
}
