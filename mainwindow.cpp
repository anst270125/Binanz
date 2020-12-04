#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QNetworkReply>
#include <QTextStream>
#include <QDateTime>
#include <algorithm>

QTextStream out(stdout);
int ident=-1;
#define tstart //QElapsedTimer timerAll,timerLap; timerAll.start();timerLap.start();++ident;
#define tlap(X) //out<<QString(ident,'~')<<X<<" "<<timerLap.elapsed()<<"\n";timerLap.restart();
#define tslap //timerLap.restart();
#define tend(X) //out<<QString(ident,'-')<<X<<" "<<timerAll.elapsed()<<"\n"<<(ident == 0 ? "\n" : "");out.flush();--ident;

QMap<QString,double> intervalMSecs  {{"1m",60000},
                                     {"3m",60000*3},
                                     {"5m",60000*5},
                                     {"15m",60000*15},
                                     {"30m",60000*30},
                                     {"1h",3.6e+6},
                                     {"2h",3.6e+6*2},
                                     {"4h",3.6e+6*4},
                                     {"6h",3.6e+6*6},
                                     {"12h",3.6e+6*12},
                                     {"1d",8.64e+7},
                                     {"3d",8.64e+7*3}};



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , valueLabel(nullptr)
    , nam(new QNetworkAccessManager(this))
    , chartView(this)
    , downloading(0)
{
    nam->get(QNetworkRequest(QString())); //first get slow solver
    ui->setupUi(this);
    setWindowTitle("binanz");
    chartView.setRenderHint(QPainter::Antialiasing);
    chartView.resize(1800,600);
    chartView.show();


    ui->tableWidget->setColumnWidth(0, 100);
    for(int i = 1; i<ui->tableWidget->columnCount(); ++i)
        ui->tableWidget->setColumnWidth(i, 60);

    connect(nam,&QNetworkAccessManager::finished,this,&MainWindow::step2_pairDataReceived);
    readWatchList();
    QDir().mkdir("data");
}

MainWindow::~MainWindow()
{
    delete nam;
    delete ui;
}


void MainWindow::step1_getPairData(QString pair)
{
    uiEnableDisable(true);
    if(QFile::exists("data/"+pair+".txt")){
        loadPairData(pair);
    }
    downloadPairData(pair);
}

void MainWindow::step2_pairDataReceived(QNetworkReply* reply)
{    
    QString url = reply->url().toString();
    if(url.isEmpty()) // first slow get solver handler
        return;

    tstart
    QString pairName = QRegularExpression("symbol=(.*?)&").match(url).captured(1);
    int limit = QRegularExpression("limit=(.*?)(&|$)").match(url).captured(1).toInt();

    QString data = QString(reply->readAll());
    data.remove(0,1);
    data.remove("\"");

    int items = 0;
    double firstTs;
    double newDataStartTs = 0;
    bool firstChunk = !pairsData.contains(pairName);
    bool itAtExistingData = false;

    QRegularExpression chunksRegex("\\[(.*?)\\]");
    QRegularExpressionMatchIterator it = chunksRegex.globalMatch(data);

    if(!it.hasNext()){ // bad pair, reenable UI and return
        uiEnableDisable(false);
        return;
    }

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QStringList fields = match.captured(1).split(",");

        if(fields.size()<11){ // bad pair, reenable UI and return
            uiEnableDisable(false);
            return;
        }

        double ts = fields[0].toDouble();
        double value = fields[1].toDouble();

        if(items == 0)
            firstTs = ts;

        if(pairsData[pairName].contains(ts)){ // if data already exists, ignore
            itAtExistingData = true;
            continue;
        }
        if(itAtExistingData && !pairsData[pairName].contains(ts)){ // at first new data (after already existing/duplicates)
            newDataStartTs = ts;                                   // save first new timestamp and prevent overwrite
            itAtExistingData = false;
        }

        pairsData[pairName][ts] =  value;
        ++items;
    }


    if (!firstChunk) //if pair chunks 2->n
        step3_updateChart();

    else // if first chunk of pair data
       addTableRow(pairName); // ---> step3_updateChart


    if((items < limit) || newDataStartTs != 0){  // if no data left to download
        uiEnableDisable(false);
        adjustCalendarRange();
        doPriceHistory(pairName);

        if(!itAtExistingData) // if only new data, or some new data
            savePairData(pairName,newDataStartTs);
    }

   else // if there should still be data, download it
        downloadPairData(pairName,firstTs-1);
   tend("step2_pairDataReceived " + pairName)


}


