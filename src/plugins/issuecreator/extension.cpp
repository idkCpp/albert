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

#include <QDebug>
#include <QPlainTextEdit>
#include <QProcess>
#include "extension.h"
#include "configwidget.h"
#include "query.h"
#include "albertapp.h"

/** ***************************************************************************/
IssueCreator::Extension::Extension() : IExtension("Issue Creator") {
    qDebug("[%s] Initialize extension", name_);

    QProcess findUnameProcess(this);
    findUnameProcess.setProgram("uname");
    findUnameProcess.setArguments({ "-orv" });
    findUnameProcess.start();
    findUnameProcess.waitForFinished();
    QByteArray findUnameRaw = findUnameProcess.readAllStandardOutput();
    findUnameRaw.replace('\n','\0');

    issueTemplateText_ = "#### Environent \n"
            "**Operating system:** `%1`\n"
            "**Desktop environment:** `%2`\n"
            "**Qt version:** `%3`\n"
            "**Albert version:** `%4`\n"
            "**Source:** `compiled from source`\n"
            "\n"
            "#### Steps to reproduce\n"
            "How did you discover this issue?\n"
            "\n"
            "#### Expected behaviour\n"
            "What did you expect to happen?\n"
            "\n"
            "#### Actual behaviour\n"
            "What happened instead?";

    issueTemplateText_ = issueTemplateText_.arg(findUnameRaw, qgetenv("XDG_CURRENT_DESKTOP"), qVersion(), qApp->applicationVersion());


    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
IssueCreator::Extension::~Extension() {
    qDebug("[%s] Finalize extension", name_);
    // Do sth.
    qDebug("[%s] Extension finalized", name_);
}



/** ***************************************************************************/
QWidget *IssueCreator::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
        QPlainTextEdit* textEdit = widget_->ui.issueText;
        textEdit->setPlainText(issueTemplateText_);
    }
    return widget_;
}



/** ***************************************************************************/
void IssueCreator::Extension::handleQuery(shared_ptr<Query> query) {
    // This extension is only for its config widget.
    // It does not actually do anything
    // Avoid annoying warnings
    Q_UNUSED(query)
}
