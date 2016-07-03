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

#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QDebug>
#include <csignal>
#include <bitset>
#include "albertapp.h"
#include "mainwindow.h"
#include "settingswidget.h"
#include "hotkeymanager.h"
#include "pluginmanager.h"
#include "extensionmanager.h"

namespace {

/** ***************************************************************************/
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    QString suffix;
    if (context.function)
        suffix = QString("  --  [%1]").arg(context.function);
    switch (type) {
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
#endif
    case QtDebugMsg:
        fprintf(stderr, "%s\n", message.toLocal8Bit().constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "\x1b[33;1mWarning:\x1b[0;1m %s%s\x1b[0m\n", message.toLocal8Bit().constData(), suffix.toLocal8Bit().constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "\x1b[31;1mCritical:\x1b[0;1m %s%s\x1b[0m\n", message.toLocal8Bit().constData(), suffix.toLocal8Bit().constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "\x1b[41;30;4mFATAL:\x1b[0;1m %s%s\x1b[0m\n", message.toLocal8Bit().constData(), suffix.toLocal8Bit().constData());
        exit(1);
    }
}

void shutdownHandler(int) {
    QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}
}

const char* AlbertApp::CFG_TERM = "terminal";
const char* AlbertApp::DEF_TERM = "xterm -e %1";

