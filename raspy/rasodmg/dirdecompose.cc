/*
* This file is part of rasdaman community.
*
* Rasdaman community is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Rasdaman community is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with rasdaman community.  If not, see <http://www.gnu.org/licenses/>.
*
* Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009 Peter Baumann /
rasdaman GmbH.
*
* For more information please see <http://www.rasdaman.org>
* or contact Peter Baumann via <baumann@rasdaman.com>.
*/
/**
 * SOURCE: dirdecomp.cc
 *
 * MODULE: rasodmg
 * CLASS:  r_DirDecomp
 *
 * COMMENTS:
 *        None
 *
*/

#include "config.h"
#include "../rasodmg/dirdecompose.hh"
#include <string.h>

// Default size of the interval buffer for holding intervals
const r_Dimension r_Dir_Decompose::DEFAULT_INTERVALS = 5;

r_Dir_Decompose::r_Dir_Decompose()
    : num_intervals(0), current_interval(0), intervals(NULL)
{
    num_intervals = r_Dir_Decompose::DEFAULT_INTERVALS;
    intervals = new r_Range[num_intervals];
}

r_Dir_Decompose::~r_Dir_Decompose()
{
    if ( intervals )
    {
        delete [] intervals;
        intervals = NULL;
    }
}

r_Dir_Decompose::r_Dir_Decompose(const r_Dir_Decompose& other)
    : num_intervals(0), current_interval(0), intervals(NULL)
{
    num_intervals = other.num_intervals;
    current_interval = other.current_interval;

    if(other.intervals)
    {
        intervals = new r_Range[num_intervals];
        memcpy(intervals, other.intervals, num_intervals*sizeof(r_Range));
    }
}

const r_Dir_Decompose& r_Dir_Decompose::operator=(const r_Dir_Decompose& other)
{
    if (this != &other)
    {
        if (intervals)
        {
            delete [] intervals;
            intervals = NULL;
        }

        num_intervals = other.num_intervals;
        current_interval = other.current_interval;

        if(other.intervals)
        {
            intervals = new r_Range[num_intervals];
            memcpy(intervals, other.intervals, num_intervals*sizeof(r_Range));
        }
    }

    return *this;
}

r_Dir_Decompose& r_Dir_Decompose::operator<<(r_Range limit)
{
    if (current_interval == num_intervals)
    {
        r_Range *aux = new r_Range[num_intervals*2];

        for (unsigned int i=0; i<num_intervals; i++)
            aux[i] = intervals[i];

        delete [] intervals;
        intervals = aux;

        num_intervals*= 2;
    }

    intervals[current_interval++] = limit;

    return *this;
}

r_Dir_Decompose& r_Dir_Decompose::prepend(r_Range limit)
{
    if (current_interval == num_intervals)
    {
        r_Range *aux = new r_Range[num_intervals*2];

        for (unsigned int i=0; i<num_intervals; i++)
            aux[i+1] = intervals[i];

        delete [] intervals;
        intervals = aux;

        num_intervals*= 2;
    }
    else
    {
        for (int i=static_cast<int>(current_interval)-1; i>=0; i--)
        {
            intervals[i+1] = intervals[i];
        }
    }
    ++current_interval;
    intervals[0] = limit;
    return *this;
}

int r_Dir_Decompose::get_num_intervals() const
{
    return static_cast<int>(current_interval);
}

r_Range r_Dir_Decompose::get_partition(int number) const
throw (r_Eindex_violation)
{
    if (number >= static_cast<int>(current_interval))
    {
        r_Eindex_violation err(0, current_interval, number);
        throw err;
    }

    return intervals[number];
}

void r_Dir_Decompose::print_status(std::ostream& os) const
{
    os << "r_Dir_Decompose[ num intervals = " << num_intervals << " current interval = " << current_interval << " intervals = {";

    for (unsigned int i=0; i<current_interval; i++)
        os << intervals[i] << " ";

    os << "} ]";
}

std::ostream& operator<<(std::ostream& os, const r_Dir_Decompose& d)
{
    d.print_status(os);

    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<r_Dir_Decompose>& vec)
{
    os << " Vector { ";

    unsigned int size = vec.size();
    for (unsigned int i = 0; i < size; i++)
        os << vec[i] << std::endl;

    os << " } ";

    return os;
}

r_Sinterval
r_Dir_Decompose::get_total_interval( )
{
    return r_Sinterval( intervals[0], intervals[current_interval - 1]);
}

