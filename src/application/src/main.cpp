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
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>
#include <csignal>
#include "mainwindow.h"
#include "hotkeymanager.h"
#include "extensionmanager.h"
#include "querymanager.h"
#include "settingswidget.h"
#include "trayicon.h"
#include "trackbert.h"


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &message);
void shutdownHandler(int);
void dispatchMessage();


static QApplication           *app;
static QueryManager           *queryManager;
static MainWindow             *mainWindow;
static HotkeyManager          *hotkeyManager;
static SettingsWidget         *settingsWidget;
static TrayIcon               *trayIcon;
static QMenu                  *trayIconMenu;
static QLocalServer           *localServer;

int main(int argc, char **argv) {

    {
        bool showSettingsWhenInitialized = false;

        /*
         *  INITIALIZE APPLICATION
         */

        qInstallMessageHandler(myMessageOutput);

        app = new QApplication(argc, argv);
        app->setApplicationName("albert");
        app->setApplicationDisplayName("Albert");
        app->setApplicationVersion("v0.9.2");
        app->setQuitOnLastWindowClosed(false);
        QString icon = XdgIconLookup::iconPath("albert");
        if ( icon.isEmpty() ) icon = ":app_icon";
        app->setWindowIcon(QIcon(icon));



        /*
         *  PARSE COMMANDLINE
         */

        QCommandLineParser parser;
        parser.setApplicationDescription("Albert is still in alpha. These options may change in future versions.");
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addOption(QCommandLineOption({"k", "hotkey"}, "Overwrite the hotkey to use.", "hotkey"));
        parser.addOption(QCommandLineOption({"p", "plugin-dirs"}, "Set the plugin dirs to use. Comma separated.", "directory"));
        parser.addPositionalArgument("command", "Command to send to a running instance, if any. (show, hide, toggle)", "[command]");
        parser.process(*app);


        /*
         *  START IPC CLIENT
         */

        const QStringList args = parser.positionalArguments();
        QLocalSocket socket;
        socket.connectToServer(app->applicationName());
        if ( socket.waitForConnected(500) ) {
            // If there is a command send it
            if ( args.count() == 1 ){
                socket.write(args.at(0).toLocal8Bit());
                socket.flush();
                socket.waitForReadyRead(500);
                if (socket.bytesAvailable())
                    qDebug() << socket.readAll();
            }
            else
                qDebug("There is another instance of albert running.");
            socket.close();
            ::exit(EXIT_SUCCESS);
        } else if ( args.count() == 1 ) {
            qDebug("There is no other instance of albert running.");
            ::exit(EXIT_FAILURE);
        }


        /*
         *  INITIALIZE PATHS
         */

        // Make sure data, cache and config dir exists
        QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        QDir dir;
        dir.setPath(dataLocation);
        if (!dir.mkpath("."))
            qFatal("Could not create dir: %s",  dataLocation.toUtf8().constData());
        dir.setPath(cacheLocation);
        if (!dir.mkpath("."))
            qFatal("Could not create dir: %s",  cacheLocation.toUtf8().constData());

        // Move old config for user convenience TODO drop somewhen
        QFileInfo oldcfg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/albert/albert.conf");
        QFileInfo newcfg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/albert.conf");
        if (oldcfg.exists()){
            if (newcfg.exists())
                QFile::remove(newcfg.path());
            QFile::rename(oldcfg.filePath(), newcfg.filePath());
            oldcfg.dir().removeRecursively();
        }


        /*
         * DETECT FIRST RUN AND VERSION CHANGE
         */

        QString lastUsedVersion;
        QFile firstRunFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/firstrun");
        if ( firstRunFile.exists() ) {

            //Try to open the first run file and read the version
            if ( !firstRunFile.open(QIODevice::ReadWrite|QIODevice::Text) )
                qCritical() << qPrintable(QString("Could not open file %1: %2").arg(firstRunFile.fileName(), firstRunFile.errorString()));
            QTextStream(&firstRunFile) >> lastUsedVersion;

        } else { // This is the first run

            // Try to create a "firstRun"-file
            if ( !firstRunFile.open(QIODevice::WriteOnly|QIODevice::Text) )
                qCritical() << qPrintable(QString("Could not open file %1: %2").arg(firstRunFile.fileName(), firstRunFile.errorString()));

            // Give the user a possibility to set a hotkey on first run
            if ( QMessageBox(QMessageBox::Information, "First run",
                             "Seems like this is the first time you run Albert. "
                             "Most probably you want to set a hotkey to show "
                             "Albert. Do you want to open the settings dialog?",
                             QMessageBox::No|QMessageBox::Yes).exec() == QMessageBox::Yes )
                showSettingsWhenInitialized = true;
        }

        // Write the current version into the file
        QTextStream out(&firstRunFile);
        out << app->applicationVersion();
        firstRunFile.close();


        /*
         * INITIALIZE DATABASE
         */

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        if ( !db.isValid() )
            qFatal("No sqlite available");

        if (!db.driver()->hasFeature(QSqlDriver::Transactions))
            qFatal("QSqlDriver::Transactions not available.");

        db.setDatabaseName(QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)).filePath("core.db"));
        if (!db.open())
            qFatal("Unable to establish a database connection.");

        db.transaction();

        // Create tables
        QSqlQuery q;
        if (!q.exec("CREATE TABLE IF NOT EXISTS usages ( "
                    "  input TEXT NOT NULL, "
                    "  itemId TEXT NOT NULL, "
                    "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP "
                    ");"))
            qFatal("Unable to create table 'usages': %s", q.lastError().text().toUtf8().constData());

        if (!q.exec("CREATE TABLE IF NOT EXISTS runtimes ( "
                    "  extensionId TEXT NOT NULL, "
                    "  runtime INTEGER NOT NULL, "
                    "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP "
                    ");"))
            qFatal("Unable to create table 'runtimes': %s", q.lastError().text().toUtf8().constData());

        // Do regular cleanup
        if (!q.exec("DELETE FROM usages WHERE julianday('now')-julianday(timestamp)>90;"))
            qWarning("Unable to cleanup usages table.");

        if (!q.exec("DELETE FROM runtimes WHERE julianday('now')-julianday(timestamp)>7;"))
            qWarning("Unable to cleanup runtimes table.");

        db.commit();


        /*
         *  INITIALIZE APPLICATION COMPONENTS
         */

        ExtensionManager::instance = new Core::ExtensionManager;
        trayIcon         = new TrayIcon;
        trayIconMenu     = new QMenu;
        hotkeyManager    = new HotkeyManager;
        mainWindow       = new MainWindow;
        queryManager     = new QueryManager(ExtensionManager::instance);
        localServer      = new QLocalServer;


        /*
         *  START IPC SERVER
         */

        // Start server so second instances will close
         QLocalServer::removeServer(app->applicationName());
        localServer->listen(app->applicationName());
        QObject::connect(localServer, &QLocalServer::newConnection, dispatchMessage);


        /*
         * Build Tray Icon
         */

        QAction* showAction     = new QAction("Show", trayIconMenu);
        QAction* settingsAction = new QAction("Settings", trayIconMenu);
        QAction* quitAction     = new QAction("Quit", trayIconMenu);

        showAction->setIcon(app->style()->standardIcon(QStyle::SP_TitleBarMaxButton));
        settingsAction->setIcon(app->style()->standardIcon(QStyle::SP_FileDialogDetailedView));
        quitAction->setIcon(app->style()->standardIcon(QStyle::SP_TitleBarCloseButton));

        trayIconMenu->addAction(showAction);
        trayIconMenu->addAction(settingsAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(quitAction);

        trayIcon->setContextMenu(trayIconMenu);


        /*
         *  Hotkey
         */

        QSettings settings(qApp->applicationName());
        QString hotkey;

        // Check for a command line override
        if ( parser.isSet("hotkey") ) {
            hotkey = parser.value("hotkey");
            if ( !hotkeyManager->registerHotkey(hotkey) )
                qFatal("Failed to set hotkey to %s.", hotkey.toLocal8Bit().constData());

        // Check if the settings contains a hotkey entry
        } else if ( settings.contains("hotkey") ) {
            hotkey = settings.value("hotkey").toString();
            if ( !hotkeyManager->registerHotkey(hotkey) ){
                if ( QMessageBox(QMessageBox::Critical, "Error",
                                 QString("Failed to set hotkey: '%1'. Do you want to open the settings?").arg(hotkey),
                                 QMessageBox::No|QMessageBox::Yes).exec() == QMessageBox::Yes )
                    showSettingsWhenInitialized = true;
            }
        }


        /*
         *  MISC
         */

        // Quit gracefully on unix signals
        for ( int sig : { SIGINT, SIGTERM, SIGHUP, SIGPIPE } )
            signal(sig, shutdownHandler);

        // Print a message if the app was not terminated graciously
        QString filePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/running";
        if (QFile::exists(filePath)){
            qWarning() << "Application has not been terminated graciously.";
        } else {
            // Create the running indicator file
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly))
                qWarning() << "Could not create file:" << filePath;
            file.close();
        }

        // Check for a plugin override
        if ( parser.isSet("plugin-dirs") )
            Core::ExtensionManager::instance->setPluginDirs(parser.value("plugin-dirs").split(','));

        // Load extensions
        Core::ExtensionManager::instance->reloadExtensions();

        // Application is initialized create the settings widget
        settingsWidget   = new SettingsWidget(mainWindow, hotkeyManager, ExtensionManager::instance, trayIcon);

        // If somebody requested the settings dialog open it
        if ( showSettingsWhenInitialized )
            settingsWidget->show();


        /*
         * SIGNALING
         */

        QObject::connect(hotkeyManager, &HotkeyManager::hotKeyPressed,
                         mainWindow, &MainWindow::toggleVisibility);

        QObject::connect(queryManager, &QueryManager::resultsReady,
                         mainWindow, &MainWindow::setModel);

        QObject::connect(showAction, &QAction::triggered,
                         mainWindow, &MainWindow::show);

        QObject::connect(settingsAction, &QAction::triggered,
                         settingsWidget, &SettingsWidget::show);

        QObject::connect(settingsAction, &QAction::triggered,
                         settingsWidget, &SettingsWidget::raise);

        QObject::connect(quitAction, &QAction::triggered,
                         app, &QApplication::quit);

        QObject::connect(trayIcon, &TrayIcon::activated, [](QSystemTrayIcon::ActivationReason reason){
            if( reason == QSystemTrayIcon::ActivationReason::Trigger)
                mainWindow->toggleVisibility();
        });


        QObject::connect(mainWindow, &MainWindow::settingsWidgetRequested,
                         std::bind(&SettingsWidget::setVisible, settingsWidget, true));

        QObject::connect(mainWindow, &MainWindow::settingsWidgetRequested,
                         settingsWidget, &SettingsWidget::raise);

        QObject::connect(mainWindow, &MainWindow::widgetShown,
                         queryManager, &QueryManager::setupSession);

        QObject::connect(mainWindow, &MainWindow::widgetHidden,
                         queryManager, &QueryManager::teardownSession);

        QObject::connect(mainWindow, &MainWindow::inputChanged,
                         queryManager, &QueryManager::startQuery);

    }


    /*
     * ENTER EVENTLOOP
     */
    memstat_t mem = allocatedMemory();