/** ***************************************************************************/
AlbertApp::AlbertApp(int &argc, char *argv[]) : QApplication(argc, argv) {

    /*
     *  INITIALIZE APPLICATION
     */

    qInstallMessageHandler(myMessageOutput);
    setOrganizationDomain("albert");
    setApplicationName("albert");
    setApplicationDisplayName("Albert");
    setApplicationVersion("v0.8.10");
    setWindowIcon(QIcon(":app_icon"));
    setQuitOnLastWindowClosed(false);


    /*
     *  SETUP PARSER FOR COMMANDLINE
     */

    QCommandLineParser parser;
    parser.setApplicationDescription("Albert is still in alpha. These options "
                                     "may change in future versions.");
    parser.addHelpOption();
    parser.addVersionOption();

    // Config options
    QCommandLineOption configOption = QCommandLineOption({"c", "config"}, "The config file to use.", "file");
    QCommandLineOption hotkeyOption = QCommandLineOption({"k", "hotkey"}, "Overwrite the hotkey to use.", "hotkey");

    // IPC options
    QCommandLineOption quitOption = QCommandLineOption({"q", "quit"}, "Shuts albert down.");
    QCommandLineOption showOption = QCommandLineOption({"s", "show"}, "Brings the albert widget to front.");
    QCommandLineOption hideOption = QCommandLineOption({"i", "hide"}, "Hides the albert widget."); // "i" for invisible because "h" is already taken by the help option
    QCommandLineOption toggleOption = QCommandLineOption({"t", "toggle"}, "Brings the albert widget to front if its hidden, hides it otherwise.");
    QCommandLineOption reloadOption = QCommandLineOption({"r", "reload"}, "Reloads the configuration");
    QString ipcOpts = "sitqr";

    parser.addOption(configOption);
    parser.addOption(hotkeyOption);
    parser.addOption(quitOption);
    parser.addOption(showOption);
    parser.addOption(hideOption);
    parser.addOption(toggleOption);
    parser.addOption(reloadOption);


    /*
     *  PARSE
     */

    parser.process(*this);

    const QStringList args = parser.positionalArguments();
    if ( args.count() > 0) {
        qFatal(parser.helpText().toStdString().c_str());
    }

    bool settingsAlternateConfigFile = parser.isSet(configOption);
    bool settingsChangeHotkey = parser.isSet(hotkeyOption);

    bool ipcBringToFront = parser.isSet(showOption);
    bool ipcHide = parser.isSet(hideOption);
    bool ipcToggle = parser.isSet(toggleOption);
    bool ipcQuit = parser.isSet(quitOption);
    bool ipcReloadSettings = parser.isSet(reloadOption);

    bool anySettingsOpt = settingsAlternateConfigFile | settingsChangeHotkey;
    bool anyIpcOpt = ipcBringToFront | ipcHide | ipcQuit | ipcToggle | ipcReloadSettings;


    /*
     *  CHECK FOR INCOMPATIBLE OPTIONS
     */

    if ( (ipcHide && ipcBringToFront) ||
            (ipcHide && ipcToggle) ||
            (ipcBringToFront && ipcToggle) ) {

        qWarning("Please specify only one of -sit");

        // Clear all because we don't know what the user wants
        ipcHide = false;
        ipcToggle = false;
        ipcBringToFront = false;
        anyIpcOpt = ipcQuit | ipcReloadSettings;
    }

    if ((ipcHide || ipcBringToFront || ipcToggle || ipcReloadSettings) && ipcQuit) {
        qWarning("Ignoring all of -sitr");

        // Clear all the show/hide/toggle opts because we want to --quit
        ipcHide = false;
        ipcToggle = false;
        ipcBringToFront = false;
        ipcReloadSettings = false;
    }


    /*
     *  CHECK FOR SINGLE INSTANCE / IPC
     */

    QLocalSocket socket;
    socket.connectToServer(applicationName());
    bool albertAlreadyRunning = socket.waitForConnected(500);

    if (anyIpcOpt && !albertAlreadyRunning) {
        if (anySettingsOpt) {
            QString err = "No albert program is running. -%1 options ignored!";
            err = err.arg(ipcOpts);
            std::string stderrstr = err.toStdString(); // Making this a local variable to give a specific scope in which the var exists
            qWarning(stderrstr.c_str());

            anyIpcOpt = false;
        } else
            qFatal("There is no albert instance running!");
    }


    /*
     *  CARRY OUT SETTINGS MANIPULATION
     */

    if (anySettingsOpt) {
        if (albertAlreadyRunning) {
            qCritical("You are modifying settings while albert is running. This can lead to data loss!");
            ipcReloadSettings = true;
            anyIpcOpt = true;
        }

        if ( parser.isSet(configOption) ) {
            settings_ = new QSettings(parser.value(configOption));
        } else {
            settings_ = new QSettings;
        }
        if ( parser.isSet(hotkeyOption) ) {
            settings_->setValue("hotkey", parser.value(hotkeyOption));
        }
    } else
        settings_ = new QSettings; // Have to make sure settings object is present even if no settings opt was given



    if (anyIpcOpt) {
        // If there is a command send it

        QString cmdToSend;

        if (ipcHide)
            cmdToSend = "hide";
        if (ipcBringToFront)
            cmdToSend = "show";
        if (ipcToggle)
            cmdToSend = "toggle";
        if (ipcQuit)
            cmdToSend = "quit";
        if (ipcReloadSettings) {
            if (parser.isSet(configOption))
                cmdToSend = "reload " + parser.value(configOption);
            else
                cmdToSend = "reload";
        }

        if ( !cmdToSend.isEmpty() ){
            socket.write(cmdToSend.toLocal8Bit());
            socket.flush();
            while (socket.waitForReadyRead(500)) // In case multiple lines arrive (within half a sec)
                if (socket.bytesAvailable())
                    qDebug() << socket.readAll();
        }
        // else miracle();

        socket.close();
        ::exit(EXIT_SUCCESS);
    }

    // Start server so second instances will close
    QLocalServer::removeServer(applicationName());
    localServer_ = new QLocalServer(this);
    localServer_->listen(applicationName());
    QObject::connect(localServer_, &QLocalServer::newConnection, [this] () {
        QLocalSocket* socket = localServer_->nextPendingConnection(); // Should be safe
        socket->waitForReadyRead(500);
        if (socket->bytesAvailable()) {
            QString msg = QString::fromLocal8Bit(socket->readAll());
            if ( msg == "show") {
                mainWindow_->show();
                socket->write("Application set visible.");
            } else if ( msg == "hide") {
                mainWindow_->hide();
                socket->write("Application set invisible.");
            } else if ( msg == "toggle") {
                mainWindow_->setVisible(!mainWindow_->isVisible());
                socket->write("Visibility toggled.");
            } else if ( msg == "quit" ) {
                socket->write("Shutting down!");
                quit();
            } else if ( msg.startsWith("reload") ) {
                socket->write("Attempting reload");
                socket->flush();
                bool fileChanged = false;
                QString settingsFile;
                if (msg.contains(" ")) {
                    QStringList split = msg.split(" ");
                    split.removeFirst();
                    settingsFile = split.join(" "); // Join should not be needed but sure is sure. Who knows...
                    fileChanged = true;
                } else
                    settingsFile = settings_->fileName();
                delete extensionManager_;
                delete pluginManager_;
                if (fileChanged) {
                    delete settings_;
                    settings_ = new QSettings(settingsFile);
                } else {
                    settings_->sync();
                }
                pluginManager_ = new PluginManager;
                extensionManager_ = new ExtensionManager;
                socket->write("Reloaded!");
            } else
                socket->write("Command not supported.");
        }
        socket->flush();
        socket->close();
        socket->deleteLater();
    });


    /*
     *  UNIX STUFF THAT SHOULD BE IN A PIMPL OR SUBCLASS
     */

    // Set terminal emulator
    QVariant v = qApp->settings()->value(CFG_TERM);
    if (v.isValid() && v.canConvert(QMetaType::QString))
        terminal_ = v.toString();
    else{
        terminal_ = getenv("TERM");
        if (terminal_.isEmpty())
            terminal_ = DEF_TERM;
        else
            terminal_.append(" -e %1");
    }

    // Quit gracefully on unix signals
    for ( int sig : {SIGINT, SIGTERM, SIGHUP} )
        signal(sig, shutdownHandler);

    // Print e message if the app was not terminated graciously
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/running";
    if (QFile::exists(filePath)){
        qCritical() << "Application has not been terminated graciously.";
        if (qApp->settings()->value("warnAboutNonGraciousQuit") != false){
            QMessageBox msgBox(QMessageBox::Critical, "Error",
                               "Albert has not been quit graciously! This "
                               "means your settings and data have not been "
                               "saved. If you did not kill albert yourself, "
                               "albert most likely crashed. Please report this "
                               "on github. Do you want to ignore this warnings "
                               "in future?",
                               QMessageBox::Yes|QMessageBox::No);
            msgBox.exec();
            if ( msgBox.result() == QMessageBox::Yes )
                qApp->settings()->setValue("warnAboutNonGraciousQuit", false);
        }
    } else {
        // Create the running indicator file
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
            qCritical() << "Could not create file:" << filePath;
        file.close();
    }


    /*
     * INITIALIZE PATHS
     */

    // Make sure data, cache and config dir exists
    QDir dir;
    dir.setPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/" + applicationName());
    dir.mkpath(".");
    dir.setPath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    dir.mkpath(".");
    dir.setPath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    dir.mkpath(".");


    /*
     * INITIALIZE MODULES
     */

    mainWindow_ = new MainWindow;
    hotkeyManager_ = new HotkeyManager;
    extensionManager_ = new ExtensionManager;
    pluginManager_ = new PluginManager;

    // toggle visibility
    QObject::connect(hotkeyManager_, &HotkeyManager::hotKeyPressed,[this](){
        mainWindow_->setVisible(!mainWindow_->isVisible());
    });

    QObject::connect(mainWindow_, &MainWindow::widgetShown,
                     extensionManager_, &ExtensionManager::setupSession);

    QObject::connect(mainWindow_, &MainWindow::widgetHidden,
                     extensionManager_, &ExtensionManager::teardownSession);

    QObject::connect(mainWindow_, &MainWindow::startQuery,
                     extensionManager_, &ExtensionManager::startQuery);

    QObject::connect(extensionManager_, &ExtensionManager::newModel,
                     mainWindow_, &MainWindow::setModel);

    QObject::connect(pluginManager_, &PluginManager::pluginLoaded,
                     extensionManager_, &ExtensionManager::registerExtension);

    QObject::connect(pluginManager_, &PluginManager::pluginAboutToBeUnloaded,
                     extensionManager_, &ExtensionManager::unregisterExtension);

    // Propagade the extensions once TODO this is bullshitty design
    for (const unique_ptr<PluginSpec> &p : pluginManager_->plugins())
        if (p->isLoaded())
            extensionManager_->registerExtension(p->instance());
}



