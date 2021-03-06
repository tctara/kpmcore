/*************************************************************************
 *  Copyright (C) 2016 by Chantara Tith <tith.chantara@gmail.com>        *
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

#include "ops/resizevolumegroupoperation.h"

#include "core/lvmdevice.h"
#include "fs/lvm2_pv.h"
#include "core/partition.h"

#include "jobs/resizevolumegroupjob.h"
#include "jobs/movephysicalvolumejob.h"

#include <QString>

#include <KLocalizedString>

/** Creates a new ResizeVolumeGroupOperation.
    @param d the Device to create the new PartitionTable on
    @param partlist list of LVM Physical Volumes that should be in LVM Volume Group
*/
ResizeVolumeGroupOperation::ResizeVolumeGroupOperation(LvmDevice& d, const QStringList partlist)
    : Operation()
    , m_Device(d)
    , m_TargetList(partlist)
    , m_CurrentList(d.deviceNodes())
    , m_GrowVolumeGroupJob(nullptr)
    , m_ShrinkVolumeGroupJob(nullptr)
    , m_MovePhysicalVolumeJob(nullptr)
{
    const QStringList curList = currentList();
    m_TargetSize = FS::lvm2_pv::getPVSize(targetList());
    m_CurrentSize = FS::lvm2_pv::getPVSize(currentList());

    QStringList toRemoveList = curList;
    for (const QString &path : partlist)
        if (toRemoveList.contains(path))
            toRemoveList.removeAll(path);

    QStringList toInsertList = partlist;
    for (const QString &path : curList)
        if (toInsertList.contains(path))
            toInsertList.removeAll(path);

    qint64 freePE = FS::lvm2_pv::getFreePE(curList) - FS::lvm2_pv::getFreePE(toRemoveList);
    qint64 movePE = FS::lvm2_pv::getAllocatedPE(toRemoveList);
    qint64 growPE = FS::lvm2_pv::getPVSize(toInsertList) / LvmDevice::getPeSize(d.name());

    if ( movePE > (freePE + growPE)) {
        // *ABORT* can't move
    } else if (partlist == curList) {
        // *DO NOTHING*
    } else {
        if (!toInsertList.isEmpty()) {
            m_GrowVolumeGroupJob = new ResizeVolumeGroupJob(d, toInsertList, ResizeVolumeGroupJob::Grow);
            addJob(growVolumeGroupJob());
        }
        if (!toRemoveList.isEmpty()) {
            m_MovePhysicalVolumeJob = new MovePhysicalVolumeJob(d, toRemoveList);
            m_ShrinkVolumeGroupJob = new ResizeVolumeGroupJob(d, toRemoveList, ResizeVolumeGroupJob::Shrink);
            addJob(movePhysicalVolumeJob());
            addJob(shrinkvolumegroupjob());
        }
    }
}

QString ResizeVolumeGroupOperation::description() const
{
    QString tList = targetList().join(QStringLiteral(", "));
    QString curList = currentList().join(QStringLiteral(", "));

    return xi18nc("@info/plain", "Resize volume %1 from %2 to %3", device().name(), curList, tList);
}

bool ResizeVolumeGroupOperation::targets(const Device& d) const
{
    return d == device();
}

bool ResizeVolumeGroupOperation::targets(const Partition& p) const
{
    for (const QString &partPath : targetList()) {
        if (partPath == p.partitionPath()) {
            return true;
        }
    }
    return false;
}

void ResizeVolumeGroupOperation::preview()
{
    //asumming that targetSize is larger than the allocated space.
    device().setTotalLogical(targetSize() / device().logicalSize());
    device().partitionTable()->setFirstUsableSector(PartitionTable::defaultFirstUsable(device(), PartitionTable::vmd));
    device().partitionTable()->setLastUsableSector(PartitionTable::defaultLastUsable(device(), PartitionTable::vmd));
    device().partitionTable()->updateUnallocated(device());
}

void ResizeVolumeGroupOperation::undo()
{
    device().setTotalLogical(currentSize() / device().logicalSize());
    device().partitionTable()->setFirstUsableSector(PartitionTable::defaultFirstUsable(device(), PartitionTable::vmd));
    device().partitionTable()->setLastUsableSector(PartitionTable::defaultLastUsable(device(), PartitionTable::vmd));
    device().partitionTable()->updateUnallocated(device());
}
