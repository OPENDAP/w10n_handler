// -*- mode: c++; c-basic-offset:4 -*-
//
// W10nJsonTransmitter.cc
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>
#include <ConstraintEvaluator.h>

#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDDSResponse.h>
#include <BESDapError.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESSyntaxUserError.h>

#include "W10nJsonTransmitter.h"

#include "W10nJsonTransform.h"
#include "W10NNames.h"
#include "w10n_utils.h"


using namespace ::libdap;

#define W10N_JSON_TEMP_DIR "/tmp"

string W10nJsonTransmitter::temp_dir;

/** @brief Construct the W10nJsonTransmitter
 *
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as abstract object representation JSON documents.
 *
 * The OPeNDAP data object is written to a JSON file locally in a
 * temporary directory specified by the BES configuration parameter
 * FoJson.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FO_JSON_TEMP_DIR.
 */
W10nJsonTransmitter::W10nJsonTransmitter() : BESBasicTransmitter()
{
    add_method(DATA_SERVICE, W10nJsonTransmitter::send_data);
    add_method(DDX_SERVICE,  W10nJsonTransmitter::send_metadata);

    if (W10nJsonTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "W10nJson.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, W10nJsonTransmitter::temp_dir, found);
        if (!found || W10nJsonTransmitter::temp_dir.empty()) {
        	W10nJsonTransmitter::temp_dir = W10N_JSON_TEMP_DIR;
        }
        string::size_type len = W10nJsonTransmitter::temp_dir.length();
        if (W10nJsonTransmitter::temp_dir[len - 1] == '/') {
        	W10nJsonTransmitter::temp_dir = W10nJsonTransmitter::temp_dir.substr(0, len - 1);
        }
    }
}



/**
 * Checks the supplied constraint expression by first removing any selection clauses and then
 * verifies that the projection clause contains only a single variable reference.
 */
void W10nJsonTransmitter::checkConstraintForW10nCompatibility(const string &ce){
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - BEGIN. ce:  "<< ce << endl);

	string projectionClause = getProjectionClause(ce);
	int firstComma = projectionClause.find(",");

	if(firstComma != -1){
		string msg = "The w10n protocol only allows one variable to be selected at a time. ";
	    msg += "The constraint expression '" + ce + "' requests more than one.";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - ERROR! "<< msg << endl);
		throw BESSyntaxUserError(msg , __FILE__, __LINE__);
	}
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::checkConstraintForW10nCompatibility() - END:  " << endl);

}

/**
 * Strips any selection clauses from the passed constraint expression and returns only the projection clause part.
 */
string W10nJsonTransmitter::getProjectionClause(const string &constraintExpression){

	string projectionClause = constraintExpression;
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::getProjectionClause() - constraintExpression: "<< constraintExpression << endl);

    int firstAmpersand = constraintExpression.find("&");
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::getProjectionClause() - firstAmpersand: "<< firstAmpersand << endl);
	if(firstAmpersand>=0)
		projectionClause = constraintExpression.substr(0,firstAmpersand);

	BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::getProjectionClause() - CE projection clause: "<< projectionClause << endl);

	return projectionClause;
}

/**
 * Strips any selection clauses from the passed constraint expression and returns only the projection clause part.
 */
string W10nJsonTransmitter::getProjectedVariableName(const string &constraintExpression){

	string varName = getProjectionClause(constraintExpression);

	int firstSquareBracket = varName.find("[");
	if(firstSquareBracket != -1){
		varName = varName.substr(0,firstSquareBracket);
	}
	return varName;


}





