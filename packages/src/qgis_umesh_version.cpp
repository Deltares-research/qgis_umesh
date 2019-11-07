//---- LGPL --------------------------------------------------------------------
//
// Copyright (C)  Stichting Deltares, 2011-2015.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation version 2.1.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, see <http://www.gnu.org/licenses/>.
//
// contact: delft3d.support@deltares.nl
// Stichting Deltares
// P.O. Box 177
// 2600 MH Delft, The Netherlands
//
// All indications and logos of, and references to, "Delft3D" and "Deltares"
// are registered trademarks of Stichting Deltares, and remain the property of
// Stichting Deltares. All rights reserved.
//
//------------------------------------------------------------------------------
// $Id: qgis_umesh_version.cpp 2194 2019-10-01 17:46:13Z mooiman $
// $HeadURL: file:///D:/SVNrepository/examples/qt/qgis/qgis_umesh/packages/src/qgis_umesh_version.cpp $
//#include <stdio.h>
#include <string.h>

#include "qgis_umesh_version.h"

#if defined(WIN32) || defined (WIN64)
# define strdup _strdup
#endif

#if defined(WIN32)
static char qgis_umesh_version[] = {qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Win32)"};
static char qgis_umesh_version_id[] = {"@(#)Deltares, "qgis_umesh_program" Version "qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Win32), "__DATE__", "__TIME__""};
#elif defined(WIN64)
static char qgis_umesh_version[] = { qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build " (Win64)" };
static char qgis_umesh_version_id[] = {"@(#)Deltares, " qgis_umesh_program " Version " qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build " (Win64), " __DATE__ ", " __TIME__ "" };
#elif defined(LINUX64)
static char qgis_umesh_version[] = {qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Linux64)"};
static char qgis_umesh_version_id[] = {"@(#)Deltares, "qgis_umesh_program" Version "qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Linux64), "__DATE__", "__TIME__""};
#else
static char qgis_umesh_version[] = {qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Unknown)"};
static char qgis_umesh_version_id[] = {"@(#)Deltares, "qgis_umesh_program" Version "qgis_umesh_major "." qgis_umesh_minor "." qgis_umesh_revision "." qgis_umesh_build" (Unknown), "__DATE__", "__TIME__""};
#endif
static char qgis_umesh_company_name[] = {"Deltares"};
static char qgis_umesh_program_name[] = { qgis_umesh_program };

char * getfullversionstring_qgis_umesh(void)
{
    return strdup(qgis_umesh_version_id);
};
char * getversionstring_qgis_umesh(void)
{
    return strdup(qgis_umesh_version);
};
char * getcompanystring_qgis_umesh(void)
{
    return strdup(qgis_umesh_company_name);
};
char * getprogramstring_qgis_umesh(void)
{
    return strdup(qgis_umesh_program_name);
};
