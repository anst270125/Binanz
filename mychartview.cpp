#include "mychartview.h"
#include <mainwindow.h>

QTextStream outt(stdout);
int identt = -1;
#define tstart QElapsedTimer timer; timer.start();++identt;
#define tend(X) outt<<QString(identt,'~')<<X<<" "<<timer.elapsed()<<"\n"<<(identt == 0 ? "\n" : "");outt.flush();--identt;

MyChartView::MyChartView(MainWindow *mw): _mw(mw)
{
    setMouseTracking(true);
    setInteractive(true);
    setRubberBand(QChartView::HorizontalRubberBand);
    rubberBand = findChild<QRubberBand *>();
    rubberBand->installEventFilter(this);
}


void MyChartView::mouseReleaseEvent(QMouseEvent *event)
{
    QChartView::mouseReleaseEvent(event);

    //prevent too much zoom-out
    if(event->button() == Qt::RightButton || event->button() == Qt::LeftButton){
        tstart
        QPair<double,double> tsRange = _mw->getTsPlotRange();

        if(tsRange.second == 0)
            return;

        QValueAxis* axisX = static_cast<QValueAxis*>(chart()->axes(Qt::Horizontal)[0]);

        if(axisX->min() < tsRange.first)
            axisX->setMin(tsRange.first);

        if(axisX->max() > tsRange.second)
            axisX->setMax(tsRange.second);

        _mw->fitYAxis();
        tend("mousReleaseEvent")

    }
}

bool MyChartView::eventFilter(QObject *watched, QEvent *event)
{
    //prevent zooming past existing timestamps
    if(watched == rubberBand && event->type() == QEvent::Resize){
        tstart
        double top = chart()->mapToValue(rubberBand->geometry().topLeft()).y();
        double bottom = chart()->mapToValue(rubberBand->geometry().bottomRight()).y();
        double left = chart()->mapToValue(rubberBand->geometry().topLeft()).x();
        double right = chart()->mapToValue(rubberBand->geometry().bottomRight()).x();

        QPair<double,double> tsRange = _mw->getTsPlotRange();

        if(left<tsRange.first)
            rubberBand->setGeometry(QRect(chart()->mapToPosition(QPointF(tsRange.first,top)).toPoint(),
                                          chart()->mapToPosition(QPointF(right,bottom)).toPoint()));

        if(right > tsRange.second)
            rubberBand->setGeometry(QRect(chart()->mapToPosition(QPointF(left,top)).toPoint(),
                                          chart()->mapToPosition(QPointF(tsRange.second,bottom)).toPoint()));

        tend("eventFilter");
        return false;

    }

    return QChartView::eventFilter(watched, event);
}
