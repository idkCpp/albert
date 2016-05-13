// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
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
#include <QFileDialog>
#include <QSettings>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "extension.h"

QString cmdline::ConfigWidget::xterm("xterm");
QString cmdline::ConfigWidget::xtermOptions("-hold -e \"cd %1; %2\"");

/** ***************************************************************************/
cmdline::ConfigWidget::ConfigWidget(QWidget *parent, QString& cmdExe, QString& params) : QWidget(parent) {
    ui.setupUi(this);

    if (!cmdExe.isEmpty())
        ui.lineEdit_executable->setText(cmdExe);
    else
        ui.lineEdit_executable->setText(xterm);
    if (!params.isEmpty())
        ui.lineEdit_options->setText(params);
    else
        ui.lineEdit_options->setText(xtermOptions);

    connect(ui.pushButton_search, &QPushButton::clicked,
            this, &ConfigWidget::clickedSearch);
    connect(ui.pushButton_save, &QPushButton::clicked,
            this, &ConfigWidget::clickedSave);
}



/** ***************************************************************************/
cmdline::ConfigWidget::~ConfigWidget() {

}

void cmdline::ConfigWidget::clickedSearch() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                "/bin"); //,
                                                //tr("Images (*.png *.xpm *.jpg)"));
    if(path.isEmpty())
        return;

    ui.lineEdit_executable->setText(path);
//    emit requestAddPath(path);
}

void cmdline::ConfigWidget::clickedSave() {
    struct stat sb;

    QString file = ui.lineEdit_executable->text();
    QString params = ui.lineEdit_options->text();

    if (stat(file.toStdString().c_str(), &sb) == 0 && sb.st_mode & S_IXUSR)
        emit requestNewExecutable(file, params);
    else
        qDebug("ERROR NO EXE");
}
