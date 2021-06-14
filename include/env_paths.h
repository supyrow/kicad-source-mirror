/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 CERN
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

///< Helper functions to substitute paths with environmental variables.

#ifndef ENV_PATHS_H
#define ENV_PATHS_H

#include <wx/string.h>
#include <settings/environment.h>

class wxFileName;
class PROJECT;

/**
 * Normalize a file path to an environmental variable, if possible.
 *
 * @param aFilePath is the full file path (path and file name) to be normalized.
 * @param aEnvVars is an optional map of environmental variables to try substitution with.
 * @param aProject is an optional project, to normalize the file path to the project path.
 * @return Normalized full file path (path and file name) if succeeded or empty string if the
 *         path could not be normalized.
 */
wxString NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const PROJECT* aProject );

/**
 * Normalize a file path to an environmental variable, if possible.
 *
 * @param aFilePath is the full file path (path and file name) to be normalized.
 * @param aEnvVars is an optional map of environmental variables to try substitution with.
 * @param aProjectPath is an optional string to normalize the file path to the project path.
 * @return Normalized full file path (path and file name) if succeeded or empty string if the
 *         path could not be normalized.
 */
wxString NormalizePath( const wxFileName& aFilePath, const ENV_VAR_MAP* aEnvVars,
                        const wxString& aProjectPath );

/**
 * Search the default paths trying to find one with the requested file.
 *
 * @param aFileName is the name of the searched file. It might be a relative path.
 * @param aEnvVars is an optional map of environmental variables that can contain paths.
 * @param aProject is an optional project, to check the project path.
 * @return Full path (apth and file name) if the file was found in one of the paths, otherwise
 *         an empty string.
*/
wxString ResolveFile( const wxString& aFileName, const ENV_VAR_MAP* aEnvVars,
                      const PROJECT* aProject );

/**
 * Check if a given filename is within a given project directory (not whether it exists!)
 *
 * @param aFileName is the absolute path to check
 * @param aProject is the project to test against
 * @param aSubPath will be filled with the relative path to the file inside the project (if any)
 * @return true if aFileName's path is inside aProject's path
 */
bool PathIsInsideProject( const wxString& aFileName, const PROJECT* aProject,
                          wxFileName* aSubPath = nullptr );

#endif /* ENV_PATHS_H */
