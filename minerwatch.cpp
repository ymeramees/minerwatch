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
    connect(serial, SIGNAL(readyRead()), this, SLOT(loeSerialist()));

    //Vaikimisi seaded Arduino jaoks, loetakse conf failist üle
    settings.baudRate = QSerialPort::Baud9600;
    settings.dataBits = QSerialPort::Data8;
    settings.parity = QSerialPort::NoParity;
    settings.stopBits = QSerialPort::OneStop;
    settings.flowControl = QSerialPort::NoFlowControl;    //Vaikimisi Arduinol seda ei ole.
    otsiSerialiNimi();

    doesNotReplyTime = 120;
    heartBeatTime = 300;
    checkInterval = 5;
    maxRestartNo = 3;
    resetPressTime = 500;
    restartTime1=360;
    restartTime2=1800;
    restartTime3=7200;

    loeConfFail();
    avaSerial();

    kontroll.setInterval(checkInterval * 1000);
    kontroll.setSingleShot(false);
    connect(&kontroll, SIGNAL(timeout()), this, SLOT(staatuseKontroll()));
//    std::cout << "cout << start" << endl;
    if(verbose)
        QTextStream(stdout) << "start" << endl;
    kirjutaLogisse("Start");
}

MinerWatch::~MinerWatch()
{
    delete ui;
}

void MinerWatch::avaSerial()
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

void MinerWatch::kirjutaLogisse(QString s)
{
    QTextStream valja(logi);
    valja.setCodec("UTF-8");
    valja << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << ": " << s << endl;
}

void MinerWatch::loeConfFail()
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
                    KaevuriRida* kaevuriRida = new KaevuriRida();
                    connect(kaevuriRida, SIGNAL(orderRestart(QString)), this, SLOT(teeRestart(QString)));
                    kaevuriRida->nameLabel->setText(read.at(0));
                    kaevuriRida->portLabel->setText(read.at(1));
                    if(QString::compare(read.at(0), oma_nimi) == 0){ //Juhuks, kui loetelus on ka juhtkaevur ise, et sellele restarti ei tehtaks
                        kaevuriRida->statusLabel->setText("ise");
                        kaevuriRida->restartButton->setEnabled(false);
                    }

                    kaevuriRead.append(kaevuriRida);
                    ui->verticalLayout->addLayout(kaevuriRead.last());
                }else if(line.startsWith("oma_nimi=")){
                    oma_nimi = line.mid(line.indexOf('=') + 1);
                    this->setWindowTitle("MinerWatch - " + oma_nimi);
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): oma_nimi=" << oma_nimi << endl;
                }else if(line.startsWith("server=yes")){
                    server = true;
                    udpSocket->bind(57615, QUdpSocket::ShareAddress);
                    connect(udpSocket, SIGNAL(readyRead()), SLOT(loeTeateid()));
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): server=yes" << endl;
                }else if(line.startsWith("server=no")){
                    server = false;
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): server=no" << endl;
                }else if(line.startsWith("baudRate=", Qt::CaseInsensitive)){
                    settings.baudRate = static_cast<QSerialPort::BaudRate>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): baudRate=" << settings.baudRate << endl;
                }else if(line.startsWith("dataBits=", Qt::CaseInsensitive)){
                    settings.dataBits = static_cast<QSerialPort::DataBits>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): dataBits=" << settings.dataBits << endl;
                }else if(line.startsWith("parity=", Qt::CaseInsensitive)){
                    settings.parity = static_cast<QSerialPort::Parity>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): parity=" << settings.parity << endl;
                }else if(line.startsWith("stopBits=", Qt::CaseInsensitive)){
                    settings.stopBits = static_cast<QSerialPort::StopBits>(line.mid(line.indexOf('=') + 1).toInt());
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): stopBits=" << settings.stopBits << endl;
                }else if(line.startsWith("heartBeatTime=", Qt::CaseInsensitive)){
                    heartBeatTime = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): heartBeatTime=" << heartBeatTime << endl;
                }else if(line.startsWith("checkInterval=", Qt::CaseInsensitive)){
                    checkInterval = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): checkInterval=" << checkInterval << endl;
                 }else if(line.startsWith("resetPressTime=", Qt::CaseInsensitive)){
                   resetPressTime = line.mid(line.indexOf('=') + 1).toInt();
                   if(verbose)
                       QTextStream(stdout) << "loeConfFail(): resetPressTime=" << resetPressTime << endl;
                }else if(line.startsWith("maxRestartNo=", Qt::CaseInsensitive)){
                    maxRestartNo = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): maxRestartNo=" << maxRestartNo << endl;
                 }else if(line.startsWith("doesNotReplyTime=", Qt::CaseInsensitive)){
                    doesNotReplyTime = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): doesNotReplyTime=" << doesNotReplyTime << endl;
                 }else if(line.startsWith("restartTime1=", Qt::CaseInsensitive)){
                    restartTime1 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): restartTime1=" << restartTime1 << endl;
                 }else if(line.startsWith("restartTime2=", Qt::CaseInsensitive)){
                    restartTime2 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): restartTime2=" << restartTime2 << endl;
                 }else if(line.startsWith("restartTime3=", Qt::CaseInsensitive)){
                    restartTime3 = line.mid(line.indexOf('=') + 1).toInt();
                    if(verbose)
                        QTextStream(stdout) << "loeConfFail(): restartTime3=" << restartTime3 << endl;
                 }
            }
        }
        ui->verticalLayout->addStretch();
        file.close();
    }else{
        QMessageBox::critical(this, "MinerWatch", "Konfiguratsiooni faili ei leitud!", QMessageBox::Ok);
    }
    kontroll.setInterval(checkInterval * 1000);
    kontroll.start();
}

