/***************************************************************************
  qfattributetablemodel.h - QfAttributeTableModel

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

#ifndef QFATTRIBUTETABLEMODEL_H
#define QFATTRIBUTETABLEMODEL_H

#include <QAbstractTableModel>
#include <QPointer>
#include <qgsfeature.h>
#include <qgsvectorlayer.h>

/**
 * \brief A table of the attributes of a vector layer's features, as in QGIS' attribute table.
 *
 * Features can be filtered with a QGIS expression and sorted by any column. Values are
 * shown the way the layer's widgets represent them (e.g. value map labels) and can be
 * edited cell by cell; each edit is committed to the layer right away.
 *
 * At most featureLimit features are loaded, so very large layers stay responsive;
 * limitReached tells when more features matched than were loaded.
 *
 * \ingroup core
 */
class QfAttributeTableModel : public QAbstractTableModel
{
    Q_OBJECT

    //! The vector layer whose features are listed
    Q_PROPERTY( QgsVectorLayer *layer READ layer WRITE setLayer NOTIFY layerChanged )
    //! A QGIS expression restricting the listed features, empty to list them all
    Q_PROPERTY( QString filterExpression READ filterExpression WRITE setFilterExpression NOTIFY filterExpressionChanged )
    //! Column the features are sorted by, -1 for the provider's order
    Q_PROPERTY( int sortColumn READ sortColumn NOTIFY sortChanged )
    //! TRUE when sorting ascending
    Q_PROPERTY( bool sortAscending READ sortAscending NOTIFY sortChanged )
    //! Maximum number of features loaded
    Q_PROPERTY( int featureLimit READ featureLimit WRITE setFeatureLimit NOTIFY featureLimitChanged )
    //! TRUE when more features matched than featureLimit allows to load
    Q_PROPERTY( bool limitReached READ limitReached NOTIFY loaded )
    //! The last error (invalid filter expression, failed edit), empty when there is none
    Q_PROPERTY( QString errorString READ errorString NOTIFY errorStringChanged )
    //! TRUE when the layer allows changing attribute values
    Q_PROPERTY( bool editable READ editable NOTIFY layerChanged )

  public:
    enum Roles
    {
      ValueRole = Qt::UserRole + 1, //!< Raw attribute value
      FeatureIdRole,                //!< Id of the row's feature
      FieldEditableRole,            //!< Whether the cell's field can be edited
      IsNullRole,                   //!< Whether the value is NULL
    };
    Q_ENUM( Roles )

    explicit QfAttributeTableModel( QObject *parent = nullptr );

    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    QHash<int, QByteArray> roleNames() const override;

    QgsVectorLayer *layer() const;
    void setLayer( QgsVectorLayer *layer );

    QString filterExpression() const { return mFilterExpression; }
    void setFilterExpression( const QString &expression );

    int sortColumn() const { return mSortColumn; }
    bool sortAscending() const { return mSortAscending; }

    int featureLimit() const { return mFeatureLimit; }
    void setFeatureLimit( int limit );

    bool limitReached() const { return mLimitReached; }
    QString errorString() const { return mErrorString; }
    bool editable() const;

    //! Returns the header label of \a column: the field alias, or its name
    Q_INVOKABLE QString columnTitle( int column ) const;

    //! Returns the name of the field shown in \a column
    Q_INVOKABLE QString fieldName( int column ) const;

    //! Returns TRUE if the values in \a column can be edited
    Q_INVOKABLE bool isColumnEditable( int column ) const;

    //! Returns the id of the feature in \a row
    Q_INVOKABLE QgsFeatureId featureId( int row ) const;

    /**
     * Sorts by \a column. Sorting again by the same column flips the direction; a third
     * time goes back to the provider's order.
     */
    Q_INVOKABLE void sortByColumn( int column );

    /**
     * Sets the value of \a column for the feature in \a row to \a value (an empty string
     * sets it to NULL) and commits the change to the layer.
     * Returns FALSE and sets errorString when the change could not be saved.
     */
    Q_INVOKABLE bool setValue( int row, int column, const QVariant &value );

    /**
     * Field calculator: sets \a column of every listed feature (those matching the filter)
     * to the result of the QGIS \a expression, e.g. "$area / 10000", and commits the changes.
     * Returns the number of features updated, or -1 and sets errorString on failure, in
     * which case no feature is changed.
     */
    Q_INVOKABLE int calculateColumn( int column, const QString &expression );

    //! Loads the features again
    Q_INVOKABLE void reload();

  signals:
    void layerChanged();
    void filterExpressionChanged();
    void sortChanged();
    void featureLimitChanged();
    void errorStringChanged();
    //! Emitted after the features were (re)loaded
    void loaded();

  private:
    void setErrorString( const QString &error );
    QString representValue( int column, const QVariant &value ) const;

    QPointer<QgsVectorLayer> mLayer;
    QgsFields mFields;
    QVector<QgsFeature> mFeatures;
    QVector<QVariant> mFormatterCaches;
    QString mFilterExpression;
    int mSortColumn = -1;
    bool mSortAscending = true;
    int mFeatureLimit = 10000;
    bool mLimitReached = false;
    bool mIsSettingValue = false;
    QString mErrorString;
};

#endif // QFATTRIBUTETABLEMODEL_H
