/***************************************************************************
  qfattributetablemodel.cpp - QfAttributeTableModel

 ---------------------
 begin                : November 2026
 copyright            : (C) 2026 by GeoCampo contributors
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qfattributetablemodel.h"

#include <qgsapplication.h>
#include <qgseditorwidgetsetup.h>
#include <qgsexpression.h>
#include <qgsexpressioncontextutils.h>
#include <qgsfeatureiterator.h>
#include <qgsfeaturerequest.h>
#include <qgsfieldformatter.h>
#include <qgsfieldformatterregistry.h>
#include <qgsvariantutils.h>
#include <qgsvectordataprovider.h>

QfAttributeTableModel::QfAttributeTableModel( QObject *parent )
  : QAbstractTableModel( parent )
{
}

int QfAttributeTableModel::rowCount( const QModelIndex &parent ) const
{
  return parent.isValid() ? 0 : static_cast<int>( mFeatures.size() );
}

int QfAttributeTableModel::columnCount( const QModelIndex &parent ) const
{
  return parent.isValid() ? 0 : mFields.count();
}

QVariant QfAttributeTableModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() || index.row() >= mFeatures.size() || index.column() >= mFields.count() )
    return QVariant();

  const QgsFeature &feature = mFeatures.at( index.row() );
  const QVariant value = feature.attribute( index.column() );

  switch ( role )
  {
    case Qt::DisplayRole:
      return representValue( index.column(), value );
    case ValueRole:
      return value;
    case FeatureIdRole:
      return feature.id();
    case FieldEditableRole:
      return isColumnEditable( index.column() );
    case IsNullRole:
      return QgsVariantUtils::isNull( value );
    default:
      return QVariant();
  }
}

QVariant QfAttributeTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( role != Qt::DisplayRole )
    return QVariant();

  if ( orientation == Qt::Horizontal )
    return columnTitle( section );

  return section + 1;
}

QHash<int, QByteArray> QfAttributeTableModel::roleNames() const
{
  QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
  roles[ValueRole] = "value";
  roles[FeatureIdRole] = "featureId";
  roles[FieldEditableRole] = "fieldEditable";
  roles[IsNullRole] = "isNull";
  return roles;
}

QgsVectorLayer *QfAttributeTableModel::layer() const
{
  return mLayer.data();
}

void QfAttributeTableModel::setLayer( QgsVectorLayer *layer )
{
  if ( mLayer == layer )
    return;

  if ( mLayer )
    disconnect( mLayer, nullptr, this, nullptr );

  mLayer = layer;
  mSortColumn = -1;
  mSortAscending = true;

  if ( mLayer )
  {
    // Edits made elsewhere in the app (feature form, digitizing) show up once committed
    const auto reloadUnlessOwnEdit = [this] {
      if ( !mIsSettingValue )
        reload();
    };
    connect( mLayer, &QgsVectorLayer::afterCommitChanges, this, reloadUnlessOwnEdit );
    connect( mLayer, &QgsVectorLayer::afterRollBack, this, reloadUnlessOwnEdit );
    connect( mLayer, &QgsVectorLayer::updatedFields, this, reloadUnlessOwnEdit );
  }

  emit layerChanged();
  emit sortChanged();
  reload();
}

void QfAttributeTableModel::setFilterExpression( const QString &expression )
{
  if ( mFilterExpression == expression )
    return;

  mFilterExpression = expression;
  emit filterExpressionChanged();
  reload();
}

void QfAttributeTableModel::setFeatureLimit( int limit )
{
  if ( mFeatureLimit == limit || limit <= 0 )
    return;

  mFeatureLimit = limit;
  emit featureLimitChanged();
  reload();
}

bool QfAttributeTableModel::editable() const
{
  if ( !mLayer || !mLayer->dataProvider() )
    return false;

  const bool isLocked = mLayer->readOnly() || mLayer->customProperty( QStringLiteral( "QFieldSync/is_attribute_editing_locked" ), false ).toBool();
  const bool isCapable = mLayer->dataProvider()->capabilities() & Qgis::VectorProviderCapability::ChangeAttributeValues;
  return !isLocked && isCapable;
}

QString QfAttributeTableModel::columnTitle( int column ) const
{
  if ( column < 0 || column >= mFields.count() )
    return QString();

  return mFields.at( column ).displayName();
}

QString QfAttributeTableModel::fieldName( int column ) const
{
  if ( column < 0 || column >= mFields.count() )
    return QString();

  return mFields.at( column ).name();
}

bool QfAttributeTableModel::isColumnEditable( int column ) const
{
  if ( !editable() || column < 0 || column >= mFields.count() )
    return false;

  // Virtual (expression) and joined fields are computed, not stored
  const Qgis::FieldOrigin origin = mFields.fieldOrigin( column );
  if ( origin == Qgis::FieldOrigin::Expression || origin == Qgis::FieldOrigin::Join )
    return false;

  return !mLayer->editFormConfig().readOnly( column );
}

QgsFeatureId QfAttributeTableModel::featureId( int row ) const
{
  if ( row < 0 || row >= mFeatures.size() )
    return FID_NULL;

  return mFeatures.at( row ).id();
}

void QfAttributeTableModel::sortByColumn( int column )
{
  if ( column < 0 || column >= mFields.count() )
    return;

  if ( mSortColumn != column )
  {
    mSortColumn = column;
    mSortAscending = true;
  }
  else if ( mSortAscending )
  {
    mSortAscending = false;
  }
  else
  {
    mSortColumn = -1;
    mSortAscending = true;
  }

  emit sortChanged();
  reload();
}

bool QfAttributeTableModel::setValue( int row, int column, const QVariant &value )
{
  if ( !mLayer || row < 0 || row >= mFeatures.size() || !isColumnEditable( column ) )
    return false;

  const QgsField field = mFields.at( column );
  QVariant newValue = value;
  if ( !newValue.isValid() || ( newValue.userType() == QMetaType::QString && newValue.toString().isEmpty() ) )
  {
    newValue = QgsVariantUtils::createNullVariant( field.type() );
  }
  else
  {
    QString error;
    if ( !field.convertCompatible( newValue, &error ) )
    {
      setErrorString( tr( "Invalid value for field %1: %2" ).arg( field.displayName(), error ) );
      return false;
    }
  }

  QgsFeature &feature = mFeatures[row];
  const QVariant oldValue = feature.attribute( column );
  if ( oldValue == newValue && QgsVariantUtils::isNull( oldValue ) == QgsVariantUtils::isNull( newValue ) )
    return true;

  mIsSettingValue = true;
  const bool wasEditing = mLayer->isEditable();
  bool success = wasEditing || mLayer->startEditing();
  if ( success )
    success = mLayer->changeAttributeValue( feature.id(), column, newValue, oldValue );
  if ( success && !wasEditing )
  {
    success = mLayer->commitChanges();
    if ( !success )
    {
      setErrorString( tr( "Could not save the change: %1" ).arg( mLayer->commitErrors().join( QStringLiteral( "\n" ) ) ) );
      mLayer->rollBack();
    }
  }
  else if ( !success )
  {
    setErrorString( tr( "Could not change the value of field %1" ).arg( field.displayName() ) );
    if ( !wasEditing && mLayer->isEditable() )
      mLayer->rollBack();
  }
  mIsSettingValue = false;

  if ( success )
  {
    feature.setAttribute( column, newValue );
    const QModelIndex changed = index( row, column );
    emit dataChanged( changed, changed );
    setErrorString( QString() );
  }
  return success;
}

int QfAttributeTableModel::calculateColumn( int column, const QString &expression )
{
  if ( !mLayer || !isColumnEditable( column ) )
  {
    setErrorString( tr( "This field cannot be edited" ) );
    return -1;
  }
  if ( mFeatures.isEmpty() )
    return 0;

  QgsExpression calculation( expression );
  if ( calculation.hasParserError() )
  {
    setErrorString( tr( "Invalid expression: %1" ).arg( calculation.parserErrorString() ) );
    return -1;
  }

  QgsExpressionContext context( QgsExpressionContextUtils::globalProjectLayerScopes( mLayer ) );
  calculation.prepare( &context );

  QgsFeatureIds ids;
  for ( const QgsFeature &feature : std::as_const( mFeatures ) )
    ids << feature.id();

  // Fetch the complete features: the expression may use any field or the geometry ($area, $length...)
  QgsFeatureRequest request( ids );
  if ( !calculation.needsGeometry() )
    request.setFlags( Qgis::FeatureRequestFlag::NoGeometry );

  const QgsField field = mFields.at( column );
  QHash<QgsFeatureId, QVariant> newValues;
  QgsFeatureIterator it = mLayer->getFeatures( request );
  QgsFeature feature;
  while ( it.nextFeature( feature ) )
  {
    context.setFeature( feature );
    QVariant value = calculation.evaluate( &context );
    if ( calculation.hasEvalError() )
    {
      setErrorString( tr( "Expression error: %1" ).arg( calculation.evalErrorString() ) );
      return -1;
    }
    QString conversionError;
    if ( !QgsVariantUtils::isNull( value ) && !field.convertCompatible( value, &conversionError ) )
    {
      setErrorString( tr( "The result cannot be stored in field %1: %2" ).arg( field.displayName(), conversionError ) );
      return -1;
    }
    newValues.insert( feature.id(), QgsVariantUtils::isNull( value ) ? QgsVariantUtils::createNullVariant( field.type() ) : value );
  }

  mIsSettingValue = true;
  const bool wasEditing = mLayer->isEditable();
  bool success = wasEditing || mLayer->startEditing();
  if ( success )
  {
    for ( auto it = newValues.constBegin(); it != newValues.constEnd() && success; ++it )
      success = mLayer->changeAttributeValue( it.key(), column, it.value() );
  }
  if ( success && !wasEditing )
  {
    success = mLayer->commitChanges();
    if ( !success )
      setErrorString( tr( "Could not save the change: %1" ).arg( mLayer->commitErrors().join( QStringLiteral( "\n" ) ) ) );
  }
  else if ( !success )
  {
    setErrorString( tr( "Could not change the value of field %1" ).arg( field.displayName() ) );
  }
  if ( !success && !wasEditing && mLayer->isEditable() )
    mLayer->rollBack();
  mIsSettingValue = false;

  // reload() resets the error string, keep the failure reason visible
  const QString failure = success ? QString() : mErrorString;
  reload();
  setErrorString( failure );
  return success ? static_cast<int>( newValues.size() ) : -1;
}

void QfAttributeTableModel::reload()
{
  beginResetModel();
  mFeatures.clear();
  mFormatterCaches.clear();
  mFields = mLayer ? mLayer->fields() : QgsFields();
  mLimitReached = false;

  QString error;
  if ( mLayer )
  {
    for ( int i = 0; i < mFields.count(); ++i )
    {
      const QgsEditorWidgetSetup setup = mLayer->editorWidgetSetup( i );
      QgsFieldFormatter *formatter = QgsApplication::fieldFormatterRegistry()->fieldFormatter( setup.type() );
      mFormatterCaches << formatter->createCache( mLayer, i, setup.config() );
    }

    QgsFeatureRequest request;
    request.setFlags( Qgis::FeatureRequestFlag::NoGeometry );
    // One extra feature tells whether the limit was reached
    request.setLimit( mFeatureLimit + 1 );

    QgsExpressionContext context( QgsExpressionContextUtils::globalProjectLayerScopes( mLayer ) );
    request.setExpressionContext( context );

    if ( !mFilterExpression.trimmed().isEmpty() )
    {
      QgsExpression expression( mFilterExpression );
      if ( expression.hasParserError() )
        error = tr( "Invalid filter expression: %1" ).arg( expression.parserErrorString() );
      else
        request.setFilterExpression( mFilterExpression );
    }

    if ( mSortColumn >= 0 && mSortColumn < mFields.count() )
    {
      request.setOrderBy( QgsFeatureRequest::OrderBy( { QgsFeatureRequest::OrderByClause( QgsExpression::quotedColumnRef( mFields.at( mSortColumn ).name() ), mSortAscending ) } ) );
    }

    if ( error.isEmpty() )
    {
      QgsFeatureIterator it = mLayer->getFeatures( request );
      QgsFeature feature;
      while ( it.nextFeature( feature ) )
      {
        if ( mFeatures.size() == mFeatureLimit )
        {
          mLimitReached = true;
          break;
        }
        mFeatures << feature;
      }
    }
  }

  endResetModel();
  setErrorString( error );
  emit loaded();
}

void QfAttributeTableModel::setErrorString( const QString &error )
{
  if ( mErrorString == error )
    return;

  mErrorString = error;
  emit errorStringChanged();
}

QString QfAttributeTableModel::representValue( int column, const QVariant &value ) const
{
  if ( !mLayer )
    return QString();

  if ( QgsVariantUtils::isNull( value ) )
    return QStringLiteral( "NULL" );

  const QgsEditorWidgetSetup setup = mLayer->editorWidgetSetup( column );
  QgsFieldFormatter *formatter = QgsApplication::fieldFormatterRegistry()->fieldFormatter( setup.type() );
  const QVariant cache = column < mFormatterCaches.size() ? mFormatterCaches.at( column ) : QVariant();
  return formatter->representValue( mLayer, column, setup.config(), cache, value );
}
