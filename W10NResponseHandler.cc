// -*- mode: c++; c-basic-offset:4 -*-
//
// W10NResponseHandler.cc
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

#include "W10NResponseHandler.h"
#include "W10NNames.h"
#include "w10n_utils.h"
#include "BESDebug.h"


#include "BESInfoList.h"
#include "BESInfo.h"
#include "BESRequestHandlerList.h"
#include "BESRequestHandler.h"
#include "BESNames.h"
#include "BESDataNames.h"
#include "BESCatalogList.h"
#include "BESCatalog.h"
#include "BESCatalogEntry.h"
#include "BESCatalogUtils.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"

W10NResponseHandler::W10NResponseHandler( const string &name ): BESResponseHandler( name )
{
}

W10NResponseHandler::~W10NResponseHandler( )
{
}

/** @brief executes the command 'show catalog|leaves [for &lt;node&gt;];' by
 * returning nodes or leaves at the top level or at the specified node.
 *
 * The response object BESInfo is created to store the information.
 *
 * @param dhi structure that holds request and response information
 * @see BESDataHandlerInterface
 * @see BESInfo
 * @see BESRequestHandlerList
 */
void W10NResponseHandler::execute(BESDataHandlerInterface &dhi) {

    BESDEBUG(W10N_DEBUG_KEY, "W10NResponseHandler::execute() - BEGIN ############################################################## BEGIN" << endl ) ;

    BESInfo *info = BESInfoList::TheList()->build_info();
    _response = info;

    string container = dhi.data[CONTAINER];
    string catname;
    string defcatname = BESCatalogList::TheCatalogList()->default_catalog();
    BESCatalog *defcat = BESCatalogList::TheCatalogList()->find_catalog(defcatname);
    if (!defcat) {
        string err = (string) "Not able to find the default catalog "
                + defcatname;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
	BESCatalogUtils *utils = BESCatalogUtils::Utils(defcat->get_catalog_name());

    // remove all of the leading slashes from the container name
    string::size_type notslash = container.find_first_not_of("/", 0);
    if (notslash != string::npos) {
        container = container.substr(notslash);
    }

    // see if there is a catalog name here. It's only a possible catalog
    // name
    string::size_type slash = container.find_first_of("/", 0);
    if (slash != string::npos) {
        catname = container.substr(0, slash);
    } else {
        catname = container;
    }

    // see if this catalog exists. If it does, then remove the catalog
    // name from the container (node)
    BESCatalog *catobj = BESCatalogList::TheCatalogList()->find_catalog(
            catname);
    if (catobj) {
        if (slash != string::npos) {
            container = container.substr(slash + 1);

            // remove repeated slashes
            notslash = container.find_first_not_of("/", 0);
            if (notslash != string::npos) {
                container = container.substr(notslash);
            }
        } else {
            container = "";
        }
    }

    if (container.empty())
        container = "/";


    BESDEBUG(W10N_DEBUG_KEY, "W10NResponseHandler::execute() - w10n_id: " << container << endl ) ;

    info->begin_response(W10N_INFO_RESPONSE_STR, dhi);
    //string coi = dhi.data[CATALOG_OR_INFO];

    map<string,string> w10n_attrs;
    w10n_attrs["id"] = container;

    info->begin_tag("w10n",&w10n_attrs);

    string validPath, remainder;
    bool isFile, isDir;

    w10n::eval_w10n_resourceId(
    		container,
    		utils->get_root_dir(),
			utils->follow_sym_links(),
			validPath,
			isFile,
			isDir,
			remainder);

    map<string,string> path_attrs;
    path_attrs["isFile"] = isFile?"true":"false";
    path_attrs["isDir"]  = isDir?"true":"false";

    info->add_tag("validPath",validPath, &path_attrs);
    info->add_tag("remainder",remainder);

    info->end_tag("w10n");


    // end the response object
    info->end_response();


    BESDEBUG(W10N_DEBUG_KEY, "W10NResponseHandler::execute() - END ################################################################## END" << endl ) ;

}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESInfo
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void
W10NResponseHandler::transmit( BESTransmitter *transmitter,
                               BESDataHandlerInterface &dhi )
{
    if( _response )
    {
	BESInfo *info = dynamic_cast<BESInfo *>(_response) ;
	if( !info )
	    throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
	info->transmit( transmitter, dhi ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
W10NResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "W10NResponseHandler::dump - ("
			     << (void *)this << ")" << std::endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
W10NResponseHandler::W10NResponseBuilder( const string &name )
{
    return new W10NResponseHandler( name ) ;
}

