
#ifndef A_W10NXMLCatalogCommand_h
#define A_W10NXMLCatalogCommand_h 1

#include "BESXMLCommand.h"
#include "BESDataHandlerInterface.h"

class W10NInfoCommand : public BESXMLCommand
{
public:
	                    W10NInfoCommand( const BESDataHandlerInterface &base_dhi ) ;
    virtual		       ~W10NInfoCommand() {}

    virtual void		parse_request( xmlNode *node ) ;

    virtual bool		has_response() { return true ; }

    virtual void		dump( ostream &strm ) const ;

    static BESXMLCommand *	CommandBuilder( const BESDataHandlerInterface &base_dhi ) ;
} ;

#endif // A_W10NXMLCatalogCommand_h