/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void W10nJsonTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - BEGIN." << endl);

    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) {
    	string msg = "Dynamic cast of BESResponseObject to BESDataDDSResponse FAILED.";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    DataDDS *dds = bdds->get_dds();
    if (!dds){
    	string msg = "No DataDDS has been created for transmit";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm){
    	string msg = "Output stream is not set, cannot return data as JSON";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }



    ConstraintEvaluator &eval = bdds->get_ce();


    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");

    checkConstraintForW10nCompatibility(ce);

    try {
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - Parsing the constraint: "<< ce << endl);
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
    	string msg = "Failed to parse the constraint expression. Msg: "+ e.get_error_message();
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESDapError(msg, 0, e.get_error_code(),__FILE__, __LINE__);
    }
    catch (...) {
    	string msg = "Failed to parse the constraint expression. Unknown exception caught";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    w10n::checkConstrainedDDSForW10nDataCompatibility(dds);


    // now we need to read the data
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - reading data into DataDDS" << endl);

    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG(W10N_DEBUG_KEY, "processing a functional constraint clause(s)." << endl);
            DataDDS *tmp_dds = eval.eval_function_clauses(*dds);
            bdds->set_dds(tmp_dds);
            delete dds;
            dds = tmp_dds;
        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.
            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(eval, *dds);
                }
            }
        }
    }
    catch (Error &e) {
    	string msg = "Failed to read data! Msg: " + e.get_error_message();
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESDapError(msg, 0, e.get_error_code(),__FILE__, __LINE__);
    }
    catch (...) {
    	string msg = "Failed to read data: Unknown exception caught";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    try {


        W10nJsonTransform ft(dds, dhi, &o_strm);

        string varName = getProjectedVariableName(ce);

        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - Sending w10n meta response for variable "
        		<< varName << endl);
    	ft.sendW10nDataForVariable(varName);
    }
    catch (BESError &e) {
    	string msg = "Failed to transmit data as w10n JSON. Msg: " + e.get_message();
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw;
    }
    catch (...) {
    	string msg = "Failed to transmit data as w10n JSON. Unknown Error.";
	    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - ERROR! "<< msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }

    cleanupW10nContexts();

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_data() - END. Done transmitting JSON" << endl);
}



/** @brief The static method registered to transmit OPeNDAP data objects as
 * a JSON file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a JSON file, and
 * streams back that JSON file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a JSON file, or
 * streaming the JSON file
 */
void W10nJsonTransmitter::send_metadata(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDDSResponse *bdds = dynamic_cast<BESDDSResponse *>(obj);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DDS has been created for transmit", __FILE__, __LINE__);

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();

    ostream &o_strm = dhi.get_output_stream();
    if (!o_strm)
        throw BESInternalError("Output stream is not set, can not return as JSON", __FILE__, __LINE__);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");

    checkConstraintForW10nCompatibility(ce);

    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESInternalError("Failed to parse the constraint expression: " + e.get_error_message(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__, __LINE__);
    }


	W10nJsonTransform ft(dds, dhi, &o_strm);

    string varName = getProjectedVariableName(ce);

    if(varName.length() == 0){
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - Sending w10n meta response for DDS" << endl);
    	ft.sendW10nMetaForDDS();
    }
    else {
        BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - Sending w10n meta response for variable "
        		<< varName << endl);
    	ft.sendW10nMetaForVariable(varName, true);
    }

    cleanupW10nContexts();

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::send_metadata() - done transmitting JSON" << endl);
}

/** @brief stream the temporary netcdf file back to the requester
 *
 * Streams the temporary netcdf file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void W10nJsonTransmitter::return_temp_stream(const string &filename, ostream &strm)
{
    //  int bytes = 0 ;    // Not used; jhrg 3/16/11
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os) {
        string err = "Can not connect to file " + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    int nbytes;
    char block[4096];

    os.read(block, sizeof block);
    nbytes = os.gcount();
    if (nbytes > 0) {
        strm.write(block, nbytes);
        //bytes += nbytes ;
    }
    else {
        // close the stream before we leave.
        os.close();

        string err = (string) "0XAAE234F: failed to stream. Internal server "
                + "error, got zero count on stream buffer." + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    while (os) {
        os.read(block, sizeof block);
        nbytes = os.gcount();
        strm.write(block, nbytes);
        //write( fileno( stdout ),(void*)block, nbytes ) ;
        //bytes += nbytes ;
    }
    os.close();
}



void W10nJsonTransmitter::cleanupW10nContexts()
{
    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::cleanupW10nContexts() - Removing context" << W10N_META_OBJECT_KEY << endl);
	BESContextManager::TheManager()->unset_context(W10N_META_OBJECT_KEY);

	BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::cleanupW10nContexts() - Removing context" << W10N_CALLBACK_KEY << endl);
	BESContextManager::TheManager()->unset_context(W10N_CALLBACK_KEY);

	BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransmitter::cleanupW10nContexts() - Removing context" << W10N_FLATTEN_KEY << endl);
	BESContextManager::TheManager()->unset_context(W10N_FLATTEN_KEY);
}
