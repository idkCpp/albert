// albert - a simple application launcher for linux
// Copyright (C) 2017 Martin Buergmann
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

#include "trackbert.h"
#include <QMutex>
#include <QMutexLocker>
using std::size_t;

typedef struct {
    char magic;
    size_t len;
} housekeeping_t;

static constexpr size_t extra = sizeof(housekeeping_t);
static QMutex newtex;
static size_t allocatedPayload = 0;
static size_t allocatedOverhead = 0;

void* operator new (size_t sz) { //throw(std::bad_alloc) {
    QMutexLocker _(&newtex);
    char* sp = (char*) malloc(sz + extra);

    allocatedPayload += sz;
    allocatedOverhead += extra;

    housekeeping_t *hk = (housekeeping_t*)sp;
    hk->magic = 0x55;
    hk->len = sz;

    return sp + extra;
}
void operator delete(void* p) throw() {
    if (p == nullptr)
        return;
    QMutexLocker _(&newtex);
    char* sp = (char*)p - extra;
    housekeeping_t *hk = (housekeeping_t*) sp;

    if (hk->magic == 0x55) {
        allocatedPayload -= hk->len;
        allocatedOverhead -= extra;
    }

    free(sp);
}

memstat_t allocatedMemory() {
    QMutexLocker _(&newtex);
    memstat_t ret;
    ret.overheadSize = allocatedOverhead;
    ret.payloadSize = allocatedPayload;
    ret.overheadRatio = (float)allocatedOverhead / (allocatedOverhead+allocatedPayload);
    return ret;
}
