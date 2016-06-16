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

#include <QProcess>
#include <QIcon>
#include <QDebug>
#include "extension.h"
#include "configwidget.h"
#include "query.h"
#include "objects.hpp"
#include "xdgiconlookup.h"
#include "albertapp.h"


namespace {

    vector<QString> configNames = {
        "lock",
        "logout",
        "suspend",
        "hibernate",
        "reboot",
        "shutdown"
    };

    vector<QString> itemTitles = {
        "Lock",
        "Logout",
        "Suspend",
        "Hibernate",
        "Reboot",
        "Shutdown"
    };

    vector<QString> itemDescriptions = {
        "Lock the session.",
        "Quit the session.",
        "Suspend the machine.",
        "Hibernate the machine.",
        "Reboot the machine.",
        "Shutdown the machine.",
    };

    vector<QString> iconNames = {
        "system-lock-screen",
        "system-log-out",
        "system-suspend",
        "system-suspend-hibernate",
        "system-reboot",
        "system-shutdown"
    };
}

/** ***************************************************************************/
System::Extension::Extension() : IExtension("System") {
    qDebug("[%s] Initialize extension", name_);

    // Load settings
    QString themeName = QIcon::themeName();
    qApp->settings()->beginGroup(name_);
    for (int i = 0; i < NUMCOMMANDS; ++i) {
        iconPaths.push_back(XdgIconLookup::instance()->themeIconPath(iconNames[i], themeName));
        commands.push_back(qApp->settings()->value(configNames[i], defaultCommand(static_cast<SupportedCommands>(i))).toString());
    }

    qDebug("[%s] Extension initialized", name_);
}



/** ***************************************************************************/
QWidget *System::Extension::widget(QWidget *parent) {
    if (widget_.isNull()) {
        widget_ = new ConfigWidget(parent);
/*
        // Initialize the content and connect the signals
        AUTOCONF_SETUP(widget_);

        AUTOCONF(widget_->ui.lineEdit_lock,      configNames[LOCK],      commands[LOCK],      commands[LOCK]);
        AUTOCONF(widget_->ui.lineEdit_logout,    configNames[LOGOUT],    commands[LOGOUT],    commands[LOGOUT]);
        AUTOCONF(widget_->ui.lineEdit_suspend,   configNames[SUSPEND],   commands[SUSPEND],   commands[SUSPEND]);
        AUTOCONF(widget_->ui.lineEdit_hibernate, configNames[HIBERNATE], commands[HIBERNATE], commands[HIBERNATE]);
        AUTOCONF(widget_->ui.lineEdit_reboot,    configNames[REBOOT],    commands[REBOOT],    commands[REBOOT]);
        AUTOCONF(widget_->ui.lineEdit_shutdown,  configNames[POWEROFF],  commands[POWEROFF],  commands[POWEROFF]);
*/
    }
    return widget_;
}



/** ***************************************************************************/
void System::Extension::handleQuery(shared_ptr<Query> query) {

    for (int i = 0; i < NUMCOMMANDS; ++i) {
        if (configNames[i].startsWith(query->searchTerm().toLower())) {
            QString cmd = commands[i];
            query->addMatch(std::make_shared<StandardItem>(
                                itemTitles[i],
                                itemDescriptions[i],
                                iconPaths[i],
                                [cmd](){QProcess::startDetached(cmd);}
            ));
        }
    }
}



/** ***************************************************************************/
QString System::Extension::defaultCommand(SupportedCommands command){

    QString de = getenv("XDG_CURRENT_DESKTOP");

    switch (command) {
    case LOCK:
        if (de == "X-Cinnamon")
            return "cinnamon-screensaver-command -l";
        else if (de == "MATE")
            return "mate-screensaver-command --lock";
        else
            return "notify-send \"Error.\" \"Lock command is not set.\" --icon=system-lock-screen";

    case LOGOUT:
        if (de == "Unity" || de == "Pantheon" || de == "Gnome")
            return "gnome-session-quit --logout";
        else if (de == "kde-plasma")
            return "qdbus org.kde.ksmserver /KSMServer logout 0 0 0";
        else if (de == "X-Cinnamon")
            return "cinnamon-session-quit --logout --no-prompt";
        else if (de == "XFCE")
            return "gnome-session-quit --logout --no-prompt";
        else if (de == "MATE")
            return "mate-session-save --logout";
        else
            return "notify-send \"Error.\" \"Logout command is not set.\" --icon=system-log-out";

    case SUSPEND:
        if (de == "XFCE")
            return "xfce4-session-logout --suspend";
        else if (de == "MATE")
            return "sh -c \"mate-screensaver-command --lock && systemctl suspend -i\"";
        else
            return "systemctl suspend -i";

    case HIBERNATE:
        if (de == "XFCE")
            return "xfce4-session-logout --hibernate";
        else if (de == "MATE")
            return "sh -c \"mate-screensaver-command --lock && systemctl hibernate -i\"";
        else
            return "systemctl hibernate -i";

    case REBOOT:
        if (de == "Unity" || de == "Pantheon" || de == "Gnome")
            return "gnome-session-quit --reboot";
        else if (de == "kde-plasma")
            return "qdbus org.kde.ksmserver /KSMServer logout 0 1 0";
        else if (de == "X-Cinnamon")
            return "cinnamon-session-quit --reboot";
        else if (de == "XFCE")
            return "xfce4-session-logout --reboot";
        else if (de == "MATE")
            return "mate-session-save --shutdown-dialog";
        else
            return "notify-send \"Error.\" \"Reboot command is not set.\" --icon=system-reboot";

    case POWEROFF:
        if (de == "Unity" || de == "Pantheon" || de == "Gnome")
            return "gnome-session-quit --power-off --no-prompt";
        else if (de == "kde-plasma")
            return "qdbus org.kde.ksmserver /KSMServer logout 0 2 0";
        else if (de == "X-Cinnamon")
            return "cinnamon-session-quit --power-off --no-prompt";
        else if (de == "XFCE")
            return "xfce4-session-logout --halt";
        else if (de == "MATE")
            return "mate-session-save --shutdown-dialog";
        else
            return "notify-send \"Error.\" \"Poweroff command is not set.\" --icon=system-shutdown";

    case NUMCOMMANDS:
        // NEVER REACHED;
        return "";
    }

    // NEVER REACHED;
    return "";
}
