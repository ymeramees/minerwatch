/////////////////////////////////////////////////////////////////////////////
/// ToDo list:
/// Vaadata üle, mis asju loetakse conf failist kliendi puhul
/// Teha kliendi puhul restardi nupp halliks
/// Lisada getters ja setters KaevuriReale
///
/////////////////////////////////////////////////////////////////////////////

#include "minerwatch.h"
#include "ui_minerwatch.h"

extern bool verbose;

MinerWatch::MinerWatch(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MinerWatch)
{
    ui->setupUi(this);
    logi = new QFile("Log.txt");
    if(!logi->open(QIODevice::Append | QIODevice::Text) && verbose)
        QTextStream(stdout) << "Logi faili ei õnnestunud kirjutada!" << endl;

    server = false;
    udpSocket = new QUdpSocket(this);
    serial = new QSerialPort(this);
    connect(serial, SIGNAL(readyRead()), this, SLOT(readFromSerial()));

    //Vaikimisi seaded Arduino jaoks, loetakse conf failist üle
    settings.baudRate = QSerialPort::Baud9600;
    settings.dataBits = QSerialPort::Data8;
    settings.parity = QSerialPort::NoParity;
    settings.stopBits = QSerialPort::OneStop;
    settings.flowControl = QSerialPort::NoFlowControl;    //Vaikimisi Arduinol seda ei ole.
    findSerialName();

    noReplyTime = 120;
    heartBeatTime = 300;
    checkInterval = 5;
    maxRestartNo = 3;
    resetPressTime = 500;
    restartTime1=360;
    restartTime2=1800;
    restartTime3=7200;

    readConfFail();
    openSerial();

    checkTimer.setInterval(checkInterval * 1000);
    checkTimer.setSingleShot(false);
    connect(&checkTimer, SIGNAL(timeout()), this, SLOT(statusCheck()));
//    std::cout << "cout << start" << endl;
    if(verbose)
        QTextStream(stdout) << "start" << endl;
    writeToLog("Start");
}

MinerWatch::~MinerWatch()
{
    delete ui;
}

void MinerWatch::openSerial()
{
    serial->setPortName(settings.serialName);
    serial->setBaudRate(settings.baudRate);
    serial->setDataBits(settings.dataBits);
    serial->setParity(settings.parity);
    serial->setStopBits(settings.stopBits);
    serial->setFlowControl(settings.flowControl);
    if(serial->open(QIODevice::ReadWrite)){
        ui->statusBar->showMessage(tr("Serial port avatud, %1 : %2, %3, %4, %5, %6").arg(settings.serialName).arg(settings.baudRate).arg(settings.dataBits).arg(settings.parity).arg(settings.stopBits).arg(settings.flowControl));
    } else {
//        QMessageBox::critical(this, tr("Viga!"), QString("%1\n%2").arg(settings.serialName).arg(serial->errorString()));

        ui->statusBar->showMessage(tr("Serial pordi %1 avamise viga!").arg(settings.serialName));
    }
}

void MinerWatch::writeToLog(QString s)
{
    QTextStream valja(logi);
    valja.setCodec("UTF-8");
    valja << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << ": " << s << endl;
}

