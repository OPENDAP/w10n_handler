// -*- mode: c++; c-basic-offset:4 -*-
//
// W10NModule.cc
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

#include <iostream>

#include "BESDebug.h"
#include "BESResponseHandlerList.h"
#include "BESXMLCommand.h"

#include "W10NModule.h"
#include "W10NNames.h"
#include "W10NResponseHandler.h"
#include "W10NInfoCommand.h"
#include "w10n_utils.h"



void
W10NModule::initialize( const string &modname )
{
    BESDEBUG(W10N_DEBUG_KEY, "Initializing w10n Modules:" << endl ) ;

    BESDEBUG( W10N_DEBUG_KEY, "    adding " << W10N_INFO_RESPONSE_STR << " command" << endl ) ;
    BESXMLCommand::add_command( W10N_INFO_RESPONSE_STR, W10NInfoCommand::CommandBuilder ) ;

    BESDEBUG(W10N_DEBUG_KEY, "    adding " << W10N_INFO_RESPONSE << " response handler" << endl ) ;
    BESResponseHandlerList::TheList()->add_handler( W10N_INFO_RESPONSE, W10NResponseHandler::W10NResponseBuilder ) ;


    BESDEBUG(W10N_DEBUG_KEY, "Done Initializing w10n Modules." << endl ) ;
}

void
W10NModule::terminate( const string &modname )
{
    BESDEBUG(W10N_DEBUG_KEY, "Removing w10n Modules:" << endl ) ;

    BESResponseHandlerList::TheList()->remove_handler( W10N_INFO_RESPONSE ) ;


    BESDEBUG(W10N_DEBUG_KEY, "Done Removing w10n Modules." << endl ) ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
W10NModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "W10NModule::dump - ("
			     << (void *)this << ")" << std::endl ;
}

extern "C"
{
    BESAbstractModule *maker()
    {
	return new W10NModule ;
    }
}
