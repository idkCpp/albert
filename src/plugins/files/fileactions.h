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
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include "file.h"

namespace Files {

/** ***************************************************************************/
class AbstractFileAction : public Action
{
public:
    AbstractFileAction(File *file) : file_(file) {}
    ~AbstractFileAction() {}

protected:
    File *file_;
};



/** ***************************************************************************/
class File::OpenFileAction final : public AbstractFileAction
{
public:
    OpenFileAction(File *file) : AbstractFileAction(file) {}

    QString text() const override { return "Open file in default application"; }
    void activate(ExecutionFlags *) override {
        QDesktopServices::openUrl(QUrl::fromLocalFile(file_->path()));
        ++file_->usage_;
    }
};



/** ***************************************************************************/
class File::RevealFileAction final : public AbstractFileAction
{
public:
    RevealFileAction(File *file) : AbstractFileAction(file) {}

    QString text() const override { return "Reveal file in default filebrowser"; }
    void activate(ExecutionFlags *) override {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(file_->path()).path()));
        ++file_->usage_;
    }
};



/** ***************************************************************************/
class File::CopyFileAction final : public AbstractFileAction
{
public:
    CopyFileAction(File *file) : AbstractFileAction(file) {}

    QString text() const override { return "Copy file to clipboard"; }
    void activate(ExecutionFlags *) override {
        //  Get clipboard
        QClipboard *cb = QApplication::clipboard();

        // Ownership of the new data is transferred to the clipboard.
        QMimeData* newMimeData = new QMimeData();

        // Copy old mimedata
        const QMimeData* oldMimeData = cb->mimeData();
        for (const QString &f : oldMimeData->formats())
            newMimeData->setData(f, oldMimeData->data(f));

        // Copy path of file
        newMimeData->setText(file_->path());

        // Copy file
        newMimeData->setUrls({QUrl::fromLocalFile(file_->path())});

        // Copy file (f*** you gnome)
        QByteArray gnomeFormat = QByteArray("copy\n").append(QUrl::fromLocalFile(file_->path()).toEncoded());
        newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);

        // Set the mimedata
        cb->setMimeData(newMimeData);

        ++file_->usage_;
    }
};



/** ***************************************************************************/
class File::CopyPathAction final : public AbstractFileAction
{
public:
    CopyPathAction(File *file) : AbstractFileAction(file) {}

    QString text() const override { return "Copy path to clipboard"; }
    void activate(ExecutionFlags *) override {
        QApplication::clipboard()->setText(file_->path());
        ++file_->usage_;
    }
};


}
