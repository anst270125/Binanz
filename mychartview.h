#ifndef MYCHARTVIEW_H
#define MYCHARTVIEW_H
#include <QtCharts>

class MainWindow;

class MyChartView : public QChartView
{
public:
    MyChartView(MainWindow* mw);

private:
    void mouseReleaseEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    QRubberBand * rubberBand;
    MainWindow* _mw;
};

#endif // MYCHARTVIEW_H
