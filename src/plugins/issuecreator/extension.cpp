// albert extension issuecreator - simply copy the template
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

#include <QDebug>
#include <QPlainTextEdit>
#include <QProcess>
#include <QStandardPaths>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include "extension.h"
#include "configwidget.h"
#include "query.h"
#include "albertapp.h"

/** ***************************************************************************/
IssueCreator::Extension::Extension() : IExtension("Issue Creator") {
    qDebug("[%s] Initialize extension", name_);

    QString curl = QStandardPaths::findExecutable("curl");
    hasCurl_ = !curl.isEmpty();

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
        QPushButton* submitBtn = widget_->ui.submitBtn;
        QLineEdit* username = widget_->ui.usernameEdit;
        QLineEdit* password = widget_->ui.passwordEdit;
        QLineEdit* title = widget_->ui.titleEdit;

        textEdit->setPlainText(issueTemplateText_);

        if (hasCurl_) {
            textEdit->setReadOnly(false);
            connect(submitBtn, SIGNAL(clicked()), this, SLOT(submitIssue()));
        } else {
            submitBtn->setEnabled(false);
            submitBtn->setText("cUrl not found! Cannot send issues from here!");
            username->setEnabled(false);
            password->setEnabled(false);
            title->setEnabled(false);
        }
    }
    return widget_;
}



/** ***************************************************************************/
void IssueCreator::Extension::submitIssue() {
    if (!hasCurl_) {
        QMessageBox::critical(widget_, "No cUrl present!", "There was no cUrl found. Cannot submit issue!");
        return;
    }

    QString upw;
    upw.append(widget_->ui.usernameEdit->text())
            .append(":")
            .append(widget_->ui.passwordEdit->text());

    QJsonObject requestDataObject;
    requestDataObject.insert("title", widget_->ui.titleEdit->text());
    requestDataObject.insert("body", widget_->ui.issueText->toPlainText());

    QJsonDocument requestData(requestDataObject);

    QProcess submitProcess(this);
    submitProcess.setProgram("curl");
    submitProcess.setArguments({
                                   "--user",
                                   upw,
                                   "--data",
                                   requestData.toJson(),
                                   "https://api.github.com/repos/ManuelSchneid3r/albert/issues"
                                   // FOR TESTING PURPOSES "https://api.github.com/repos/idkCpp/Tests/issues"
                               });

    submitProcess.start();
    submitProcess.waitForFinished();

    QJsonDocument response = QJsonDocument::fromJson(submitProcess.readAllStandardOutput());
    QJsonObject obj = response.object();

    if (obj.contains("message")) {
        QMessageBox::critical(widget_, "GitHub reports:", obj.find("message").value().toString());
    } else if (obj.contains("number")) {
        widget_->ui.passwordEdit->setText("");
        widget_->ui.titleEdit->setText("");
        widget_->ui.issueText->setPlainText("");

        QString msg("You issue has number %1");
        QMessageBox::information(widget_, "GitHub reports:", msg.arg(obj.find("number").value().toInt()));
    }
}



/** ***************************************************************************/
void IssueCreator::Extension::handleQuery(shared_ptr<Query> query) {
    // This extension is only for its config widget.
    // It does not actually do anything
    // Avoid annoying warnings
    Q_UNUSED(query)
}