void MainWindow::step3_updateChart(double referenceTs, bool doFitYAxis)
{
    tstart
    QChart* chart = new QChart;
    //chart->setTheme(QChart::ChartThemeDark);
    if(valueLabel != nullptr) //delete old one
        delete valueLabel;

    valueLabel = new QGraphicsTextItem(chart);

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
        series->setUseOpenGL(true); //only if it worked with spline series

        connect(series,&QSplineSeries::hovered, this, &MainWindow::seriesHovered);

        for(double ts : pairsData[pairName].keys())
            series->append(ts, pairsData[pairName][ts]);

        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);
        QPen pen = series->pen();
        pen.setWidth(3);
        series->setPen(pen);
    }
    tslap

  //  QPair<double,double> tsRange = getTsPlotRange(); // min(firstTS(data)) , max(lastTS(data))  // can be removes i guess
  //  axisX->setRange(QDateTime::fromMSecsSinceEpoch(tsRange.first),
  //                  QDateTime::fromMSecsSinceEpoch(tsRange.second));


    QChart* oldChart = chartView.chart();
    chartView.setChart(chart);
    tlap("setchart")
    if(oldChart != nullptr)
        delete oldChart;

    if(doFitYAxis)
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
    tend("updateChart ")

}

void MainWindow::downloadPairData(QString pair, double endTime)
{
    QString interval = "4h";
    QString limit = "1000";
    QString url = "https://api.binance.com/api/v3/klines?";

    url += "symbol=" + pair;
    url += "&interval=" + interval;
    url += "&limit=" + limit;

    if(endTime != 0)
        url += "&endTime=" + QString::number(static_cast<qint64>(endTime));

    nam->get(QNetworkRequest(url));  
    ui->pairEdit->clearFocus();
}

void MainWindow::loadPairData(QString pairName)
{
    tstart
    QFile file("data/"+pairName+".txt");
    file.open(QFile::ReadOnly);

    while(!file.atEnd()){
        QByteArray line = file.readLine();
        double ts = line.left(13).toDouble();
        double val = line.mid(13).toDouble();
        pairsData[pairName][ts] = val;
    }

    addTableRow(pairName);
    tend("loadPairData "+pairName);
}

void MainWindow::doPriceHistory(QString pairName)
{
    tstart
    int row = ui->tableWidget->findItems(pairName,Qt::MatchExactly)[0]->row(); // find which row in table is pair
    double lastPrice = pairsData[pairName].last();

    double interval = intervalMSecs["4h"];

    QList<double> historyTss;
    historyTss << QDateTime::currentDateTime().addSecs(-3600 * 24).toMSecsSinceEpoch(); //24h
    historyTss << QDateTime::currentDateTime().addDays(-7).toMSecsSinceEpoch();         //1w
    historyTss << QDateTime::currentDateTime().addMonths(-1).toMSecsSinceEpoch();       //1m
    historyTss << QDateTime::currentDateTime().addMonths(-3).toMSecsSinceEpoch();       //3m
    historyTss << QDateTime::currentDateTime().addYears(-1).toMSecsSinceEpoch();        //1y

    for(int i = 0; i<historyTss.size();++i){ // calculate perc inc/dec,  write in table, and color accordingly
        auto it = pairsData[pairName].lowerBound(historyTss[i]);
        if(it != pairsData[pairName].end() && it.key() - historyTss[i] <= interval){ // if no data in range (ts, ts+interval), ignore
            double percChange = ((lastPrice - it.value()) / abs(it.value())) * 100;
            ui->tableWidget->item(row,i+1)->setText(QString::number(percChange,'f',1)+"%");

            if(percChange>0)
                ui->tableWidget->item(row,i+1)->setForeground(QBrush(QColor(80, 175, 0)));
            else
                ui->tableWidget->item(row,i+1)->setForeground(QBrush(QColor(225, 80, 60)));
        }
    }

    tend("doPriceHistory "+pairName)
}

void MainWindow::addTableRow(QString pairName)
{
    tstart
    QTableWidgetItem* item = new QTableWidgetItem(pairName);
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    ui->tableWidget->insertRow(ui->tableWidget->rowCount() );
    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,0, item);
    item->setCheckState(Qt::Checked); // ---> updateChart()

    for(int i = 1; i< ui->tableWidget->columnCount(); ++i){
        QTableWidgetItem *item = new QTableWidgetItem("-");
        item->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->setItem(ui->tableWidget->rowCount()-1,i, item);
    }
    tend("addTableRow " + pairName)
}