qDebug("Entering event loop! Current memory %f MiB + %.02f %% overhead", (float)mem.payloadSize / 1024 / 1024, mem.overheadRatio * 100);
    int retval = app->exec();
    mem = allocatedMemory();
qDebug("Exited event loop! Current memory %f MiB + %.02f %% overhead", (float)mem.payloadSize / 1024 / 1024, mem.overheadRatio * 100);


    /*
     *  FINALIZE APPLICATION
     */

    delete settingsWidget;
    delete trayIconMenu;
    delete trayIcon;
    delete queryManager;
    delete hotkeyManager;
    delete mainWindow;
    delete ExtensionManager::instance;

    // Delete the running indicator file
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+"/running");

    return retval;
}


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



/** ***************************************************************************/
void shutdownHandler(int) {
    QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
}



/** ***************************************************************************/
void dispatchMessage() {
    QLocalSocket* socket = localServer->nextPendingConnection(); // Should be safe
    socket->waitForReadyRead(500);
    if (socket->bytesAvailable()) {
        QString msg = QString::fromLocal8Bit(socket->readAll());
        if ( msg == "show") {
            mainWindow->setVisible(true);
            socket->write("Application set visible.");
        } else if ( msg == "hide") {
            mainWindow->setVisible(false);
            socket->write("Application set invisible.");
        } else if ( msg == "toggle") {
            mainWindow->toggleVisibility();
            socket->write("Visibility toggled.");
        } else
            socket->write("Command not supported.");
    }
    socket->flush();
    socket->close();
    socket->deleteLater();
}
