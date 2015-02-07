// -*- mode: c++; c-basic-offset:4 -*-
//
// FoW10nJsonTransform.cc
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

#ifndef W10NJSONTRANSFORM_H_
#define W10NJSONTRANSFORM_H_

#include <string>
#include <vector>
#include <map>


#include <BaseType.h>
#include <DDS.h>
#include <Array.h>


#include <BESObj.h>
#include <BESDataHandlerInterface.h>

/**
 * Used to transform a DDS into a w10n JSON metadata or w10n JSON data document.
 * The output is written to a local file whose name is passed as a parameter
 * to the constructor.
 */
class W10nJsonTransform: public BESObj {
private:
	libdap::DDS *_dds;
	std::string _localfile;
	std::string _returnAs;
	std::string _indent_increment;

	std::ostream *_ostrm;

	void writeNodeMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
	void writeLeafMetadata(std::ostream *strm, libdap::BaseType *bt, std::string indent);
	void writeDatasetMetadata(std::ostream *strm, libdap::DDS *dds, std::string indent);

	void transformAtomic(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);


	void transform(std::ostream *strm, libdap::DDS *dds, std::string indent, bool sendData);
	void transform(std::ostream *strm, libdap::BaseType *bt, std::string indent, bool sendData);

    //void transform(std::ostream *strm, Structure *s,string indent );
    //void transform(std::ostream *strm, Grid *g, string indent);
    //void transform(std::ostream *strm, Sequence *s, string indent);
	void transform(std::ostream *strm, libdap::Constructor *cnstrctr, std::string indent, bool sendData);
	void transform_node_worker(std::ostream *strm, std::vector<libdap::BaseType *> leaves, std::vector<libdap::BaseType *> nodes, std::string indent, bool sendData);


    void transform(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);
    void transform(std::ostream *strm, libdap::AttrTable &attr_table, std::string  indent);

    template<typename T>
    void json_simple_type_array(std::ostream *strm, libdap::Array *a, std::string indent, bool sendData);

    template<typename T>
    unsigned  int json_simple_type_array_worker(
    		std::ostream *strm,
    		T *values,
    		unsigned int indx,
    		std::vector<unsigned int> *shape,
    		unsigned int currentDim
    		);



public:

    W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const std::string &localfile);
    W10nJsonTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, std::ostream *ostrm);
	virtual ~W10nJsonTransform();

	virtual void transform(bool sendData);

	virtual void dump(std::ostream &strm) const;

};

#endif /* W10NJSONTRANSFORM_H_ */
