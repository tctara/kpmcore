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

#include "jobs/resizevolumegroupjob.h"

#include "core/lvmdevice.h"

#include "util/report.h"

#include <KLocalizedString>

/** Creates a new ResizeVolumeGroupJob
*/
ResizeVolumeGroupJob::ResizeVolumeGroupJob(LvmDevice& dev, const QStringList partlist, const Type type) :
    Job(),
    m_Device(dev),
    m_PartList(partlist),
    m_Type(type)
{
}

bool ResizeVolumeGroupJob::run(Report& parent)
{
    bool rval = false;

    Report* report = jobStarted(parent);

    for (const auto &pvpath : partList()) {
        if (type() == ResizeVolumeGroupJob::Grow) {
            rval = LvmDevice::insertPV(*report, device(), pvpath);
        } else if (type() == ResizeVolumeGroupJob::Shrink) {
            rval = LvmDevice::removePV(*report, device(), pvpath);
        }
        if (rval == false) {
            break;
        }
    }

    jobFinished(*report, rval);

    return rval;
}

QString ResizeVolumeGroupJob::description() const
{
    const QString partitionList = partList().join(QStringLiteral(", "));
    const qint32 count = partList().count();

    if (type() == ResizeVolumeGroupJob::Grow) {
        return xi18ncp("@info/plain", "Adding LVM Physical Volume %2 to %3.", "Adding LVM Physical Volumes %2 to %3.", count, partitionList, device().name());
    }
    if (type() == ResizeVolumeGroupJob::Shrink) {
        return xi18ncp("@info/plain", "Removing LVM Physical Volume %2 from %3.", "Removing LVM Physical Volumes %2 from %3.", count, partitionList, device().name());
    }
    return xi18nc("@info/plain", "Resizing Volume Group %1 to %2.", device().name(), partitionList);
}
