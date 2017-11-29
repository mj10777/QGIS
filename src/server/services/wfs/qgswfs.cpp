/***************************************************************************
                              qgswms.cpp
                              -------------------------
  begin                : December 20 , 2016
  copyright            : (C) 2007 by Marco Hugentobler  ( parts fron qgswmshandler)
                         (C) 2012 by René-Luc D'Hont    ( parts fron qgswmshandler)
                         (C) 2014 by Alessandro Pasotti ( parts from qgswmshandler)
                         (C) 2016 by David Marteau
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
                         a dot pasotti at itopen dot it
                         david dot marteau at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmodule.h"
#include "qgswfsutils.h"
#include "qgswfsgetcapabilities.h"
#include "qgswfsgetcapabilities_1_0_0.h"
#include "qgswfsgetfeature.h"
#include "qgswfsdescribefeaturetype.h"
#include "qgswfstransaction.h"
#include "qgswfstransaction_1_0_0.h"

#define QSTR_COMPARE( str, lit )\
  (str.compare( QStringLiteral( lit ), Qt::CaseInsensitive ) == 0)

namespace QgsWfs
{

  class Service: public QgsService
  {
    public:
      // Constructor
      Service( QgsServerInterface *serverIface )
        : mServerIface( serverIface )
      {}

      QString name()    const { return QStringLiteral( "WFS" ); }
      QString version() const { return implementationVersion(); }

      bool allowMethod( QgsServerRequest::Method method ) const
      {
        return method == QgsServerRequest::GetMethod || method == QgsServerRequest::PostMethod;
      }

      void executeRequest( const QgsServerRequest &request, QgsServerResponse &response,
                           const QgsProject *project )
      {
        QgsServerRequest::Parameters params = request.parameters();
        QString versionString = params.value( "VERSION" );

        // Set the default version
        if ( versionString.isEmpty() )
        {
          versionString = version(); // defined in qgswfsutils.h
        }

        // Get the request
        QString req = params.value( QStringLiteral( "REQUEST" ) );
        if ( req.isEmpty() )
        {
          throw QgsServiceException( QStringLiteral( "OperationNotSupported" ),
                                     QStringLiteral( "Please check the value of the REQUEST parameter" ) );
        }

        if ( QSTR_COMPARE( req, "GetCapabilities" ) )
        {
          // Supports WFS 1.0.0
          if ( QSTR_COMPARE( versionString, "1.0.0" ) )
          {
            v1_0_0::writeGetCapabilities( mServerIface, project, versionString, request, response );
          }
          else
          {
            writeGetCapabilities( mServerIface, project, versionString, request, response );
          }
        }
        else if ( QSTR_COMPARE( req, "GetFeature" ) )
        {
          writeGetFeature( mServerIface, project, versionString, request, response );
        }
        else if ( QSTR_COMPARE( req, "DescribeFeatureType" ) )
        {
          writeDescribeFeatureType( mServerIface, project, versionString, request, response );
        }
        else if ( QSTR_COMPARE( req, "Transaction" ) )
        {
          // Supports WFS 1.0.0
          if ( QSTR_COMPARE( versionString, "1.0.0" ) )
          {
            v1_0_0::writeTransaction( mServerIface, project, versionString, request, response );
          }
          else
          {
            writeTransaction( mServerIface, project, versionString, request, response );
          }
        }
        else
        {
          // Operation not supported
          throw QgsServiceException( QStringLiteral( "OperationNotSupported" ),
                                     QStringLiteral( "Request %1 is not supported" ).arg( req ) );
        }
      }

    private:
      QgsServerInterface *mServerIface = nullptr;
  };


} // namespace QgsWfs


// Module
class QgsWfsModule: public QgsServiceModule
{
  public:
    void registerSelf( QgsServiceRegistry &registry, QgsServerInterface *serverIface )
    {
      QgsDebugMsg( "WFSModule::registerSelf called" );
      registry.registerService( new  QgsWfs::Service( serverIface ) );
    }
};


// Entry points
QGISEXTERN QgsServiceModule *QGS_ServiceModule_Init()
{
  static QgsWfsModule module;
  return &module;
}
QGISEXTERN void QGS_ServiceModule_Exit( QgsServiceModule * )
{
  // Nothing to do
}





