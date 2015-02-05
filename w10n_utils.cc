// -*- mode: c++; c-basic-offset:4 -*-
//
// w10n_utils.cc
//
// This file is part of BES w10n handler
//
// Copyright (c) 2015v OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// Please read the full copyright statement in the file COPYRIGHT_URI.
//


#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>

using std::istringstream;
using std::cout;
using std::endl;

#include "BESUtil.h"
#include "BESDebug.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESInternalError.h"

#include "w10n_utils.h"
#include "W10NNames.h"



namespace w10n {

void eval_resource_path(
		const string &w10nResourceId,
		const string &catalogRoot,
		const bool follow_sym_links,
		string &validPath,
		bool &isFile,
		bool &isDir,
		string &remainder) {

	BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - CatalogRoot: "<< catalogRoot << endl);
	BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - w10n resourceID: "<< w10nResourceId << endl);


	// Rather than have two basically identical code paths for the two cases (follow and !follow symlinks)
	// We evaluate the follow_sym_links switch and use a function pointer to get the correct "stat"
	// function for the eval operation.
	int (*ye_old_stat_function)(const char *pathname, struct stat *buf);
	if(follow_sym_links){
		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - Using 'stat' function (follow_sym_links = true)" << endl);
		ye_old_stat_function = &stat;
	}
	else {
		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - Using 'lstat' function (follow_sym_links = false)" << endl);
		ye_old_stat_function = &lstat;
	}


	// if nothing is passed in path, then the path checks out since root is
	// assumed to be valid.
	if (w10nResourceId == ""){
		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - w10n resourceID is empty" << endl);

		validPath = "";
		remainder = "";
		return;
	}

	// make sure there are no ../ in the path, backing up in any way is
	// not allowed.
	string::size_type dotdot = w10nResourceId.find("..");
	if (dotdot != string::npos) {
		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - ERROR: w10n resourceID '" << w10nResourceId <<"' contains the substring '..' This is Forbidden." << endl);
		string s = (string) "Invalid node name '" + w10nResourceId +"' ACCESS IS FORBIDDEN";
		throw BESForbiddenError(s, __FILE__, __LINE__);
	}

	// What I want to do is to take each part of path and check to see if it
	// is a symbolic link and it is accessible. If everything is ok, add the
	// next part of the path.
	bool done = false;

	// what is remaining to check
    string rem = w10nResourceId;
    remainder = rem;

	// Remove leading slash
	if (rem[0] == '/')
		rem = rem.substr(1, rem.length() - 1);

	// Remove trailing slash
	if (rem[rem.length() - 1] == '/')
		rem = rem.substr(0, rem.length() - 1);

	// full path of the thing to check
	string fullpath = catalogRoot;
	// Remove leading slash
	if (fullpath[fullpath.length() - 1] == '/') {
		fullpath = fullpath.substr(0, fullpath.length() - 1);
	}


	// path checked so far
	string checking;
	// string validPath;


	isFile = false;
	isDir = false;

	while (!done) {
		size_t slash = rem.find('/');
		if (slash == string::npos) {
			BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - Checking final path component: " << rem << endl);
			fullpath = fullpath + "/" + rem;
			checking = validPath + "/" + rem;
			rem ="";
			done = true;
		} else {
			fullpath = fullpath + "/" + rem.substr(0, slash);
			checking = validPath + "/" + rem.substr(0, slash);
			rem = rem.substr(slash + 1, rem.length() - slash);
		}

		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - fullpath: "<< fullpath << endl);
		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - checking: "<< checking << endl);


		struct stat sb;
		int statret = ye_old_stat_function(fullpath.c_str(), &sb);


		if (statret == -1) {
			int errsv = errno;
			// stat failed, so not accessible. Get the error string,
			// store in error, and throw exception
			char *s_err = strerror(errsv);
			string error = "Unable to access node " + checking + ": ";
			if (s_err) {
				error = error + s_err;
			} else {
				error = error + "unknown access error";
			}
			BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - error: "<< error << "   errno: " << errno << endl);

			// ENOENT means that the node wasn't found. Otherwise, access
			// is denied for some reason
			if (errsv == ENOENT || errsv == ENOTDIR) {
				BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - validPath: "<< validPath << endl);
				BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - remainder: "<< remainder << endl);
				return;
			} else {
				throw BESForbiddenError(error, __FILE__, __LINE__);
			}
		} else {
			validPath = checking;
			remainder = rem;

	        if (S_ISREG(sb.st_mode)) {
	    		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - '"<< checking << "' Is regular file." << endl);
	    		isFile = true;
	    		isDir = false;
	        }
	        else if (S_ISDIR(sb.st_mode)) {
	    		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - '"<< checking << "' Is directory." << endl);
	    		isFile = false;
	    		isDir = true;
	        }
	        else if (S_ISLNK(sb.st_mode)) {
	    		BESDEBUG(W10N_DEBUG_KEY,"eval_resource_path() - '"<< checking << "' Is symbolic Link." << endl);
				string error = "Service not configured to traverse symbolic links as embodied by the node '"
						+ checking + "' ACCESS IS FORBIDDEN";
				throw BESForbiddenError(error, __FILE__, __LINE__);
			}
		}


	}

}


} // namespace w10n

