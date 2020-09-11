#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkReply>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>

QTextStream out(stdout);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , nam(new QNetworkAccessManager(this))
    , chartView(this)
{
    ui->setupUi(this);
    setWindowTitle("binanz");
    chartView.setRenderHint(QPainter::Antialiasing);
    chartView.resize(1800,600);
    chartView.show();

    connect(nam,&QNetworkAccessManager::finished,this,&MainWindow::step2_pairDataReceived);

}

MainWindow::~MainWindow()
{
    delete nam;
    delete ui;
}


void MainWindow::step1_getPairData(QString pair, double endTime)
{
    QString interval = ui->comboBox->currentText();
    QString limit = "1000";
    QString url = "https://api.binance.com/api/v3/klines?";

    url += "symbol=" + pair;
    url += "&interval=" + interval;
    url += "&limit=" + limit;

    if(endTime != 0)
        url += "&endTime=" + QString::number(static_cast<qint64>(endTime));

    nam->get(QNetworkRequest(url));
    ui->radioButton->setEnabled(false);
    ui->radioButton_2->setEnabled(false);
    ui->addPairButton->setEnabled(false);
    ui->pairEdit->clearFocus();

}

void MainWindow::step2_pairDataReceived(QNetworkReply* reply)
{
    QString url = reply->url().toString();

    QString pairName = QRegularExpression("symbol=(.*?)&").match(url).captured(1);
    QString limit = QRegularExpression("limit=(.*?)(&|$)").match(url).captured(1);

    QString data = QString(reply->readAll());
    data.remove(0,1);
    data.remove("\"");

    QMap<double,double> priceList;
    QRegularExpression chunksRegex("\\[(.*?)\\]");
    QRegularExpressionMatchIterator it = chunksRegex.globalMatch(data);

    if(!it.hasNext())
        return;

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QStringList fields = match.captured(1).split(",");

        if(fields.size()<11)
            return;

        priceList[fields[0].toDouble()] =  fields[1].toDouble();
    }

    if (pairsData.empty()) //prevent interval change after first downloaded pair
        ui->comboBox->setEnabled(false);

    if (pairsData.contains(pairName)){
        for(double ts : priceList.keys())
            pairsData[pairName][ts] = priceList[ts];
        step3_updateChart();
    }

    else{
        pairsData[pairName] = priceList;
        QListWidgetItem* item = new QListWidgetItem(pairName);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        ui->pairsList->addItem(item);
        item->setCheckState(Qt::Checked); // ---> updateChart()
    }

    if (priceList.size() >= limit.toInt())
        step1_getPairData(pairName,(priceList.constBegin().key()));

    else{
        ui->radioButton->setEnabled(true);
        ui->radioButton_2->setEnabled(true);
        ui->addPairButton->setEnabled(true);
    }

    adjustCalendarRange();

}


void MainWindow::step3_updateChart(double referenceTs)
{
    QChart* chart = new QChart;

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(15);
    axisX->setFormat("dd MMM yy");
    chart->addAxis(axisX, Qt::AlignBottom);


    QValueAxis *axisY = new QValueAxis;
    chart->addAxis(axisY, Qt::AlignLeft);

    for(QString& pairName : pairsData.keys()){

        if(!shownPairs[pairName])
            continue;

        QSplineSeries* series = new QSplineSeries;
        series->setName(pairName);

        for(double ts : pairsData[pairName].keys())
            series->append(ts, pairsData[pairName][ts]);

        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }


    QPair<double,double> tsRange = getTsPlotRange();
    axisX->setRange(QDateTime::fromMSecsSinceEpoch(tsRange.first),
                    QDateTime::fromMSecsSinceEpoch(tsRange.second));


    QChart* oldChart = chartView.chart();
    chartView.setChart(chart);
    if(oldChart != nullptr)
        delete oldChart;

    fitYAxis(axisY);
    //chart->setAnimationOptions(QChart::SeriesAnimations);  // laggy for some reason


//    if(referenceTs != 0){ //vertical lines on reference timestamps and 0%
//        QLineSeries* referenceLineV = new QLineSeries;
//        QLineSeries* referenceLineH = new QLineSeries;
//        QPen pen(QColor(150,200,255));
//        pen.setWidth(1);
//        referenceLineH->setPen(pen);
//        referenceLineV->setPen(pen);


//        referenceLineV->append(referenceTs,axisY->max());
//        referenceLineV->append(referenceTs,axisY->min());

//        referenceLineH->append(tsRange.first,0);
//        referenceLineH->append(tsRange.second,0);

//        chart->addSeries(referenceLineV);
//        referenceLineV->attachAxis(axisX);
//        referenceLineV->attachAxis(axisY);
//        chart->addSeries(referenceLineH);
//        referenceLineH->attachAxis(axisX);
//        referenceLineH->attachAxis(axisY);
//    }

}