void MinerWatch::loeSerialist()
{
    QString saabunud(serial->readAll());
    if(verbose)
        QTextStream(stdout) << "Serialist loetud: " << saabunud << endl;
    if(saabunud.contains("Tere")){
        serial->write("a");
        if(verbose)
            QTextStream(stdout) << "Serialisse saadetud: " << "a" << endl;
        serial->write(QByteArray::number(heartBeatTime));
        serial->waitForBytesWritten();
        if(verbose)
            QTextStream(stdout) << "Serialisse saadetud: " << QByteArray::number(heartBeatTime) << endl;

        serial->write("d");
        if(verbose)
            QTextStream(stdout) << "Serialisse saadetud: " << "d" << endl;
        serial->write(QByteArray::number(resetPressTime));
        if(verbose)
            QTextStream(stdout) << "Serialisse saadetud: " << QByteArray::number(resetPressTime) << endl;
    }
}

void MinerWatch::loeTeateid()
{
    QByteArray datagram;
    while (udpSocket->hasPendingDatagrams()) {
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        udpSocket->readDatagram(datagram.data(), datagram.size());
        statusBar()->showMessage(tr("Saadud teade: \"%1\"").arg(datagram.constData()), 5000);
        QString nimi(datagram);
        nimi.chop(nimi.length() - nimi.indexOf(' '));
        for(int i = 0; i < kaevuriRead.count(); i++){
            if(QString::compare(kaevuriRead[i]->nameLabel->text(), nimi) == 0){
                kaevuriRead[i]->alive();
            }
        }
    }
}

void MinerWatch::otsiSerialiNimi()
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

