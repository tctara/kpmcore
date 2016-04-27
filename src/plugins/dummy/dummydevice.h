/*************************************************************************
 *  Copyright (C) 2010 by Volker Lanz <vl@fidra.de>                      *
 *                                                                       *
 *  This program is free software; you can redistribute it and/or        *
 *  modify it under the terms of the GNU General Public License as       *
 *  published by the Free Software Foundation; either version 3 of       *
 *  the License, or (at your option) any later version.                  *
 *                                                                       *
 *  This program is distributed in the hope that it will be useful,      *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *  GNU General Public License for more details.                         *
 *                                                                       *
 *  You should have received a copy of the GNU General Public License    *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 *************************************************************************/

#if !defined(DUMMYDEVICE__H)

#define DUMMYDEVICE__H

#include "backend/corebackenddevice.h"

#include <QtGlobal>

class Partition;
class PartitionTable;
class Report;
class CoreBackendPartitionTable;

class DummyDevice : public CoreBackendDevice
{
    Q_DISABLE_COPY(DummyDevice);

public:
    DummyDevice(const QString& device_node);
    ~DummyDevice();

public:
    virtual bool open() override;
    virtual bool openExclusive() override;
    virtual bool close() override;

    virtual CoreBackendPartitionTable* openPartitionTable() override;

    virtual bool createPartitionTable(Report& report, const PartitionTable& ptable) override;

    virtual bool readSectors(void* buffer, qint64 offset, qint64 numSectors) override;
    virtual bool writeSectors(void* buffer, qint64 offset, qint64 numSectors) override;
};

#endif