void MainWindow::adjustCalendarRange()
{
    double min = 0, max = QDateTime::currentDateTime().toMSecsSinceEpoch();
    for (QString pairName : pairsData.keys()){
        if(!shownPairs[pairName])
            continue;

        if(pairsData[pairName].firstKey() > min)
            min = pairsData[pairName].firstKey();
        if(pairsData[pairName].lastKey() < max)
            max = pairsData[pairName].lastKey();

    }
    ui->calendarWidget->setDateRange(QDateTime::fromMSecsSinceEpoch(min).date(),
                                     QDateTime::fromMSecsSinceEpoch(max).date());
}

void MainWindow::doPairPercentage(QString pairName, double referenceValue)
{
    for(double ts : origPairsData[pairName].keys()){
        pairsData[pairName][ts] = (origPairsData[pairName][ts]*100)/referenceValue - 100;
    }
}

template<typename inttype>
void MainWindow::showTsRange(QDateTime (QDateTime::*func)(inttype) const, inttype tsRange)
{
    QDateTime timeNow = QDateTime::currentDateTime();
    QDateTime startTime = (timeNow.*func)(tsRange);
    double firstTSwithData = getTsPlotRange().first;

    if(firstTSwithData == 1e+16)
        return;

    if(startTime.toMSecsSinceEpoch() < firstTSwithData)
        startTime = QDateTime::fromMSecsSinceEpoch(firstTSwithData);

    static_cast<QDateTimeAxis*>(chartView.chart()->axes(Qt::Horizontal)[0])->setRange(startTime,timeNow);

    bool perctgMode = !ui->radioButton->isChecked();
    if(perctgMode){
        ui->calendarWidget->setSelectedDate(startTime.date());
        on_calendarWidget_activated(startTime.date()); //---> fitYAxis();
    }

    else
        fitYAxis();
}

QPair<double, double> MainWindow::getTsPlotRange()
{
    double tsMin = 10e15, tsMax = 0;

    for(QString pairName : pairsData.keys()){
        if(!shownPairs[pairName])
            continue;

        if(pairsData[pairName].firstKey() < tsMin)
            tsMin = pairsData[pairName].firstKey();
        if(pairsData[pairName].lastKey() > tsMax)
            tsMax = pairsData[pairName].lastKey();

    }
    return {tsMin,tsMax};
}

QPair<double, double> MainWindow::getPricePlotRange() // for visible region
{
    double priceMin = 10e8, priceMax = 0;

    QDateTimeAxis* axisX = static_cast<QDateTimeAxis*>(chartView.chart()->axes(Qt::Horizontal)[0]);

    double minVisibleTs = axisX->min().toMSecsSinceEpoch();
    double maxVisibleTs = axisX->max().toMSecsSinceEpoch();

    for(QString pairName : pairsData.keys()){

        if(!shownPairs[pairName])
            continue;

        auto startIt = pairsData[pairName].lowerBound(minVisibleTs);
        auto endIt = pairsData[pairName].lowerBound(maxVisibleTs);

        if(startIt == endIt)//pair data beyond current view
            continue;

        if(endIt == pairsData[pairName].end())
            endIt--;

        while(startIt != endIt+1){

            double price = startIt.value();
            if(price < priceMin)
                priceMin = price;
            if(price > priceMax)
                priceMax = price;

            ++startIt;
        }
    }
    return {priceMin,priceMax};
}