/** ***************************************************************************/
AlbertApp::~AlbertApp() {

    /*
     *  FINALIZE APPLICATION
     */
    // Unload the plugins
    delete extensionManager_;
    delete pluginManager_;
    delete hotkeyManager_;
    delete mainWindow_;

    // Delete the running indicator file
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/running");
}



/** ***************************************************************************/
int AlbertApp::exec() {
    //  HOTKEY  //  Albert without hotkey is useless. Force it!
    QVariant v;
    if (!(qApp->settings()->contains("hotkey")
          && (v=qApp->settings()->value("hotkey")).canConvert(QMetaType::QString)
          && hotkeyManager_->registerHotkey(v.toString()))){
        QMessageBox msgBox(QMessageBox::Critical, "Error",
                           "Hotkey is not set or invalid. Press ok to open "
                           "the settings or press close to quit albert.",
                           QMessageBox::Close|QMessageBox::Ok);
        msgBox.exec();
        if ( msgBox.result() == QMessageBox::Ok ) {
            //hotkeyManager->disable();
            openSettings();
            //QObject::connect(settingsWidget, &QWidget::destroyed, hotkeyManager, &HotkeyManager::enable);
        }
        else
            return EXIT_FAILURE;
    }
    return QApplication::exec();
}



/** ***************************************************************************/
void AlbertApp::openSettings() {
    if (!settingsWidget_)
        settingsWidget_ = new SettingsWidget(mainWindow_, hotkeyManager_, pluginManager_);
    settingsWidget_->show();
    settingsWidget_->raise();
}



/** ***************************************************************************/
void AlbertApp::showWidget() {
    mainWindow_->show();
}



/** ***************************************************************************/
void AlbertApp::hideWidget() {
    mainWindow_->hide();
}



/** ***************************************************************************/
void AlbertApp::clearInput() {
    mainWindow_->setInput("");
}



/** ***************************************************************************/
QSettings *AlbertApp::settings() {
    return settings_;
}



/** ***************************************************************************/
QString AlbertApp::term(){
    return terminal_;
}



/** ***************************************************************************/
void AlbertApp::setTerm(const QString &terminal){
    qApp->settings()->setValue(CFG_TERM, terminal);
    terminal_ = terminal;
}