void MinerWatch::staatuseKontroll()
{
    if(server){ //Server loeb teateid
//        QMessageBox::information(this, "MinerWatch", "Staatusekontroll", QMessageBox::Ok);
        QTime aeg(QTime::currentTime());
        serial->write("e"); //Arduinole enda elusolemise teate saatmine (e = olen elus)

        if(verbose)
            QTextStream(stdout) << aeg.toString("hh:mm:ss") << " Saadan serialisse: e" << endl;

        for(int i = 0; i < kaevuriRead.count(); i++){
            if(kaevuriRead[i]->timeLabel->text() != "00:00:00"){
//                kaevuriRead[0]->portLabel->setText(QString("%1").arg(aeg.secsTo(QTime::fromString(kaevuriRead[i]->timeLabel->text()))));
                if(aeg.secsTo(QTime::fromString(kaevuriRead[i]->timeLabel->text())) < -restartTime1 && kaevuriRead[i]->restartNo == 0 && !kaevuriRead[i]->statusLabel->text().contains("ise")){
                    QString saadetis = QString("r%1").arg(kaevuriRead[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << aeg.toString("hh:mm:ss") << " Saadan serialisse: " << saadetis << endl;
                    serial->write(saadetis.toLatin1());
                    kaevuriRead[i]->statusLabel->setText("1. restart");
                    kaevuriRead[i]->restartNo++;
                    kirjutaLogisse(kaevuriRead[i]->nameLabel->text() + ", pesa: " + kaevuriRead[i]->portLabel->text() + ", viimane vastus: " + kaevuriRead[i]->timeLabel->text() + ", 1. restart");
                }else if(aeg.secsTo(QTime::fromString(kaevuriRead[i]->timeLabel->text())) < -restartTime2 && kaevuriRead[i]->restartNo == 1){
                    QString saadetis = QString("r%1").arg(kaevuriRead[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << aeg.toString("hh:mm:ss") << " Saadan serialisse: " << saadetis << endl;
                    serial->write(saadetis.toLatin1());
                    kaevuriRead[i]->statusLabel->setText("2. restart");
                    kaevuriRead[i]->restartNo++;
                    kirjutaLogisse(kaevuriRead[i]->nameLabel->text() + ", pesa: " + kaevuriRead[i]->portLabel->text() + ", viimane vastus: " + kaevuriRead[i]->timeLabel->text() + ", 2. restart");
                }else if(aeg.secsTo(QTime::fromString(kaevuriRead[i]->timeLabel->text())) < -restartTime3 && kaevuriRead[i]->restartNo == 2){
                    QString saadetis = QString("r%1").arg(kaevuriRead[i]->portLabel->text());   //Pesa nr on kirjas sildil
                    if(verbose)
                        QTextStream(stdout) << aeg.toString("hh:mm:ss") << " Saadan serialisse: " << saadetis << endl;
                    serial->write(saadetis.toLatin1());
                    kaevuriRead[i]->statusLabel->setText("3. restart");
                    kaevuriRead[i]->restartNo++;
                    kirjutaLogisse(kaevuriRead[i]->nameLabel->text() + ", pesa: " + kaevuriRead[i]->portLabel->text() + ", viimane vastus: " + kaevuriRead[i]->timeLabel->text() + ", 3. restart");
                }else if(aeg.secsTo(QTime::fromString(kaevuriRead[i]->timeLabel->text())) < -doesNotReplyTime && !kaevuriRead[i]->statusLabel->text().contains("restart", Qt::CaseInsensitive) && !kaevuriRead[i]->statusLabel->text().contains("ise")){
                    kaevuriRead[i]->statusLabel->setText("ei vasta");
                }
            }
        }
    }else{  //Klient saadab elus olemise teate
        QByteArray datagram = oma_nimi.toLatin1() + " alive";
        udpSocket->writeDatagram(datagram, QHostAddress::Broadcast, 57615);
        statusBar()->showMessage(tr("Saadetud teade: \"%1\"").arg(datagram.constData()), 5000);
    }

}

void MinerWatch::teeRestart(QString s)
{
    QTime aeg(QTime::currentTime());
    QString saadetis = QString("r%1").arg(s);   //Pesa nr on kirjas argumendis
    if(verbose)
        QTextStream(stdout) << aeg.toString("hh:mm:ss") << " Saadan serialisse: " << saadetis << endl;
    serial->write(saadetis.toLatin1());
    for(int i = 0; i < kaevuriRead.count(); i++)
        if(kaevuriRead[i]->portLabel->text().compare(s) == 0){
            kaevuriRead[i]->statusLabel->setText("restart");
            kirjutaLogisse(kaevuriRead[i]->nameLabel->text() + ", pesa: " + kaevuriRead[i]->portLabel->text() + ", viimane vastus: " + kaevuriRead[i]->timeLabel->text() + ", käsitsi restart");
        }
}