void MainWindow::fitYAxis(QValueAxis* axisY)
{
    if(axisY == nullptr)
        axisY = static_cast<QValueAxis*>(chartView.chart()->axes(Qt::Vertical)[0]);

    bool perctgMode = !ui->radioButton->isChecked();
    QPair<double,double> priceRange = getPricePlotRange();


    if(perctgMode){
        axisY->setLabelFormat("%i%");
        axisY->setTickType(QValueAxis::TickType::TicksDynamic);
        axisY->setTickAnchor(0);

        int tickInterval = priceRange.second-priceRange.first;

        if(tickInterval<20)
            tickInterval = 1;
        else if(tickInterval<100)
            tickInterval = 5;
        else if(tickInterval<200)
            tickInterval = 10;
        else if(tickInterval<400)
            tickInterval = 20;
        else if(tickInterval<500)
            tickInterval = 25;
        else if(tickInterval<1000)
            tickInterval = 50;
        else
            tickInterval = 100;

        axisY->setTickInterval(tickInterval);
        axisY->setRange(priceRange.first /*+ 0.05 * priceRange.first*/, priceRange.second /*+ 0.05 * priceRange.second*/);
    }

    else{
        axisY->setTickCount(10);
        int nrOf0s = log10(priceRange.second);
        axisY->setLabelFormat("%."+QString::number(4-nrOf0s)+"f");
        axisY->setRange(0,priceRange.second+priceRange.second*0.05);
    }
}



bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    return QMainWindow::eventFilter(watched, event);
}


void MainWindow::on_pairCombo1_currentTextChanged(const QString &)
{
    ui->pairEdit->setText(ui->pairCombo1->currentText()+ui->pairCombo2->currentText());
}

void MainWindow::on_pairCombo2_currentTextChanged(const QString &)
{
    ui->pairEdit->setText(ui->pairCombo1->currentText()+ui->pairCombo2->currentText());
}

void MainWindow::on_addPairButton_clicked()
{
    QString pairName = ui->pairEdit->text();

    if(pairsData.contains(pairName))
        return;

    step1_getPairData(pairName);
}

void MainWindow::on_pairsList_itemChanged(QListWidgetItem *item)
{
    shownPairs[item->text()] = (item->checkState() == Qt::Checked);
    step3_updateChart();
    adjustCalendarRange();
}

void MainWindow::on_radioButton_toggled(bool checked)
{
    ui->addPairButton->setEnabled(checked);
    if(checked){
        ui->calendarWidget->setEnabled(false);
        pairsData = origPairsData;
        step3_updateChart();
    }
    else{
        ui->calendarWidget->setEnabled(true);
        origPairsData = pairsData;
        on_pushButton1m_clicked();
    }
}

void MainWindow::on_calendarWidget_activated(const QDate &date)
{
    double ts = QDateTime(date).toMSecsSinceEpoch();

    for(QString pairName: pairsData.keys()){
        double referenceValue = origPairsData[pairName].lowerBound(ts).value();
        doPairPercentage(pairName,referenceValue);
    }

    step3_updateChart(ts);

    bool perctgMode = !ui->radioButton->isChecked();
    if(perctgMode){
        static_cast<QDateTimeAxis*>(chartView.chart()->axes(Qt::Horizontal)[0])->setRange(QDateTime::fromMSecsSinceEpoch(ts),QDateTime::currentDateTime());
        fitYAxis();
    }

}

void MainWindow::on_pushButton1y_clicked()
{
    showTsRange(&QDateTime::addYears,-1);
}

void MainWindow::on_pushButton3m_clicked()
{
    showTsRange(&QDateTime::addMonths,-3);
}

void MainWindow::on_pushButton1m_clicked()
{
    showTsRange(&QDateTime::addMonths,-1);
}

void MainWindow::on_pushButton7d_clicked()
{
    showTsRange(&QDateTime::addDays,qint64(-7));
}

void MainWindow::on_pushButton1d_clicked()
{
    showTsRange(&QDateTime::addDays,qint64(-1));
}
