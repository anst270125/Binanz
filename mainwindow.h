#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QtCharts>
#include "mychartview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QPair<double,double> getTsPlotRange();
    QPair<double,double> getPricePlotRange();
    void fitYAxis(QValueAxis *axisY=nullptr);

private slots:
    void on_pairCombo1_currentTextChanged(const QString &arg1);
    void on_pairCombo2_currentTextChanged(const QString &arg1);
    void on_addPairButton_clicked();
    void on_pairsList_itemChanged(QListWidgetItem *item);
    void on_radioButton_toggled(bool checked);
    void on_calendarWidget_activated(const QDate &date);

    void on_pushButton1y_clicked();
    void on_pushButton3m_clicked();
    void on_pushButton1m_clicked();
    void on_pushButton7d_clicked();
    void on_pushButton1d_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *nam;  
    MyChartView chartView;
    QMap<QString,bool> shownPairs;

    QMap<QString,QMap<double,double>> pairsData;
    QMap<QString,QMap<double,double>> origPairsData;

    void readWatchList();
    void step1_getPairData(QString pair,double endTime=0);
    void step2_pairDataReceived(QNetworkReply* reply);
    void step3_updateChart(double ts = 0);
    void adjustCalendarRange();
    void doPairPercentage(QString pairName, double referenceValue);

    template<typename inttype>
    void showTsRange(QDateTime(QDateTime::*func)(inttype) const,inttype tsRange);
    bool eventFilter(QObject *watched, QEvent *event) override;
};
#endif // MAINWINDOW_H
