#include "CpPlotCurve.h"
#include "qwt_clipper.h"
#include "qwt_color_map.h"
#include "qwt_point_mapper.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include <qpainter.h>

class CpPlotCurve::PrivateData
{
public:
    PrivateData():
        colorRange( 0.0, 1000.0 ),
        penWidth(2.0),
        paintAttributes( CpPlotCurve::ClipPoints )
    {
        colorMap = new QwtLinearColorMap();
    }

    ~PrivateData()
    {
        delete colorMap;
    }

    QwtColorMap *colorMap;
    QwtInterval colorRange;
    QVector<QRgb> colorTable;
    double penWidth;
    CpPlotCurve::PaintAttributes paintAttributes;
};

CpPlotCurve::CpPlotCurve( const QwtText &title ):
    QwtPlotSeriesItem( title )
{
    init();
}

CpPlotCurve::CpPlotCurve( const QString &title ):
    QwtPlotSeriesItem( QwtText( title ) )
{
    init();
}

CpPlotCurve::~CpPlotCurve()
{
    delete d_data;
}

void
CpPlotCurve::init()
{
    setItemAttribute( QwtPlotItem::Legend );
    setItemAttribute( QwtPlotItem::AutoScale );

    d_data = new PrivateData;
    setData( new QwtPoint3DSeriesData() );

    setZ( 20.0 );
}

int
CpPlotCurve::rtti() const
{
    return 1001;
}

void
CpPlotCurve::setPaintAttribute( PaintAttribute attribute, bool on )
{
    if ( on )
        d_data->paintAttributes |= attribute;
    else
        d_data->paintAttributes &= ~attribute;
}

bool
CpPlotCurve::testPaintAttribute( PaintAttribute attribute ) const
{
    return ( d_data->paintAttributes & attribute );
}

void
CpPlotCurve::setSamples( const QVector<QwtPoint3D> &samples )
{
    setData( new QwtPoint3DSeriesData( samples ) );
}

void
CpPlotCurve::setSamples( QwtSeriesData<QwtPoint3D> *data )
{
    setData( data );
}  

void
CpPlotCurve::setColorMap( QwtColorMap *colorMap )
{
    if ( colorMap != d_data->colorMap )
    {
        delete d_data->colorMap;
        d_data->colorMap = colorMap;
    }

    legendChanged();
    itemChanged();
}

const QwtColorMap *
CpPlotCurve::colorMap() const
{
    return d_data->colorMap;
}

void
CpPlotCurve::setColorRange( const QwtInterval &interval )
{
    if ( interval != d_data->colorRange )
    {
        d_data->colorRange = interval;

        legendChanged();
        itemChanged();
    }
}

QwtInterval &
CpPlotCurve::colorRange() const
{
    return d_data->colorRange;
}

void CpPlotCurve::setPenWidth(double penWidth)
{
    if ( penWidth < 0.0 )
        penWidth = 0.0;

    if ( d_data->penWidth != penWidth )
    {
        d_data->penWidth = 10;//penWidth;

        legendChanged();
        itemChanged();
    }
}

double CpPlotCurve::penWidth() const
{
    return d_data->penWidth;
}

/*!
  Draw a subset of the points

  \param painter Painter
  \param xMap Maps x-values into pixel coordinates.
  \param yMap Maps y-values into pixel coordinates.
  \param canvasRect Contents rectangle of the canvas
  \param from Index of the first sample to be painted
  \param to Index of the last sample to be painted. If to < 0 the
         series will be painted to its last sample.

  \sa drawDots()
*/
void
CpPlotCurve::drawSeries( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    if ( !painter || dataSize() <= 0 )
        return;

    if ( to < 0 )
        to = dataSize() - 1;

    if ( from < 0 )
        from = 0;

    if ( from > to )
        return;

    drawDots( painter, xMap, yMap, canvasRect, from, to );

    // Not working, to debug...
    //drawLines( painter, xMap, yMap, canvasRect, from, to );
}

