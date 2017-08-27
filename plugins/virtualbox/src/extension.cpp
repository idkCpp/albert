// albert - a simple application launcher for linux
// Copyright (C) 2014-2015 Manuel Schneider
// Contributed to by 2016-2017 Martin Buergmann
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
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileSystemWatcher>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <functional>
#include "extension.h"
#include "vm.h"
#include "core/query.h"
#include "util/standarditem.h"
#include "util/standardaction.h"
#include "xdg/iconlookup.h"
using Core::Action;
using Core::StandardAction;
using Core::StandardItem;

#define RT_OS_LINUX 1

#include "nsMemory.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsEventQueueUtils.h"

#include "nsIExceptionService.h"

#include "VirtualBox_XPCOM.h"


class VirtualBox::Private
{
public:

    QList<VM*> vms;
    QFileSystemWatcher vboxWatcher;

    void rescanVBoxConfig(QString path);

    // SDK stuff
    nsCOMPtr<nsIServiceManager> serviceManager;
    nsCOMPtr<nsIEventQueue> eventQ;
    nsCOMPtr<nsIComponentManager> manager;
    nsCOMPtr<IVirtualBox> virtualBox;

};



/** ***************************************************************************/
void VirtualBox::Private::rescanVBoxConfig(QString path) {

    qInfo() << "Start indexing VirtualBox images.";

    QFile vboxConfigFile(path);
    if (!vboxConfigFile.exists())
        return;
    if (!vboxConfigFile.open(QFile::ReadOnly)) {
        qCritical() << "Could not open VirtualBox config file for read operation!";
        return;
    }

    QDomDocument vboxConfig;
    QString errMsg = "";
    int errLine = 0, errCol = 0;
    if (!vboxConfig.setContent(&vboxConfigFile, &errMsg, &errLine, &errCol)) {
        qWarning() << qPrintable(QString("Parsing VBox config failed because %s in line %d col %d").arg(errMsg).arg(errLine, errCol));
        vboxConfigFile.close();
        return;
    }
    vboxConfigFile.close();

    QDomElement root = vboxConfig.documentElement();
    if (root.isNull()) {
        qCritical() << "In VBox config file: Root element is null.";
        return;
    }

    QDomElement global = root.firstChildElement("Global");
    if (global.isNull()) {
        qCritical() << "In VBox config file: Global element is null.";
        return;
    }

    QDomElement machines = global.firstChildElement("MachineRegistry");  // List of MachineEntry
    if (machines.isNull()) {
        qCritical() << "In VBox config file: Machine registry element is null.";
        return;
    }

    // With this we iterate over the machine entries
    QDomElement machine = machines.firstChildElement();

    // And we count how many entries we find for information reasons
    int found = 0;

    qDeleteAll(vms);
    vms.clear();

    while (!machine.isNull()) {

        vms.append(new VM(machine.attribute("src")));

        machine = machine.nextSiblingElement();
        found++;
    }

    qInfo() << qPrintable(QString("Indexed %2 VirtualBox images.").arg(found));
}



