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

#pragma once

#include <memory>

typedef struct {
    std::size_t payloadSize;
    std::size_t overheadSize;
    float overheadRatio;
} memstat_t;

typedef struct {
    std::size_t allocated = 0;
} tracker_t;

memstat_t allocatedMemory();
void* operator new (size_t, tracker_t&);
