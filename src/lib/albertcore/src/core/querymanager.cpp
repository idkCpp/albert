// albert - a simple application launcher for linux
// Copyright (C) 2014-2017 Manuel Schneider
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
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <chrono>
#include <vector>
#include "extension.h"
#include "extensionmanager.h"
#include "item.h"
#include "matchcompare.h"
#include "queryexecution.h"
#include "querymanager.h"
#include "queryhandler.h"
#include "fallbackprovider.h"
using namespace Core;
using std::set;
using std::chrono::system_clock;
using std::vector;
using std::shared_ptr;

/** ***************************************************************************/
Core::QueryManager::QueryManager(ExtensionManager* em, QObject *parent)
    : QObject(parent),
      extensionManager_(em),
      currentQuery_(nullptr) {

    // Initialize the order
    Core::MatchCompare::update();
}



/** ***************************************************************************/
void Core::QueryManager::setupSession() {

    qDebug() << "========== SESSION SETUP STARTED ==========";

    system_clock::time_point start = system_clock::now();

    // Call all setup routines
    for (Core::QueryHandler *handler : extensionManager_->objectsByType<Core::QueryHandler>()){
        system_clock::time_point start = system_clock::now();
        handler->setupSession();
        long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
        qDebug() << qPrintable(QString("TIME: %1 µs SESSION SETUP [%2]").arg(duration, 6).arg(handler->id));
    }

    long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
    qDebug() << qPrintable(QString("TIME: %1 µs SESSION SETUP OVERALL").arg(duration, 6));
}



/** ***************************************************************************/
void Core::QueryManager::teardownSession() {

    qDebug() << "========== SESSION TEARDOWN STARTED ==========";

    system_clock::time_point start = system_clock::now();

    // Call all teardown routines
    for (Core::QueryHandler *handler : extensionManager_->objectsByType<Core::QueryHandler>()){
        system_clock::time_point start = system_clock::now();
        handler->teardownSession();
        long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
        qDebug() << qPrintable(QString("TIME: %1 µs SESSION TEARDOWN [%2]").arg(duration, 6).arg(handler->id));
    }

    // Open database to store the runtimes
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery sqlQuery;
    db.transaction();

    // Delete finished queries and prepare sql transaction containing runtimes
    vector<QueryExecution*>::iterator it = pastQueries_.begin();
    while ( it != pastQueries_.end()){
        if ( (*it)->state() != QueryExecution::State::Running ) {

            // Store the runtimes
            for ( const std::pair<QString,uint> &handlerRuntime : (*it)->runtimes() ) {
                sqlQuery.prepare("INSERT INTO runtimes (extensionId, runtime) VALUES (:extensionId, :runtime);");
                sqlQuery.bindValue(":extensionId", handlerRuntime.first);
                sqlQuery.bindValue(":runtime", handlerRuntime.second);
                if (!sqlQuery.exec())
                    qWarning() << sqlQuery.lastError();
            }

            // Delete the query
            (*it)->deleteLater();
            it = pastQueries_.erase(it);
        } else
            ++it;
    }

    // Finally send the sql transaction
    db.commit();

    // Compute new match rankings
    Core::MatchCompare::update();

    long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
    qDebug() << qPrintable(QString("TIME: %1 µs SESSION TEARDOWN OVERALL").arg(duration, 6));
}



/** ***************************************************************************/
void Core::QueryManager::startQuery(const QString &searchTerm) {

    qDebug() << "========== QUERY:" << searchTerm << " ==========";

    if ( currentQuery_ != nullptr ) {
        // Stop last query
        disconnect(currentQuery_, &QueryExecution::resultsReady,
                   this, &QueryManager::resultsReady);
        currentQuery_->cancel();
        // Store for later deletion (listview still has the model)
        pastQueries_.push_back(currentQuery_);
    }

    // Do nothing if nothing is loaded
    if ( extensionManager_->objects().empty() )
        return;

    system_clock::time_point start = system_clock::now();

    // Start query
    currentQuery_ = new QueryExecution(extensionManager_->objectsByType<QueryHandler>(),
                                       extensionManager_->objectsByType<FallbackProvider>(),
                                       searchTerm);
    connect(currentQuery_, &QueryExecution::resultsReady, this, &QueryManager::resultsReady);
    currentQuery_->run();

    connect(currentQuery_, &QueryExecution::stateChanged, [start](QueryExecution::State state){
        if ( state == QueryExecution::State::Finished ) {
            long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
            qDebug() << qPrintable(QString("TIME: %1 µs QUERY OVERALL").arg(duration, 6));
        }
    });


    long duration = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now()-start).count();
    qDebug() << qPrintable(QString("TIME: %1 µs SESSION TEARDOWN OVERALL").arg(duration, 6));
}