// -*- mode: c++; c-basic-offset:4 -*-
//
// W10nJsonTransform.cc
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

#include "W10NNames.h"
#include "W10nJsonTransform.h"
#include "config.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <stddef.h>
#include <string>

using std::ostringstream;
using std::istringstream;


#include <DDS.h>
#include <Structure.h>
#include <Constructor.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <Str.h>
#include <Url.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include <w10n_utils.h>


/**
 * Returns the correct w10n type for the passed instance of a DAP type.
 */
string getW10nTypeString(libdap::BaseType *bt){
	switch(bt->type()){
	// Handle the atomic types - that's easy!
	case libdap::dods_byte_c:
		return "b";
		break;

	case libdap::dods_int16_c:
		return "i";
		break;

	case libdap::dods_uint16_c:
		return "ui";
		break;

	case libdap::dods_int32_c:
		return "i";
		break;

	case libdap::dods_uint32_c:
		return "ui";
		break;

	case libdap::dods_float32_c:
		return "f";
		break;

	case libdap::dods_float64_c:
		return "f";
		break;

	case libdap::dods_str_c:
		return "s";
		break;

	case libdap::dods_url_c:
		return "s";
		break;

	case libdap::dods_structure_c:
	case libdap::dods_grid_c:
	case libdap::dods_sequence_c:
	case libdap::dods_array_c:
	{
		string s = (string) "W10nJsonTransform:  W10N only supports complex data types as nodes. "
				"The variable " + bt->type_name() +" " +bt->name() +" is a node type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}


	case libdap::dods_int8_c:
	case libdap::dods_uint8_c:
	case libdap::dods_int64_c:
	case libdap::dods_uint64_c:
	case libdap::dods_url4_c:
	case libdap::dods_enum_c:
	case libdap::dods_group_c:
	{
		string s = (string) "W10nJsonTransform:  DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	default:
	{
		string s = (string) "W10nJsonTransform:  Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	}

}

/**
 *  @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
 *
 */
template<typename T> unsigned  int W10nJsonTransform::json_simple_type_array_worker(ostream *strm, T *values, unsigned int indx, vector<unsigned int> *shape, unsigned int currentDim){

	*strm << "[";

	unsigned int currentDimSize = (*shape)[currentDim];

    for(int i=0; i<currentDimSize ;i++){
		if(currentDim < shape->size()-1){
			BESDEBUG(W10N_DEBUG_KEY, "json_simple_type_array_worker() - Recursing! indx:  " << indx
					<< " currentDim: " << currentDim
					<< " currentDimSize: " << currentDimSize
					<< endl);
			indx = json_simple_type_array_worker<T>(strm,values,indx,shape,currentDim+1);
			if(i+1 != currentDimSize)
		    	*strm << ", ";
		}
		else {
	    	if(i)
		    	*strm << ", ";
	    	*strm << values[indx++];
		}
    }
	*strm << "]";

	return indx;
}



/**
 * Writes the w10n json representation of the passed DAP Array of simple types. If the
 * parameter "sendData" evaluates to true then data will also be sent.
 */
template<typename T>void W10nJsonTransform::json_simple_type_array(ostream *strm, libdap::Array *a, string indent, bool sendData){


	*strm << indent << "{" << endl;\

	string childindent = indent + _indent_increment;

	writeLeafMetadata(strm,a,childindent);

	int numDim = a->dimensions(true);
	vector<unsigned int> shape(numDim);
	long length = w10n::computeConstrainedShape(a, &shape);

	*strm << childindent << "\"shape\": [";

	for(int i=0; i<shape.size() ;i++){
		if(i>0)
			*strm << ",";
		*strm << shape[i];
	}
	*strm << "]";

	if(sendData){
		*strm << ","<< endl;

		// Data
		*strm << childindent << "\"data\": ";

	    T *src = new T[length];
	    a->value(src);

	    unsigned int indx = json_simple_type_array_worker(strm, src, 0, &shape, 0);

	    delete src;

	    if(length != indx)
			BESDEBUG(W10N_DEBUG_KEY, "json_simple_type_array() - indx NOT equal to content length! indx:  " << indx << "  length: " << length << endl);


	}

	*strm << endl << indent << "}";

}



/**
 * Writes the w10n json opener for the Dataset, including name and top level DAP attributes.
 */
void W10nJsonTransform::writeDatasetMetadata(ostream *strm, libdap::DDS *dds, string indent){

	// Name
	*strm << indent << "\"name\": \""<< dds->get_dataset_name() << "\"," << endl;

	//Attributes
	transform(strm, dds->get_attr_table(), indent);
	*strm << "," << endl;

}

/**
 * Writes w10n json opener for a DAP object that seen as a "node" in w10n semantics.
 * Header includes object name and attributes
 */
void W10nJsonTransform::writeNodeMetadata(ostream *strm, libdap::BaseType *bt, string indent){

	// Name
	*strm << indent << "\"name\": \""<< bt->name() << "\"," << endl;

	//Attributes
	transform(strm, bt->get_attr_table(), indent);
	*strm << "," << endl;



}

/**
 * Writes w10n json opener for a DAP object that is seen as a "leaf" in w10n semantics.
 * Header includes object name. attributes, and w10n type.
 */
void W10nJsonTransform::writeLeafMetadata(ostream *strm, libdap::BaseType *bt, string indent){

	// Name
	*strm << indent << "\"name\": \""<< bt->name() << "\"," << endl;



	// w10n type
	if(bt->type() == libdap::dods_array_c){
		libdap::Array *a = (libdap::Array *)bt;
		*strm << indent << "\"type\": \""<< getW10nTypeString(a->var()) << "\"," << endl;
	}
	else {
		*strm << indent << "\"type\": \""<< getW10nTypeString(bt) << "\"," << endl;
	}


	//Attributes
	transform(strm, bt->get_attr_table(), indent);
	*strm << "," << endl;



}




/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
W10nJsonTransform::W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const string &localfile) :
        _dds(dds), _localfile(localfile), _indent_increment("  "), _ostrm(0)
{
    if (!_dds)
        throw BESInternalError("W10nJsonTransform:  null DDS passed to constructor", __FILE__, __LINE__);

    if (_localfile.empty())
        throw BESInternalError("W10nJsonTransform:  empty local file name passed to constructor", __FILE__, __LINE__);
}

W10nJsonTransform::W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, std::ostream *ostrm) :
        _dds(dds), _localfile(""), _indent_increment("  "), _ostrm(ostrm)
{
    if (!_dds)
        throw BESInternalError("W10nJsonTransform:  null DDS passed to constructor", __FILE__, __LINE__);

    if (!_ostrm)
    	throw BESInternalError("W10nJsonTransform:  null stream pointer passed to constructor", __FILE__, __LINE__);
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
W10nJsonTransform::~W10nJsonTransform()
{
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FoJson objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void W10nJsonTransform::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "W10nJsonTransform::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl;
    if(_dds != 0){
        _dds->print(strm);
    }
    BESIndent::UnIndent();
}




/** @brief Transforms each of the marked variables of the DataDDS to the JSON
 * file.
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the JSON file. Each OPeNDAP data type translates into a
 * particular JSON type. Also write out any global attributes stored at the
 * top level of the DataDDS.
 */
void W10nJsonTransform::transform(bool sendData)
{
    // used to ensure the _ostrm is closed only when it's a temp file
	bool used_temp_file = false;
	fstream temp_file;

	if (!_ostrm) {
		temp_file.open(_localfile.c_str(), std::fstream::out);
		if (!temp_file)
			throw BESInternalError("Could not open temp file: " + _localfile, __FILE__, __LINE__);
		_ostrm = &temp_file;
		used_temp_file = true;
	}

	try {
		transform(_ostrm, _dds, "", sendData);
		if (used_temp_file)
			temp_file.close();
	}
	catch (...) {
		if (used_temp_file)
			temp_file.close();
		throw;
	}
}


/**
 * DAP Constructor types are semantically equivalent to a w10n node type so they
 * must be represented as a collection of child nodes and leaves.
 */
void W10nJsonTransform::transform(ostream *strm, libdap::Constructor *cnstrctr, string indent, bool sendData){
	vector<libdap::BaseType *> leaves;
	vector<libdap::BaseType *> nodes;


	// Sort the variables into two sets/
	libdap::DDS::Vars_iter vi = cnstrctr->var_begin();
	libdap::DDS::Vars_iter ve = cnstrctr->var_end();
	for (; vi != ve; vi++) {
		if ((*vi)->send_p()) {
			libdap::BaseType *v = *vi;
			v->is_constructor_type();
			libdap::Type type = v->type();
			if(type == libdap::dods_array_c){
				type = v->var()->type();
			}
			if(v->is_constructor_type() ||
					(v->is_vector_type() && v->var()->is_constructor_type())){
				nodes.push_back(v);
			}
			else {
				leaves.push_back(v);
			}
		}
	}

	// Declare this node
	*strm << indent << "{" << endl ;
	string child_indent = indent + _indent_increment;

	// Write this node's metadata (name & attributes)
	writeNodeMetadata(strm, cnstrctr, child_indent);

	transform_node_worker(strm, leaves,  nodes, child_indent, sendData);

	*strm << indent << "}" << endl;

}

/**
 * This worker method allows us to recursively traverse a "node" variables contents and
 * any child nodes will be traversed as well.
 */
void W10nJsonTransform::transform_node_worker(ostream *strm, vector<libdap::BaseType *> leaves, vector<libdap::BaseType *> nodes, string indent, bool sendData){

	// Write down this nodes leaves
	*strm << indent << "\"leaves\": [";
	if(leaves.size() > 0)
		*strm << endl;
	for(int l=0; l< leaves.size(); l++){
		libdap::BaseType *v = leaves[l];
		BESDEBUG(W10N_DEBUG_KEY, "Processing LEAF: " << v->name() << endl);
		if( l>0 ){
			*strm << "," ;
			*strm << endl ;
		}
		transform(strm, v, indent + _indent_increment, sendData);
	}
	if(leaves.size()>0)
		*strm << endl << indent;
	*strm << "]," << endl;


	// Write down this nodes child nodes
	*strm << indent << "\"nodes\": [";
	if(nodes.size() > 0)
		*strm << endl;
	for(int n=0; n< nodes.size(); n++){
		libdap::BaseType *v = nodes[n];
		transform(strm, v, indent + _indent_increment, sendData);
	}
	if(nodes.size()>0)
		*strm << endl << indent;

	*strm << "]" << endl;


}


/**
 * Writes a w10n JSON representation of the DDS to the passed stream. Data is sent is the sendData
 * flag is true.
 */
void W10nJsonTransform::transform(ostream *strm, libdap::DDS *dds, string indent, bool sendData){



	/**
	 * w10 sees the world in terms of leaves and nodes. Leaves have data, nodes have other nodes and leaves.
	 */
	vector<libdap::BaseType *> leaves;
	vector<libdap::BaseType *> nodes;

	libdap::DDS::Vars_iter vi = dds->var_begin();
	libdap::DDS::Vars_iter ve = dds->var_end();
	for (; vi != ve; vi++) {
		if ((*vi)->send_p()) {
			libdap::BaseType *v = *vi;
			libdap::Type type = v->type();
			if(type == libdap::dods_array_c){
				type = v->var()->type();
			}
			if(v->is_constructor_type() ||
					(v->is_vector_type() && v->var()->is_constructor_type())){
				nodes.push_back(v);
			}
			else {
				leaves.push_back(v);
			}
		}
	}

	// Declare this node
	*strm << indent << "{" << endl ;
	string child_indent = indent + _indent_increment;

	// Write this node's metadata (name & attributes)
	writeDatasetMetadata(strm, dds, child_indent);

	transform_node_worker(strm, leaves,  nodes, child_indent, sendData);

	*strm << indent << "}" << endl;

}


/**
 * Write the w10n json representation of the passed BAseType instance. If the
 * parameter sendData is true then include the data.
 */
void W10nJsonTransform::transform(ostream *strm, libdap::BaseType *bt, string  indent, bool sendData)
{
	switch(bt->type()){
	// Handle the atomic types - that's easy!
	case libdap::dods_byte_c:
	case libdap::dods_int16_c:
	case libdap::dods_uint16_c:
	case libdap::dods_int32_c:
	case libdap::dods_uint32_c:
	case libdap::dods_float32_c:
	case libdap::dods_float64_c:
	case libdap::dods_str_c:
	case libdap::dods_url_c:
		transformAtomic(strm, bt, indent, sendData);
		break;

	case libdap::dods_structure_c:
		transform(strm, (libdap::Structure *) bt, indent, sendData);
		break;

	case libdap::dods_grid_c:
		transform(strm, (libdap::Grid *) bt, indent, sendData);
		break;

	case libdap::dods_sequence_c:
		transform(strm, (libdap::Sequence *) bt, indent, sendData);
		break;

	case libdap::dods_array_c:
		transform(strm, (libdap::Array *) bt, indent, sendData);
		break;

	case libdap::dods_int8_c:
	case libdap::dods_uint8_c:
	case libdap::dods_int64_c:
	case libdap::dods_uint64_c:
	case libdap::dods_url4_c:
	case libdap::dods_enum_c:
	case libdap::dods_group_c:
	{
		string s = (string) "W10nJsonTransform:  DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	default:
	{
		string s = (string) "W10nJsonTransform:  Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	}

}

/**
 * Write the w10n json representation of the passed BaseType instance - which had better be one of the
 * atomic DAP types. If the parameter sendData is true then include the data.
 */
void W10nJsonTransform::transformAtomic(ostream *strm, libdap::BaseType *b, string indent, bool sendData){

	*strm << indent << "{" << endl;

	string childindent = indent + _indent_increment;

	writeLeafMetadata(strm, b, childindent);

	*strm << childindent << "\"shape\": [1]," << endl;

	if(sendData){
		// Data
		*strm << childindent << "\"data\": [";

		if(b->type() == libdap::dods_str_c || b->type() == libdap::dods_url_c ){
			// String values need to be escaped.
			std::stringstream ss;
			b->print_val(ss,"",false);
			*strm << "\"" << w10n::backslash_escape(ss.str(), '"') << "\"";
		}
		else {
			b->print_val(*strm, "", false);
		}


		*strm << "]";
	}

}



/**
 * Write the w10n json representation of the passed DAP Array instance - which had better be one of
 * atomic DAP types. If the parameter sendData is true then include the data.
 */
void W10nJsonTransform::transform(ostream *strm, libdap::Array *a, string indent, bool sendData){

    BESDEBUG(W10N_DEBUG_KEY, "W10nJsonTransform::transform() - Processing Array. "
            << " a->type(): " << a->type()
			<< " a->var()->type(): " << a->var()->type()
			<< endl);

	switch(a->var()->type()){
	// Handle the atomic types - that's easy!
	case libdap::dods_byte_c:
		json_simple_type_array<libdap::dods_byte>(strm,a,indent,sendData);
		break;

	case libdap::dods_int16_c:
		json_simple_type_array<libdap::dods_int16>(strm,a,indent,sendData);
		break;

	case libdap::dods_uint16_c:
		json_simple_type_array<libdap::dods_uint16>(strm,a,indent,sendData);
		break;

	case libdap::dods_int32_c:
		json_simple_type_array<libdap::dods_int32>(strm,a,indent,sendData);
		break;

	case libdap::dods_uint32_c:
		json_simple_type_array<libdap::dods_uint32>(strm,a,indent,sendData);
		break;

	case libdap::dods_float32_c:
		json_simple_type_array<libdap::dods_float32>(strm,a,indent,sendData);
    	break;

	case libdap::dods_float64_c:
		json_simple_type_array<libdap::dods_float64>(strm,a,indent,sendData);
		break;

	case libdap::dods_str_c:
	{
		// @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
		//json_simple_type_array<string>(strm,a,indent,sendData);
		//break;

		string s = (string) "W10nJsonTransform:  Arrays of String objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_url_c:
	{
		// @TODO Handle String and URL Arrays including backslash escaping double quotes in values.
		//json_simple_type_array<string>(strm,a,indent,sendData);
		//break;

		string s = (string) "W10nJsonTransform:  Arrays of URL objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_structure_c:
	{
		string s = (string) "W10nJsonTransform:  Arrays of Structure objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case libdap::dods_grid_c:
	{
		string s = (string) "W10nJsonTransform:  Arrays of Grid objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_sequence_c:
	{
		string s = (string) "W10nJsonTransform:  Arrays of Sequence objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	case libdap::dods_array_c:
	{
		string s = (string) "W10nJsonTransform:  Arrays of Array objects not a supported return type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}
	case libdap::dods_int8_c:
	case libdap::dods_uint8_c:
	case libdap::dods_int64_c:
	case libdap::dods_uint64_c:
	case libdap::dods_url4_c:
	case libdap::dods_enum_c:
	case libdap::dods_group_c:
	{
		string s = (string) "W10nJsonTransform:  DAP4 types not yet supported.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	default:
	{
		string s = (string) "W10nJsonTransform:  Unrecognized type.";
        throw BESInternalError(s, __FILE__, __LINE__);
		break;
	}

	}

}


/**
 * Write the w10n json representation of the passed DAP AttrTable instance.
 * Supports multi-valued attributes and nested attributes.
 */
void W10nJsonTransform::transform(ostream *strm, libdap::AttrTable &attr_table, string  indent){

	string child_indent = indent + _indent_increment;

	// Start the attributes block
	*strm << indent << "\"attributes\": [";


//	if(attr_table.get_name().length()>0)
//		*strm  << endl << child_indent << "{\"name\": \"name\", \"value\": \"" << attr_table.get_name() << "\"},";


	// Only do more if there are actually attributes in the table
	if(attr_table.get_size() != 0) {
		*strm << endl;
		libdap::AttrTable::Attr_iter begin = attr_table.attr_begin();
		libdap::AttrTable::Attr_iter end = attr_table.attr_end();


		for(libdap::AttrTable::Attr_iter at_iter=begin; at_iter !=end; at_iter++){

			switch (attr_table.get_attr_type(at_iter)){
				case libdap::Attr_container:
				{
					libdap::AttrTable *atbl = attr_table.get_attr_table(at_iter);

					// not first thing? better use a comma...
					if(at_iter != begin )
						*strm << "," << endl;

					// Attribute Containers need to be opened and then a recursive call gets made
					*strm << child_indent << "{" << endl;

					// If the table has a name, write it out as a json property.
					if(atbl->get_name().length()>0)
						*strm << child_indent + _indent_increment << "\"name\": \"" << atbl->get_name() << "\"," << endl;


					// Recursive call for child attribute table.
					transform(strm, *atbl, child_indent + _indent_increment);
					*strm << endl << child_indent << "}";

					break;

				}
				default:
				{
					// not first thing? better use a comma...
					if(at_iter != begin)
						*strm << "," << endl;

					// Open attribute object, write name
					*strm << child_indent << "{\"name\": \""<< attr_table.get_name(at_iter) << "\", ";

					// Open value array
					*strm  << "\"value\": [";
					vector<string> *values = attr_table.get_attr_vector(at_iter);
					// write values
					for(int i=0; i<values->size() ;i++){

						// not first thing? better use a comma...
						if(i>0)
							*strm << ",";

						// Escape the double quotes found in String and URL type attribute values.
						if(attr_table.get_attr_type(at_iter) == libdap::Attr_string || attr_table.get_attr_type(at_iter) == libdap::Attr_url){
							*strm << "\"";
							string value = (*values)[i] ;
							*strm << w10n::backslash_escape(value, '"') ;
							*strm << "\"";
						}
						else {

							*strm << (*values)[i] ;
						}

					}
					// close value array
					*strm << "]}";
					break;
				}

			}
		}
		*strm << endl << indent;

	}

	// close AttrTable JSON

	*strm << "]";



}