void MinerWatch::readConfFail()
{
    QFile file("config.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();

            if(!line.startsWith(';') && !line.startsWith('#') && !line.isEmpty()){
                if(line.contains(';')){ //Rida, kus on nimi ja number
                    QStringList read = line.split(';'); //Nimi;number
                    if(read.count() < 2)
                        return; //Rida oli liiga lühike
                    KaevuriRida* minerRow = new KaevuriRida();
                    connect(minerRow, SIGNAL(orderRestart(QString)), this, SLOT(doRestart(QString)));
                    minerRow->nameLabel->setText(read.at(0));
                    minerRow->portLabel->setText(read.at(1));
                    if(QString::compare(read.at(0), own_name) == 0){ //Juhuks, kui loetelus on ka juhtkaevur ise, et sellele restarti ei tehtaks
                        minerRow->statusLabel->setText("this");
                        minerRow->restartButton->setEnabled(false);
                    }

                    minerRows.append(minerRow);
                    ui->verticalLayout->addLayout(minerRows.last());
                }else if(line.startsWith("own_name=")){
                    own_name = line.mid(line.indexOf('=') + 1);
                    this->setWindowTitle("MinerWatch - " + own_name);
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): own_name=" << own_name << endl;
                }else if(line.startsWith("server=yes")){
                    server = true;
                    udpSocket->bind(57615, QUdpSocket::ShareAddress);
                    connect(udpSocket, SIGNAL(readyRead()), SLOT(readBroadcasts()));
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): server=yes" << endl;
                }else if(line.startsWith("server=no")){
                    server = false;
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): server=no" << endl;
                }else if(line.startsWith("baudRate=", Qt::CaseInsensitive)){
                    settings.baudRate = static_cast<QSerialPort::BaudRate>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): baudRate=" << settings.baudRate << endl;
                }else if(line.startsWith("dataBits=", Qt::CaseInsensitive)){
                    settings.dataBits = static_cast<QSerialPort::DataBits>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): dataBits=" << settings.dataBits << endl;
                }else if(line.startsWith("parity=", Qt::CaseInsensitive)){
                    settings.parity = static_cast<QSerialPort::Parity>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): parity=" << settings.parity << endl;
                }else if(line.startsWith("stopBits=", Qt::CaseInsensitive)){
                    settings.stopBits = static_cast<QSerialPort::StopBits>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): stopBits=" << settings.stopBits << endl;
                }else if(line.startsWith("heartBeatTime=", Qt::CaseInsensitive)){
                    heartBeatTime = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): heartBeatTime=" << heartBeatTime << endl;
                }else if(line.startsWith("checkInterval=", Qt::CaseInsensitive)){
                    checkInterval = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): checkInterval=" << checkInterval << endl;
                 }else if(line.startsWith("resetPressTime=", Qt::CaseInsensitive)){
                   resetPressTime = line.mid(line.indexOf('=') + 1).toInt();
                   if(verbose)
                       QTextStream(stdout) << "readConfFail(): resetPressTime=" << resetPressTime << endl;
                }else if(line.startsWith("maxRestartNo=", Qt::CaseInsensitive)){
                    maxRestartNo = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): maxRestartNo=" << maxRestartNo << endl;
                 }else if(line.startsWith("noReplyTime=", Qt::CaseInsensitive)){
                    noReplyTime = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): noReplyTime=" << noReplyTime << endl;
                 }else if(line.startsWith("restartTime1=", Qt::CaseInsensitive)){
                    restartTime1 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): restartTime1=" << restartTime1 << endl;
                 }else if(line.startsWith("restartTime2=", Qt::CaseInsensitive)){
                    restartTime2 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): restartTime2=" << restartTime2 << endl;
                 }else if(line.startsWith("restartTime3=", Qt::CaseInsensitive)){
                    restartTime3 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "readConfFail(): restartTime3=" << restartTime3 << endl;
                }else{
                    QTextStream(stdout) << "readConfFail(): unknown setting: " << line << endl;
                }
            }
        }
        ui->verticalLayout->addStretch();
        file.close();
    }else{
        QMessageBox::critical(this, "MinerWatch", "Configuration file not found!", QMessageBox::Ok);
    }
    checkTimer.setInterval(checkInterval * 1000);
    checkTimer.start();
}

void MinerWatch::readFromSerial()
{
    QString received(serial->readAll());
    if(verbose)
        QTextStream(stdout) << "Read from serial: " << received << endl;
    if(received.contains("Tere")){
        serial->write("a");
        if(verbose)
            QTextStream(stdout) << "Sent to serial: " << "a" << endl;
        serial->write(QByteArray::number(heartBeatTime));
        serial->waitForBytesWritten();
        if(verbose)
            QTextStream(stdout) << "Sent to serial: " << QByteArray::number(heartBeatTime) << endl;

        serial->write("d");
        if(verbose)
            QTextStream(stdout) << "Sent to serial: " << "d" << endl;
        serial->write(QByteArray::number(resetPressTime));
        if(verbose)
            QTextStream(stdout) << "Sent to serial: " << QByteArray::number(resetPressTime) << endl;
    }
}

void MinerWatch::readBroadcasts()
{
    QByteArray datagram;
    while (udpSocket->hasPendingDatagrams()) {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        statusBar()->showMessage(tr("Received broadcast: \"%1\"").arg(datagram.constData()), 5000);
        QString name(datagram);
        name.chop(name.length() - name.indexOf(' '));
        for(int i = 0; i < minerRows.count(); i++){
            if(QString::compare(minerRows[i]->nameLabel->text(), name) == 0){
                minerRows[i]->alive();
            }
        }
    }
}

