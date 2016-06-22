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

#include "configwidget.h"
#include <QListWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include "extension.h"

/** ***************************************************************************/
SSH::ConfigWidget::ConfigWidget(QWidget *parent, QStringList& dirs) : QWidget(parent) {
    ui.setupUi(this);

    if (!dirs.isEmpty()) {
        dirs_ = dirs;
        ui.listWidget->addItems(dirs);
    }

    connect(ui.add_btn, &QPushButton::clicked, this, &ConfigWidget::add_clicked);
    connect(ui.del_btn, &QPushButton::clicked, this, &ConfigWidget::del_clicked);
}



/** ***************************************************************************/
SSH::ConfigWidget::~ConfigWidget() {

}

void SSH::ConfigWidget::add_clicked()
{
    QStringList path = QFileDialog::getOpenFileNames(
                this,
                tr("Choose path"),
                QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

    if(path.isEmpty())
        return;

    dirs_ += path;
    dirs_.removeDuplicates();

    ui.listWidget->clear();
    ui.listWidget->addItems(dirs_);

    emit dirsChanged(dirs_);
}

void SSH::ConfigWidget::del_clicked()
{
    QListWidget* widget = ui.listWidget;
    QList<QListWidgetItem *> selected = widget->selectedItems();
    for (QListWidgetItem* item : selected)
        dirs_.removeOne(item->text());

    ui.listWidget->clear();
    ui.listWidget->addItems(dirs_);

    emit dirsChanged(dirs_);
}