void MainWindow::savePairData(QString pair, double newDataStartTs)
{
    tstart
    QFile file("data/"+pair+".txt");
    if(!file.exists())
        file.open(QIODevice::WriteOnly);
    else
        file.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream fileStream(&file);
    fileStream.setRealNumberNotation(QTextStream::FixedNotation);

    auto it = pairsData[pair].lowerBound(newDataStartTs); //either newDataStartTs or first element
    while(it != pairsData[pair].end()){
        fileStream.setRealNumberPrecision(0);
        fileStream<<it.key()<<" ";
        fileStream.setRealNumberPrecision(6);
        fileStream<<it.value()<<'\n';
        ++it;
    }
    tend("savePairData "+pair)

}

void MainWindow::adjustCalendarRange() // adjust selectable dates in calendar, based on max(firstTS(data))
{
    QPair<double,double> tsRange = getTsPlotRange();
    ui->calendarWidget->setDateRange(QDateTime::fromMSecsSinceEpoch(tsRange.first).date(),
                                     QDateTime::fromMSecsSinceEpoch(tsRange.second).date());
}

void MainWindow::doPairPercentage(QString pairName, double referenceValue) // percentage change strategy
{
    for(double ts : origPairsData[pairName].keys())
        pairsData[pairName][ts] = ((origPairsData[pairName][ts] - referenceValue) / abs(referenceValue)) * 100;

}

void MainWindow::uiEnableDisable(bool disable)
{
    if(disable){
        ++downloading;
        if(downloading == 1){
            ui->radioButton->setEnabled(false); // disable UI stuff until data downloaded
            ui->radioButton_2->setEnabled(false);
            ui->addPairButton->setEnabled(false);
        }
    }
    else{
        --downloading;
        if(downloading == 0){
            ui->radioButton->setEnabled(true); // disable UI stuff until data downloaded
            ui->radioButton_2->setEnabled(true);
            ui->addPairButton->setEnabled(true);
        }
    }
}

template<typename inttype> // show last 1d, 7d, 1m, 3m, 1y
void MainWindow::showTsRange(QDateTime (QDateTime::*func)(inttype) const, inttype tsRange)
{
    QDateTime timeNow = QDateTime::currentDateTime();
    QDateTime startTime = (timeNow.*func)(tsRange);
    bool perctgMode = !ui->radioButton->isChecked();

    double firstTSwithData = getTsPlotRange().first;

    if(firstTSwithData == 1e+16) // dont crash on history buttons when  no data
        return;
    if(startTime.toMSecsSinceEpoch() < firstTSwithData)
        startTime = QDateTime::fromMSecsSinceEpoch(firstTSwithData);

    if(perctgMode){ // in perc mode, also sync data's 0% point
        ui->calendarWidget->setSelectedDate(startTime.date());
        on_calendarWidget_activated(startTime.date()); //---> fitYAxis(); , setRange();
    }

    else{
        static_cast<QDateTimeAxis*>(chartView.chart()->axes(Qt::Horizontal)[0])->setRange(startTime,timeNow);
        fitYAxis();
    }
}

QPair<double, double> MainWindow::getTsPlotRange()
{
    bool perctgMode = !ui->radioButton->isChecked();

    double tsMin = perctgMode ? 0 : 1e+16;
    double tsMax = perctgMode ? 1e+16 : 0;

    for(QString pairName : pairsData.keys()){
        if(!shownPairs[pairName])
            continue;

        if(perctgMode){ // if percMode return max(firstTS(data)), min(lastTS(data)), else min(firstTS(data)), max(lastTS(data))
            if(pairsData[pairName].firstKey() > tsMin)
                tsMin = pairsData[pairName].firstKey();
            if(pairsData[pairName].lastKey() < tsMax)
                tsMax = pairsData[pairName].lastKey();
        }
        else{
            if(pairsData[pairName].firstKey() < tsMin)
                tsMin = pairsData[pairName].firstKey();
            if(pairsData[pairName].lastKey() > tsMax)
                tsMax = pairsData[pairName].lastKey();
        }

    }
    return {tsMin,tsMax};
}

QPair<double, double> MainWindow::getPricePlotRange() // get price/% range in visible TS range
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

        if(startIt == endIt)//pair data not in visible ts range, ignore pair
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