/*!
  Draw a subset of the points

  \param painter Painter
  \param xMap Maps x-values into pixel coordinates.
  \param yMap Maps y-values into pixel coordinates.
  \param canvasRect Contents rectangle of the canvas
  \param from Index of the first sample to be painted
  \param to Index of the last sample to be painted. If to < 0 the
         series will be painted to its last sample.

  \sa drawSeries()
*/
void CpPlotCurve::drawDots( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    if ( !d_data->colorRange.isValid() )
        return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    const QwtColorMap::Format format = d_data->colorMap->format();
    if ( format == QwtColorMap::Indexed )
        d_data->colorTable = d_data->colorMap->colorTable( d_data->colorRange );

    const QwtSeriesData<QwtPoint3D> *series = data();

    for ( int i = from; i <= to; i++ )
    {
        const QwtPoint3D sample = series->sample( i );

        double xi = xMap.transform( sample.x() );
        double yi = yMap.transform( sample.y() );
        if ( doAlign )
        {
            xi = qRound( xi );
            yi = qRound( yi );
        }

        if ( d_data->paintAttributes & CpPlotCurve::ClipPoints )
        {
            if ( !canvasRect.contains( xi, yi ) )
                continue;
        }

        if ( format == QwtColorMap::RGB )
        {
            const QRgb rgb = d_data->colorMap->rgb(
                d_data->colorRange, sample.z() );

            painter->setPen( QPen( QColor::fromRgba( rgb ), d_data->penWidth ) );
        }
        else
        {
            const unsigned char index = d_data->colorMap->colorIndex(
                d_data->colorRange, sample.z() );

            painter->setPen( QPen( QColor::fromRgba( d_data->colorTable[index] ), 
                d_data->penWidth ) );
        }

        QwtPainter::drawPoint( painter, QPointF( xi, yi ) );
    }

    d_data->colorTable.clear();
}

void
CpPlotCurve::drawLines( QPainter *painter,
     const QwtScaleMap &xMap, const QwtScaleMap &yMap,
     const QRectF &canvasRect, int from, int to ) const
{
    if ( !d_data->colorRange.isValid() )
       return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    const QwtColorMap::Format format = d_data->colorMap->format();
    if ( format == QwtColorMap::Indexed )
        d_data->colorTable = d_data->colorMap->colorTable( d_data->colorRange );

    const QwtSeriesData<QwtPoint3D> *series = data();

    for ( int i = from; i < to; i++ )
    {
        const QwtPoint3D sample = series->sample( i );

        double xi = xMap.transform( sample.x() );
        double yi = yMap.transform( sample.y() );
        if ( doAlign )
        {
            xi = qRound( xi );
            yi = qRound( yi );
        }

        int y=i+1;
        double xi1 = xi;
        double yi1 = yi;
        QwtPoint3D nextSample;

        do {
            nextSample = series->sample( y );

            xi1 = xMap.transform( nextSample.x() );
            yi1 = yMap.transform( nextSample.y() );

            if ( doAlign )
            {
                xi1 = qRound( xi1 );
                yi1 = qRound( yi1 );
            }
            y++;
        } while (y < to && xi1 == xi &&  yi1 == yi );


        //qDebug() << "x" << xi << xi1;
        //qDebug() << "y" << yi << yi1;


        if ( d_data->paintAttributes & CpPlotCurve::ClipPoints )
        {
            if ( !canvasRect.contains( xi, yi ) )
                continue;
        }

        QwtScaleMap _xMap = QwtScaleMap(xMap);
        _xMap.setPaintInterval(xi, xi1);
        //_xMap.setPaintInterval(sample.x(), nextSample.x());
        QwtScaleMap _yMap = QwtScaleMap(yMap);
        _yMap.setPaintInterval(yi, yi1);
        //_yMap.setPaintInterval(sample.y(), nextSample.y());

         QRectF clipRect;
         if ( true  )
         {
             qreal pw = qMax( qreal( 1.0 ), painter->pen().widthF());
             clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);
         }

         const bool noDuplicates = true; //d_data->paintAttributes & FilterPoints;

         QwtPointMapper mapper;
         mapper.setFlag( QwtPointMapper::RoundPoints, true  );
         mapper.setFlag( QwtPointMapper::WeedOutPoints, true  );
         mapper.setBoundingRect( canvasRect );

         QVector<QPointF> samples;
         samples.append(QPointF(xi, yi));
         samples.append(QPointF(xi1, yi1));


         QwtPointSeriesData *data = new QwtPointSeriesData(samples);
         QPolygonF polyline = mapper.toPolygonF( _xMap, _yMap, data, 0, 1 );

         if ( format == QwtColorMap::RGB )
         {
             const QRgb rgb = d_data->colorMap->rgb(
                 d_data->colorRange, sample.z() );

             painter->setPen( QPen( QColor::fromRgba( rgb ), d_data->penWidth ) );
         }
         else
         {
             const unsigned char index = d_data->colorMap->colorIndex(
                 d_data->colorRange, sample.z() );

             painter->setPen( QPen( QColor::fromRgba( d_data->colorTable[index] ),
                 d_data->penWidth ) );
         }

         painter->setPen( QPen( Qt::green, 1 ) );
         qDebug() << "draw " << data->sample(0).x() << data->sample(0).y() << data->sample(1).x() << data->sample(1).y() ;

         if ( d_data->paintAttributes & true )
         {
             const QPolygonF clipped = QwtClipper::clipPolygonF(
                 clipRect, polyline, false );

             QwtPainter::drawPolyline( painter, clipped );
         }
         else
         {
             QwtPainter::drawPolyline( painter, polyline );
         }

     }

 }