/** ***************************************************************************/
/** ***************************************************************************/
/** ***************************************************************************/
/** ***************************************************************************/
VirtualBox::Extension::Extension()
    : Core::Extension("org.albert.extension.virtualbox"),
      Core::QueryHandler(Core::Extension::id),
      d(new Private) {

    /*
     * Check that PRUnichar is equal in size to what compiler composes L""
     * strings from; otherwise NS_LITERAL_STRING macros won't work correctly
     * and we will get a meaningless SIGSEGV. This, of course, must be checked
     * at compile time in xpcom/string/nsTDependentString.h, but XPCOM lacks
     * compile-time assert macros and I'm not going to add them now.
     */
    if (sizeof(PRUnichar) != sizeof(wchar_t))
    {
        throw new QString("Error compiler size mismatch!");
        /*
        printf("Error: sizeof(PRUnichar) {%lu} != sizeof(wchar_t) {%lu}!\n"
               "Probably, you forgot the -fshort-wchar compiler option.\n",
               (unsigned long) sizeof(PRUnichar),
               (unsigned long) sizeof(wchar_t));
        return -1;*/
    }

    nsresult rc;

    /*
     * This is the standard XPCOM init procedure.
     * What we do is just follow the required steps to get an instance
     * of our main interface, which is IVirtualBox.
     *
     * Note that we scope all nsCOMPtr variables in order to have all XPCOM
     * objects automatically released before we call NS_ShutdownXPCOM at the
     * end. This is an XPCOM requirement.
     */
    rc = NS_InitXPCOM2(getter_AddRefs(d->serviceManager), nsnull, nsnull);
    if (NS_FAILED(rc))
    {
        throw QString("Error: XPCOM could not be initialized! rc=%1\n").arg(rc);
    }

    /*
     * Make sure the main event queue is created. This event queue is
     * responsible for dispatching incoming XPCOM IPC messages. The main
     * thread should run this event queue's loop during lengthy non-XPCOM
     * operations to ensure messages from the VirtualBox server and other
     * XPCOM IPC clients are processed. This use case doesn't perform such
     * operations so it doesn't run the event loop.
     */
    rc = NS_GetMainEventQ(getter_AddRefs(d->eventQ));
    if (NS_FAILED(rc))
    {
        throw QString("Error: could not get main event queue! rc=%1\n").arg(rc);
    }

    /*
     * Now XPCOM is ready and we can start to do real work.
     * IVirtualBox is the root interface of VirtualBox and will be
     * retrieved from the XPCOM component manager. We use the
     * XPCOM provided smart pointer nsCOMPtr for all objects because
     * that's very convenient and removes the need deal with reference
     * counting and freeing.
     */
    rc = NS_GetComponentManager(getter_AddRefs(d->manager));
    if (NS_FAILED(rc))
    {
        throw QString("Error: could not get component manager! rc=%1\n").arg(rc);
    }

    rc = d->manager->CreateInstanceByContractID(NS_VIRTUALBOX_CONTRACTID,
                                             nsnull,
                                             NS_GET_IID(IVirtualBox),
                                             getter_AddRefs(d->virtualBox));
    if (NS_FAILED(rc))
    {
        throw QString("Error, could not instantiate VirtualBox object! rc=%1\n").arg(rc);
    }
    printf("VirtualBox object created\n");

    VMItem::iconPath_ = XDG::IconLookup::iconPath("virtualbox");
    if ( VMItem::iconPath_.isNull() )
        VMItem::iconPath_ = ":vbox";

    QString vboxConfigPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, "VirtualBox/VirtualBox.xml");
    if (vboxConfigPath.isEmpty())
        throw "VirtualBox was not detected!";

    d->rescanVBoxConfig(vboxConfigPath);
    d->vboxWatcher.addPath(vboxConfigPath);
    connect(&d->vboxWatcher, &QFileSystemWatcher::fileChanged,
            std::bind(&Private::rescanVBoxConfig, d.get(), std::placeholders::_1));
}



/** ***************************************************************************/
VirtualBox::Extension::~Extension() {
    /*
     * Perform the standard XPCOM shutdown procedure.
     */
    NS_ShutdownXPCOM(nsnull);
}



/** ***************************************************************************/
QWidget *VirtualBox::Extension::widget(QWidget *parent) {
    return new QWidget(parent);
}



/** ***************************************************************************/
void VirtualBox::Extension::setupSession() {
    for (VM *vm : d->vms)
        vm->probeState();
}



/** ***************************************************************************/
void VirtualBox::Extension::handleQuery(Core::Query * query) {

    if ( query->searchTerm().isEmpty() )
        return;

    for (VM* vm : d->vms) {
        if (vm->startsWith(query->searchTerm()))
            query->addMatch(std::shared_ptr<Item>(vm->produceItem())); // Implicit move
    }
}