void MainWindow::fitYAxis(QValueAxis* axisY) // vertical zoom on y axis to fit visible range
{
    tstart
    if(axisY == nullptr)
        axisY = static_cast<QValueAxis*>(chartView.chart()->axes(Qt::Vertical)[0]);

    bool perctgMode = !ui->radioButton->isChecked();
    QPair<double,double> priceRange = getPricePlotRange();


    if(perctgMode){ // in perc mode: 1%,5%,10%,20%,50%,100% ticks
        axisY->setLabelFormat("%i%");
        axisY->setTickType(QValueAxis::TickType::TicksDynamic);
        axisY->setTickAnchor(0);

        qreal tickInterval = priceRange.second-priceRange.first;

        axisY->setLabelFormat((tickInterval <= 10) ? "%.1f%" : "%i%"); // for % range < 10, add one digit to axis label
        if(tickInterval<10)
            tickInterval = 0.5;
        else if(tickInterval<20)
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
        tslap
        axisY->setRange(priceRange.first - abs(0.05 * priceRange.first), priceRange.second + abs(0.05 * priceRange.second));
        tlap("setyrange")
    }

    else{
        axisY->setTickCount(10);
        int nrOfDigits = log10(priceRange.second)+1; // total number of digits is 5, so label like : 12351, 1400.4, 123.25, 13.123 ...
        axisY->setLabelFormat("%."+QString::number(5-nrOfDigits)+"f");       
        axisY->setRange(0,priceRange.second+priceRange.second*0.05); // yAxisMax + 5% padding above
    }
    tend("fitYAxis")
}



bool MainWindow::eventFilter(QObject *watched, QEvent *event) // no idea
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


void MainWindow::on_radioButton_toggled(bool checked)
{
    tstart
    ui->addPairButton->setEnabled(checked);
    if(checked){ // normal mode
        ui->calendarWidget->setEnabled(false);
        pairsData = origPairsData; // restore original data
        step3_updateChart();
    }
    else{ // perc mode
        adjustCalendarRange();
        ui->calendarWidget->setEnabled(true);
        origPairsData = pairsData; // backup original data
        on_pushButton1m_clicked(); // -----> updateChart()   1m history default view in perc mode
    }
    tend("radioButtonToggled")
}

void MainWindow::on_calendarWidget_activated(const QDate &date) // on date clicked
{
    tstart
    double ts = QDateTime(date).toMSecsSinceEpoch();

    for(QString pairName: pairsData.keys()){
        double referenceValue = origPairsData[pairName].lowerBound(ts).value(); // reference value is first after date timestamp
        doPairPercentage(pairName,referenceValue);
    }
    step3_updateChart(ts,false); //fitYAxis after setting X range

    bool perctgMode = !ui->radioButton->isChecked();
    if(perctgMode){ // in perc mode(??) zoom both axes accordingly
        tslap
        static_cast<QDateTimeAxis*>(chartView.chart()->axes(Qt::Horizontal)[0])->setRange(QDateTime::fromMSecsSinceEpoch(ts),QDateTime::currentDateTime());
        tlap("setxrange")

        fitYAxis();
    }
    tend("date selected")
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

void MainWindow::readWatchList()
{
    QFile file("watchlist.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    statusBar()->showMessage("Importing watchlist...",1500);
    QTextStream in(&file);
    QStringList pairs;
    while (!in.atEnd())
        pairs<<in.readLine();

    pairs.removeDuplicates();
    for(QString pair : pairs)
        step1_getPairData(pair);
}

void MainWindow::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    if(item->column() != 0) // if changes in history columns we dont care
        return;

    bool cellState = item->checkState() == Qt::Checked;
    if(!(shownPairs[item->text()] != cellState)) // if change is other than check/uncheck, return
        return;

    shownPairs[item->text()] = cellState;

    if(!ui->radioButton->isChecked()) //in percmode get in default last month view
        on_pushButton1m_clicked(); // ---> updateChart
    else
        step3_updateChart();

    adjustCalendarRange();
}

void MainWindow::on_pushButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://www.blockchain.com/btc/address/13SBQhN2BJr5DxcYeEciWqxjN6QLt8XRDu"));
}

void MainWindow::on_pushButton_2_clicked()
{
    QDesktopServices::openUrl(QUrl("https://www.blockchain.com/eth/address/0xD2F304a5428234D488F5Ce38DAB3099c43937E0A"));
}

void MainWindow::seriesHovered(QPointF point,bool hover)
{
    QSplineSeries* series = static_cast<QSplineSeries*>(sender()); // get hovered series
    QPen pen = series->pen();

    if(hover){
        QString label;
        if(!ui->radioButton->isChecked())
            label = QString::number(point.y(),'f',2)+"%";
        else
             label = QString::number(point.y());

        pen.setWidth(5); //highlight
        valueLabel->setPlainText(QString("X: %1 \nY: %2 ").arg(point.x()).arg(point.y()));
        valueLabel->setHtml(QString("<div style='background-color: #eeeeff; font-size: 14px;'>") +label+"</div>"); // padding, border not working
        valueLabel->setPos(this->chartView.chart()->mapToPosition(point) + QPoint(-20, -30)); //convert position to axis value
        valueLabel->setZValue(10); // raise over series
        valueLabel->show();
    }
    else {
        pen.setWidth(3); //lowlight
        valueLabel->hide();
    }
    series->setPen(pen);
}
