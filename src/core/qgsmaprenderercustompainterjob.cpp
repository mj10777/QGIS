/***************************************************************************
  qgsmaprenderercustompainterjob.cpp
  --------------------------------------
  Date                 : December 2013
  Copyright            : (C) 2013 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaprenderercustompainterjob.h"

#include "qgsfeedback.h"
#include "qgslabelingengine.h"
#include "qgslogger.h"
#include "qgsproject.h"
#include "qgsmaplayerrenderer.h"
#include "qgsvectorlayer.h"
#include "qgsrenderer.h"
#include "qgsmaplayerlistutils.h"

#include <QtConcurrentRun>

QgsMapRendererCustomPainterJob::QgsMapRendererCustomPainterJob( const QgsMapSettings &settings, QPainter *painter )
  : QgsMapRendererJob( settings )
  , mPainter( painter )
  , mActive( false )
  , mRenderSynchronously( false )
{
  QgsDebugMsgLevel( "QPAINTER construct", 5 );
}

QgsMapRendererCustomPainterJob::~QgsMapRendererCustomPainterJob()
{
  QgsDebugMsgLevel( "QPAINTER destruct", 5 );
  Q_ASSERT( !mFutureWatcher.isRunning() );
  //cancel();
}

void QgsMapRendererCustomPainterJob::start()
{
  if ( isActive() )
    return;

  mRenderingStart.start();

  mActive = true;

  mErrors.clear();

  QgsDebugMsgLevel( "QPAINTER run!", 5 );

  QgsDebugMsgLevel( "Preparing list of layer jobs for rendering", 5 );
  QTime prepareTime;
  prepareTime.start();

  // clear the background
  mPainter->fillRect( 0, 0, mSettings.outputSize().width(), mSettings.outputSize().height(), mSettings.backgroundColor() );

  mPainter->setRenderHint( QPainter::Antialiasing, mSettings.testFlag( QgsMapSettings::Antialiasing ) );

#ifndef QT_NO_DEBUG
  QPaintDevice *paintDevice = mPainter->device();
  QString errMsg = QStringLiteral( "pre-set DPI not equal to painter's DPI (%1 vs %2)" ).arg( paintDevice->logicalDpiX() ).arg( mSettings.outputDpi() );
  Q_ASSERT_X( qgsDoubleNear( paintDevice->logicalDpiX(), mSettings.outputDpi() ), "Job::startRender()", errMsg.toLatin1().data() );
#endif

  mLabelingEngineV2.reset();

  if ( mSettings.testFlag( QgsMapSettings::DrawLabeling ) )
  {
    mLabelingEngineV2.reset( new QgsLabelingEngine() );
    mLabelingEngineV2->setMapSettings( mSettings );
  }

  bool canUseLabelCache = prepareLabelCache();
  mLayerJobs = prepareJobs( mPainter, mLabelingEngineV2.get() );
  mLabelJob = prepareLabelingJob( mPainter, mLabelingEngineV2.get(), canUseLabelCache );

  QgsDebugMsgLevel( "Rendering prepared in (seconds): " + QString( "%1" ).arg( prepareTime.elapsed() / 1000.0 ), 4 );

  if ( mRenderSynchronously )
  {
    // do the rendering right now!
    doRender();
    return;
  }

  // now we are ready to start rendering!
  connect( &mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsMapRendererCustomPainterJob::futureFinished );

  mFuture = QtConcurrent::run( staticRender, this );
  mFutureWatcher.setFuture( mFuture );
}


void QgsMapRendererCustomPainterJob::cancel()
{
  if ( !isActive() )
  {
    QgsDebugMsgLevel( "QPAINTER not running!", 4 );
    return;
  }

  QgsDebugMsgLevel( "QPAINTER canceling", 5 );
  disconnect( &mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsMapRendererCustomPainterJob::futureFinished );
  cancelWithoutBlocking();

  QTime t;
  t.start();

  mFutureWatcher.waitForFinished();

  QgsDebugMsgLevel( QString( "QPAINER cancel waited %1 ms" ).arg( t.elapsed() / 1000.0 ), 5 );

  futureFinished();

  QgsDebugMsgLevel( "QPAINTER canceled", 5 );
}

void QgsMapRendererCustomPainterJob::cancelWithoutBlocking()
{
  if ( !isActive() )
  {
    QgsDebugMsg( "QPAINTER not running!" );
    return;
  }

  mLabelJob.context.setRenderingStopped( true );
  for ( LayerRenderJobs::iterator it = mLayerJobs.begin(); it != mLayerJobs.end(); ++it )
  {
    it->context.setRenderingStopped( true );
    if ( it->renderer && it->renderer->feedback() )
      it->renderer->feedback()->cancel();
  }
}

void QgsMapRendererCustomPainterJob::waitForFinished()
{
  if ( !isActive() )
    return;

  disconnect( &mFutureWatcher, &QFutureWatcher<void>::finished, this, &QgsMapRendererCustomPainterJob::futureFinished );

  QTime t;
  t.start();

  mFutureWatcher.waitForFinished();

  QgsDebugMsgLevel( QString( "waitForFinished: %1 ms" ).arg( t.elapsed() / 1000.0 ), 4 );

  futureFinished();
}

bool QgsMapRendererCustomPainterJob::isActive() const
{
  return mActive;
}

bool QgsMapRendererCustomPainterJob::usedCachedLabels() const
{
  return mLabelJob.cached;
}

QgsLabelingResults *QgsMapRendererCustomPainterJob::takeLabelingResults()
{
  if ( mLabelingEngineV2 )
    return mLabelingEngineV2->takeResults();
  else
    return nullptr;
}


void QgsMapRendererCustomPainterJob::waitForFinishedWithEventLoop( QEventLoop::ProcessEventsFlags flags )
{
  QEventLoop loop;
  connect( &mFutureWatcher, &QFutureWatcher<void>::finished, &loop, &QEventLoop::quit );
  loop.exec( flags );
}


void QgsMapRendererCustomPainterJob::renderSynchronously()
{
  mRenderSynchronously = true;
  start();
  futureFinished();
  mRenderSynchronously = false;
}


void QgsMapRendererCustomPainterJob::futureFinished()
{
  mActive = false;
  mRenderingTime = mRenderingStart.elapsed();
  QgsDebugMsgLevel( "QPAINTER futureFinished", 5 );

  logRenderingTime( mLayerJobs, mLabelJob );

  // final cleanup
  cleanupJobs( mLayerJobs );
  cleanupLabelJob( mLabelJob );

  emit finished();
}


void QgsMapRendererCustomPainterJob::staticRender( QgsMapRendererCustomPainterJob *self )
{
  try
  {
    self->doRender();
  }
  catch ( QgsException &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( "Caught unhandled QgsException: " + e.what() );
  }
  catch ( std::exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( "Caught unhandled std::exception: " + QString::fromLatin1( e.what() ) );
  }
  catch ( ... )
  {
    QgsDebugMsg( "Caught unhandled unknown exception" );
  }
}

void QgsMapRendererCustomPainterJob::doRender()
{
  QgsDebugMsgLevel( "Starting to render layer stack.", 5 );
  QTime renderTime;
  renderTime.start();

  for ( LayerRenderJobs::iterator it = mLayerJobs.begin(); it != mLayerJobs.end(); ++it )
  {
    LayerRenderJob &job = *it;

    if ( job.context.renderingStopped() )
      break;

    if ( job.context.useAdvancedEffects() )
    {
      // Set the QPainter composition mode so that this layer is rendered using
      // the desired blending mode
      mPainter->setCompositionMode( job.blendMode );
    }

    if ( !job.cached )
    {
      QTime layerTime;
      layerTime.start();

      if ( job.img )
      {
        job.img->fill( 0 );
        job.imageInitialized = true;
      }

      job.renderer->render();

      job.renderingTime = layerTime.elapsed();
    }

    if ( job.img )
    {
      // If we flattened this layer for alternate blend modes, composite it now
      mPainter->setOpacity( job.opacity );
      mPainter->drawImage( 0, 0, *job.img );
      mPainter->setOpacity( 1.0 );
    }

  }

  QgsDebugMsgLevel( "Done rendering map layers", 5 );

  if ( mSettings.testFlag( QgsMapSettings::DrawLabeling ) && !mLabelJob.context.renderingStopped() )
  {
    if ( !mLabelJob.cached )
    {
      QTime labelTime;
      labelTime.start();

      if ( mLabelJob.img )
      {
        QPainter painter;
        mLabelJob.img->fill( 0 );
        painter.begin( mLabelJob.img );
        mLabelJob.context.setPainter( &painter );
        drawLabeling( mSettings, mLabelJob.context, mLabelingEngineV2.get(), &painter );
        painter.end();
      }
      else
      {
        drawLabeling( mSettings, mLabelJob.context, mLabelingEngineV2.get(), mPainter );
      }

      mLabelJob.complete = true;
      mLabelJob.renderingTime = labelTime.elapsed();
      mLabelJob.participatingLayers = _qgis_listRawToQPointer( mLabelingEngineV2->participatingLayers() );
    }
  }
  if ( mLabelJob.img && mLabelJob.complete )
  {
    mPainter->setCompositionMode( QPainter::CompositionMode_SourceOver );
    mPainter->setOpacity( 1.0 );
    mPainter->drawImage( 0, 0, *mLabelJob.img );
  }

  QgsDebugMsg( "Rendering completed in (seconds): " + QString( "%1" ).arg( renderTime.elapsed() / 1000.0 ) );
}


void QgsMapRendererJob::drawLabeling( const QgsMapSettings &settings, QgsRenderContext &renderContext, QgsLabelingEngine *labelingEngine2, QPainter *painter )
{
  QgsDebugMsgLevel( "Draw labeling start", 5 );

  QTime t;
  t.start();

  // Reset the composition mode before rendering the labels
  painter->setCompositionMode( QPainter::CompositionMode_SourceOver );

  // TODO: this is not ideal - we could override rendering stopped flag that has been set in meanwhile
  renderContext = QgsRenderContext::fromMapSettings( settings );
  renderContext.setPainter( painter );

  if ( labelingEngine2 )
  {
    // set correct extent
    renderContext.setExtent( settings.visibleExtent() );
    renderContext.setCoordinateTransform( QgsCoordinateTransform() );

    labelingEngine2->run( renderContext );
  }

  QgsDebugMsg( QString( "Draw labeling took (seconds): %1" ).arg( t.elapsed() / 1000. ) );
}


bool QgsMapRendererJob::needTemporaryImage( QgsMapLayer *ml )
{
  if ( ml->type() == QgsMapLayer::VectorLayer )
  {
    QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( ml );
    if ( vl->renderer() && vl->renderer()->forceRasterRender() )
    {
      //raster rendering is forced for this layer
      return true;
    }
    if ( mSettings.testFlag( QgsMapSettings::UseAdvancedEffects ) &&
         ( ( vl->blendMode() != QPainter::CompositionMode_SourceOver )
           || ( vl->featureBlendMode() != QPainter::CompositionMode_SourceOver )
           || ( !qgsDoubleNear( vl->opacity(), 1.0 ) ) ) )
    {
      //layer properties require rasterization
      return true;
    }
  }
  else if ( ml->type() == QgsMapLayer::RasterLayer )
  {
    // preview of intermediate raster rendering results requires a temporary output image
    if ( mSettings.testFlag( QgsMapSettings::RenderPartialOutput ) )
      return true;
  }

  return false;
}

