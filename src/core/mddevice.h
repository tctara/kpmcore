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

#if !defined(MDDEVICE__H)

#define MDDEVICE__H

#include "core/volumemanagerdevice.h"

#include "util/libpartitionmanagerexport.h"
#include "util/report.h"

#include <QString>
#include <QObject>
#include <QtGlobal>

class SmartStatus;

/** A device.

    Represents a device like /dev/sda.

    Devices are the outermost entity; they contain a PartitionTable that itself contains Partitions.

    @see PartitionTable, Partition
*/

class LIBKPMCORE_EXPORT MdDevice : public VolumeManagerDevice
{
    Q_DISABLE_COPY(MdDevice)

public:
    MdDevice(const QString name, const QString& iconname = QString());

public:
    static QList<MdDevice*> scanSystemMD();

    static qint32 getRaidLevel(QString mdpath);
    static qint64 getChunkSize(QString mdpath);
    static qint64 getTotalChunk(QString mdpath);
    static qint64 getArraySize(QString mdpath);

    static QString getUUID(const QString mdpath);
    static QString getDetail(QString mdpath);

    static bool failDevice(Report& report, MdDevice& dev, const QString& devpath);
    static bool removeDevice(Report& report, MdDevice& dev, const QString& devpath);
    static bool insertDevice(Report& report, MdDevice& dev, const QString& devpath);

    static bool stopMD(Report& report, MdDevice& dev);
    static bool removeMD(Report& report, MdDevice& dev);
    static bool createMD(Report& report, MdDevice& dev, const QStringList devpathList, const qint32 chunkSize = 512 ); //512 B or K ???

protected:
    void initPartitions();
    QList<QString> deviceNodeList() const override;
    qint64 mappedSector(const QString& devNode, qint64 sector) const override;

private:

};

#endif

