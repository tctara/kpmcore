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

#include "core/mddevice.h"
#include "fs/filesystem.h"
#include "fs/luks.h"
#include "fs/filesystemfactory.h"
#include "core/partition.h"

#include "core/partitiontable.h"
#include "util/externalcommand.h"
#include "util/helpers.h"

#include <QRegularExpression>
#include <QStringList>
#include <KMountPoint>
#include <KDiskFreeSpaceInfo>
#include <KLocalizedString>

/** Constructs a representation of LVM device with functionning LV as Partition
 *
 *  @param name Volume Group name
 */
MdDevice::MdDevice(const QString name, const QString& iconname)
    : VolumeManagerDevice(name,
                          (QStringLiteral("/dev/") + name),
                          getChunkSize(QStringLiteral("/dev/") + name),
                          getTotalChunk(QStringLiteral("/dev/") + name),
                          iconname,
                          Device::RAID_Device)
{
    initPartitions();
}

void MdDevice::initPartitions()
{
    qint64 firstUsable = 0; //TODO: check if there's an offset
    qint64 lastUsable  = totalLogical() - 1;
    PartitionTable* ptable = new PartitionTable(PartitionTable::vmd, firstUsable, lastUsable);
    FileSystem::Type type = FileSystem::detectFileSystem(deviceNode());

    if (type != FileSystem::Unknown) {
        FileSystem* fs = FileSystemFactory::create(type, firstUsable, lastUsable);

        bool mounted = isMounted(deviceNode());
        QString mountPoint = QString();

        KMountPoint::List mountPointList = KMountPoint::currentMountPoints(KMountPoint::NeedRealDeviceName);
        mountPointList.append(KMountPoint::possibleMountPoints(KMountPoint::NeedRealDeviceName));
        mountPoint = mountPointList.findByDevice(deviceNode()) ?
            mountPointList.findByDevice(deviceNode())->mountPoint() :
            QString();

        const KDiskFreeSpaceInfo freeSpaceInfo = KDiskFreeSpaceInfo::freeSpaceInfo(mountPoint);
        if (mounted && freeSpaceInfo.isValid() && !mountPoint.isEmpty()) {
            fs->setSectorsUsed(freeSpaceInfo.used() / logicalSize());
        } else if (fs->supportGetUsed() == FileSystem::cmdSupportFileSystem) {
            fs->setSectorsUsed(fs->readUsedCapacity(deviceNode()) / logicalSize());
        }

        PartitionRole::Roles r = PartitionRole::Primary;

        if (fs->supportGetLabel() != FileSystem::cmdSupportNone) {
            fs->setLabel(fs->readLabel(deviceNode()));
        }

        Partition* part = new Partition(ptable,
                *this,
                PartitionRole(r),
                fs,
                firstUsable,
                lastUsable,
                deviceNode(),
                PartitionTable::Flag::FlagRaid,
                mountPoint,
                mounted);

        ptable->append(part);
    }

    ptable->updateUnallocated(*this);
    setPartitionTable(ptable);
}

QList<QString> MdDevice::deviceNodeList() const
{
    QList<QString> devlist;
    QString output = getDetail(deviceNode());
    if (!output.isEmpty()) {
        QRegularExpression re(QStringLiteral("\/dev\/(\\w+)"));
        QRegularExpressionMatchIterator i = re.globalMatch(output);
        while (i.hasNext()) {
            QRegularExpressionMatch reMatch = i.next();
            QString devpath = QStringLiteral("/dev/") + reMatch.captured(1).trimmed();
            if (devpath != deviceNode()) {
                devlist << devpath;
            }
        }
    }
    return devlist;
}

qint64 MdDevice::mappedSector(const QString& devNode, qint64 sector) const
{
    return sector;
}

