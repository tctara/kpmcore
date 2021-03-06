/*************************************************************************
 *  Copyright (C) 2008 by Volker Lanz <vl@fidra.de>                      *
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

#include "util/globallog.h"

GlobalLog* GlobalLog::instance()
{
    static GlobalLog* p = nullptr;

    if (p == nullptr)
        p = new GlobalLog();

    return p;
}

void GlobalLog::flush(Log::Level lev)
{
    emit newMessage(lev, msg);
    msg.clear();
}

// --------------------------------------------------------------------------

Log::~Log()
{
    if (--ref == 0)
        GlobalLog::instance()->flush(level);
}