void MinerWatch::findSerialName()
{
    ports = QSerialPortInfo::availablePorts();
    if(verbose)
        QTextStream(stdout) << "QSerialPortInfo::availablePorts() = " << ports.count() << endl;


    foreach (QSerialPortInfo port, ports) {
        if(verbose)
            QTextStream(stdout) << "QSerialPortInfo::portName() = " << port.portName() << " " << port.description() << " " << port.manufacturer() << " " << port.systemLocation() << endl;
        if(port.description().contains("Arduino", Qt::CaseInsensitive) || port.description().contains("ch341", Qt::CaseInsensitive) || port.description().contains("FTDI", Qt::CaseInsensitive) || port.description().contains("serial", Qt::CaseInsensitive)){
            settings.serialName = port.portName();  //Arduino leitud
            return;
        }
    }
}

void MinerWatch::statusCheck()
{
    if(server){ //Server loeb teateid
//        QMessageBox::information(this, "MinerWatch", "statusCheck", QMessageBox::Ok);
        QTime time(QTime::currentTime());
        serial->write("e"); //Arduinole enda elusolemise teate saatmine (e = olen elus)

        if(verbose)
            QTextStream(stdout) << time.toString("hh:mm:ss") << " Sending to serial: e" << endl;

        for(int i = 0; i < minerRows.count(); i++){
            if(minerRows[i]->timeLabel->text() != "00:00:00"){
//                minerRows[0]->portLabel->setText(QString("%1").arg(time.secsTo(QTime::fromString(minerRows[i]->timeLabel->text()))));
                if(time.secsTo(QTime::fromString(minerRows[i]->timeLabel->text())) < -restartTime1 && minerRows[i]->restartNo == 0 && !minerRows[i]->statusLabel->text().contains("this")){
                    QString package = QString("r%1").arg(minerRows[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << time.toString("hh:mm:ss") << " Sending to serial: " << package << endl;
                    serial->write(package.toLatin1());
                    minerRows[i]->statusLabel->setText("1. restart");
                    minerRows[i]->restartNo++;
                    writeToLog(minerRows[i]->nameLabel->text() + ", pin: " + minerRows[i]->portLabel->text() + ", last reply: " + minerRows[i]->timeLabel->text() + ", 1. restart");
                }else if(time.secsTo(QTime::fromString(minerRows[i]->timeLabel->text())) < -restartTime2 && minerRows[i]->restartNo == 1){
                    QString package = QString("r%1").arg(minerRows[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << time.toString("hh:mm:ss") << " Sending to serial: " << package << endl;
                    serial->write(package.toLatin1());
                    minerRows[i]->statusLabel->setText("2. restart");
                    minerRows[i]->restartNo++;
                    writeToLog(minerRows[i]->nameLabel->text() + ", pin: " + minerRows[i]->portLabel->text() + ", last reply: " + minerRows[i]->timeLabel->text() + ", 2. restart");
                }else if(time.secsTo(QTime::fromString(minerRows[i]->timeLabel->text())) < -restartTime3 && minerRows[i]->restartNo == 2){
                    QString package = QString("r%1").arg(minerRows[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << time.toString("hh:mm:ss") << " Sending to serial: " << package << endl;
                    serial->write(package.toLatin1());
                    minerRows[i]->statusLabel->setText("3. restart");
                    minerRows[i]->restartNo++;
                    writeToLog(minerRows[i]->nameLabel->text() + ", pin: " + minerRows[i]->portLabel->text() + ", last reply: " + minerRows[i]->timeLabel->text() + ", 3. restart");
                }else if(time.secsTo(QTime::fromString(minerRows[i]->timeLabel->text())) < -noReplyTime && !minerRows[i]->statusLabel->text().contains("restart", Qt::CaseInsensitive) && !minerRows[i]->statusLabel->text().contains("this")){
                    minerRows[i]->statusLabel->setText("no reply");
                }
            }
        }
    }else{  //Klient saadab elus olemise teate
        QByteArray datagram = own_name.toLatin1() + " alive";
        udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 57615);
        statusBar()->showMessage(tr("Saadetud teade: \"%1\"").arg(datagram.constData()), 5000);
    }

}

void MinerWatch::doRestart(QString s)
{
    QTime time(QTime::currentTime());
    QString package = QString("r%1").arg(s);   //Pesa nr on kirjas argumendis
    if(verbose)
        QTextStream(stdout) << time.toString("hh:mm:ss") << " Sending to serial: " << package << endl;
    serial->write(package.toLatin1());
    for(int i = 0; i < minerRows.count(); i++)
        if(minerRows[i]->portLabel->text().compare(s) == 0){
            minerRows[i]->statusLabel->setText("restart");
            writeToLog(minerRows[i]->nameLabel->text() + ", pin: " + minerRows[i]->portLabel->text() + ", last reply: " + minerRows[i]->timeLabel->text() + ", käsitsi restart");
        }
}