QList<MdDevice*> MdDevice::scanSystemMD()
{
    QList<MdDevice*> mdList;

    ExternalCommand scanMD(QStringLiteral("cat"),
            { QStringLiteral("/proc/mdstat") });

    if (scanMD.run(-1) && scanMD.exitCode() == 0) {
        QRegularExpression re(QStringLiteral("md(\\d+)\\s+:"));
        QRegularExpressionMatchIterator i  = re.globalMatch(scanMD.output());
        while (i.hasNext()) {
            QRegularExpressionMatch reMatch = i.next();
            mdList << new MdDevice(QStringLiteral("md") + reMatch.captured(1).trimmed());
        }
    }

    return mdList;
}

qint32 MdDevice::getRaidLevel(QString mdpath)
{
    QString output = getDetail(mdpath);
    if (!output.isEmpty()) {
        QRegularExpression re(QStringLiteral("Raid Level :\\s+\\w+(\\d+)"));
        QRegularExpressionMatch reMatch = re.match(output);
        if (reMatch.hasMatch())
            return reMatch.captured(1).toLongLong();
    }
    return -1;
}

qint64 MdDevice::getTotalChunk(QString mdpath)
{
    return getArraySize(mdpath) / getChunkSize(mdpath);
}

qint64 MdDevice::getChunkSize(QString mdpath)
{
    QString output = getDetail(mdpath);
    if (!output.isEmpty()) {
        QRegularExpression re(QStringLiteral("Chunk Size :\\s+(\\d+)"));
        QRegularExpressionMatch reMatch = re.match(output);
        if (reMatch.hasMatch())
            return reMatch.captured(1).toLongLong() * 1024;
    }
    return -1;
}

// return array size in bytes
qint64 MdDevice::getArraySize(QString mdpath)
{
    QString output = getDetail(mdpath);
    if (!output.isEmpty()) {
        QRegularExpression re(QStringLiteral("Array Size :\\s+(\\d+)"));
        QRegularExpressionMatch reMatch = re.match(output);
        if (reMatch.hasMatch())
            return reMatch.captured(1).toLongLong() * 1024;
    }
    return -1;
}

QString MdDevice::getDetail(QString mdpath)
{
    ExternalCommand cmd(QStringLiteral("mdadm"),
                    {   QStringLiteral("--detail"),
                        mdpath});
    return (cmd.run(-1) && cmd.exitCode() == 0) ? cmd.output() : QString();
}

bool failDevice(Report& report, MdDevice& dev, const QString& devpath)
{
    ExternalCommand cmd(report, QStringLiteral("mdadm"),
            {QStringLiteral("--fail"),
             dev.deviceNode(),
             devpath});
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}

bool removeDevice(Report& report, MdDevice& dev, const QString& devpath)
{
    ExternalCommand cmd(report, QStringLiteral("mdadm"),
            {QStringLiteral("--remove"),
             dev.deviceNode(),
             devpath});
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}

bool insertDevice(Report& report, MdDevice& dev, const QString& devpath)
{
    ExternalCommand cmd(report, QStringLiteral("mdadm"),
            {QStringLiteral("--add"),
             dev.deviceNode(),
             devpath});
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}

bool stopMD(Report& report, MdDevice& dev)
{
    ExternalCommand cmd(report, QStringLiteral("mdadm"),
            {QStringLiteral("--stop"),
             dev.deviceNode() });
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}

bool removeMD(Report& report, MdDevice& dev)
{
    ExternalCommand cmd(report, QStringLiteral("mdadm"),
            {QStringLiteral("--remove"),
             dev.deviceNode() });
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}

bool createMD(Report& report, MdDevice& dev, const QStringList devpathList, const qint32 level, const qint32 chunkSize)
{
    QStringList args = {QStringLiteral("--create"),
             QStringLiteral("--verbose"),
             dev.deviceNode(),
             QStringLiteral("--level=") + QString::number(level),
             QStringLiteral("--raid-devices=") + QString::number(devpathList.count())};
    foreach (QString devpath, devpathList) {
        args << devpath;
    }
    ExternalCommand cmd(report, QStringLiteral("mdadm"), args);
    return (cmd.run(-1) && cmd.exitCode() == 0) ? true : false;
}
